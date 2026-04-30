#include "esp.h"
#include "../core/memory.h"
#include "../core/offsets.h"
#include "../core/overlay.h"
#include "../core/renderer.h"
#include "../utils/debug_log.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cwchar>

namespace cs2::esp {

using namespace memory;

struct ESPEntity {
    uintptr_t pawn;
    Vector3 origin;
    Vector3 head_pos;
    Vector2 screen_origin;
    Vector2 screen_head;
    int health;
    int team;
    float distance;
    bool valid;
    std::string name;
    std::string weapon_name;
    int bone_positions[BoneIndex::MAX_BONES * 2];
    bool bones_valid;
};

// ── Bone connections (BoneIndex enum) ───────────────────────────────
static const int BONE_CONNECTIONS[][2] = {
    {BoneIndex::HEAD, BoneIndex::NECK},
    {BoneIndex::NECK, BoneIndex::SPINE_0},
    {BoneIndex::SPINE_0, BoneIndex::SPINE_1},
    {BoneIndex::SPINE_1, BoneIndex::SPINE_2},
    {BoneIndex::SPINE_2, BoneIndex::PELVIS},
    {BoneIndex::SPINE_0, BoneIndex::CLAVICLE_L},
    {BoneIndex::CLAVICLE_L, BoneIndex::ARM_UPPER_L},
    {BoneIndex::ARM_UPPER_L, BoneIndex::ARM_LOWER_L},
    {BoneIndex::ARM_LOWER_L, BoneIndex::HAND_L},
    {BoneIndex::SPINE_0, BoneIndex::CLAVICLE_R},
    {BoneIndex::CLAVICLE_R, BoneIndex::ARM_UPPER_R},
    {BoneIndex::ARM_UPPER_R, BoneIndex::ARM_LOWER_R},
    {BoneIndex::ARM_LOWER_R, BoneIndex::HAND_R},
    {BoneIndex::PELVIS, BoneIndex::LEG_UPPER_L},
    {BoneIndex::LEG_UPPER_L, BoneIndex::LEG_LOWER_L},
    {BoneIndex::LEG_LOWER_L, BoneIndex::ANKLE_L},
    {BoneIndex::PELVIS, BoneIndex::LEG_UPPER_R},
    {BoneIndex::LEG_UPPER_R, BoneIndex::LEG_LOWER_R},
    {BoneIndex::LEG_LOWER_R, BoneIndex::ANKLE_R},
};
static constexpr int NUM_BONE_CONNS = sizeof(BONE_CONNECTIONS) / sizeof(BONE_CONNECTIONS[0]);

// ── Bone world position via Matrix3x4 ───────────────────────────────
static Vector3 get_bone_pos(uintptr_t pawn, int bone_idx) {
    using namespace cs2::offsets;
    uintptr_t scene = read<uintptr_t>(pawn + NetVars::m_pGameSceneNode);
    if (!scene) return {};

    // m_modelState is embedded in CSkeletonInstance (not a pointer)
    uintptr_t model_state = scene + NetVars::m_modelState;
    uintptr_t bone_array = read<uintptr_t>(model_state + NetVars::m_pBones);
    if (!bone_array) return {};

    return read<Matrix3x4>(bone_array + bone_idx * 0x20).get_position();
}

static bool get_bone_array_ptr(uintptr_t pawn, uintptr_t& out) {
    using namespace cs2::offsets;
    uintptr_t scene = read<uintptr_t>(pawn + NetVars::m_pGameSceneNode);
    if (!scene) return false;

    uintptr_t model_state = scene + NetVars::m_modelState;
    out = read<uintptr_t>(model_state + NetVars::m_pBones);
    return out != 0;
}

// ── Weapon name via item definition index ───────────────────────────
static const char* weapon_id_to_name(uint16_t id) {
    switch (id) {
    case 1:  return "DEAGLE";
    case 2:  return "ELITE";
    case 3:  return "FIVESEVEN";
    case 4:  return "GLOCK";
    case 7:  return "AK47";
    case 8:  return "AUG";
    case 9:  return "AWP";
    case 10: return "FAMAS";
    case 11: return "G3SG1";
    case 13: return "GALIL";
    case 14: return "M249";
    case 16: return "M4A4";
    case 17: return "MAC10";
    case 19: return "P90";
    case 23: return "MP5SD";
    case 24: return "UMP45";
    case 25: return "XM1014";
    case 26: return "BIZON";
    case 27: return "MAG7";
    case 28: return "NEGEV";
    case 29: return "SAWEDOFF";
    case 30: return "TEC9";
    case 31: return "ZEUS";
    case 32: return "P2000";
    case 33: return "MP7";
    case 34: return "MP9";
    case 35: return "NOVA";
    case 36: return "P250";
    case 38: return "SCAR20";
    case 39: return "SG556";
    case 40: return "SSG08";
    case 60: return "M4A1S";
    case 61: return "USPS";
    case 63: return "CZ75";
    case 64: return "REVOLVER";
    default: return "";
    }
}

static std::string get_weapon_name(uintptr_t pawn) {
    using namespace cs2::offsets;
    uintptr_t svc = read<uintptr_t>(pawn + NetVars::m_pWeaponServices);
    if (!svc) return "";
    uint32_t h = read<uint32_t>(svc + NetVars::m_hActiveWeapon);
    if (!h) return "";
    uint32_t idx = h & 0x7FFF;
    if (!idx) return "";

    uintptr_t list = read<uintptr_t>(g_offsets.dwEntityList);
    if (!list) return "";
    uintptr_t entry = read<uintptr_t>(list + 8 * (idx >> 9) + 16);
    if (!entry) return "";
    uintptr_t weapon = read<uintptr_t>(entry + 112 * (idx & 0x1FF));
    if (!weapon) return "";

    uint16_t def = read<uint16_t>(weapon + 0x1180 + 0x50 + 0x1BA);
    return weapon_id_to_name(def);
}

// ═════════════════════════════════════════════════════════════════════
//  ESP main entry (FIXED VERSION)
// ═════════════════════════════════════════════════════════════════════
void run(const ESPConfig& cfg) {
    using namespace cs2::renderer;

    debug_log("ESP run: enabled=%d, show_box=%d, team_check=%d, max_dist=%.1f",
        cfg.enabled, cfg.show_box, cfg.team_check, cfg.max_distance);

    using namespace cs2::offsets;
    if (!cfg.enabled || !overlay::is_ready()) return;

    // ── Local player ────────────────────────────────────────────────
    uintptr_t local_ctrl = read<uintptr_t>(g_offsets.dwLocalPlayerController);
    if (!local_ctrl) { debug_log("ESP: no local controller"); return; }

    uint32_t local_handle = read<uint32_t>(local_ctrl + NetVars::m_hPawn);
    if (!local_handle) { debug_log("ESP: no local pawn handle"); return; }

    uintptr_t local_pawn = get_entity_from_handle(local_handle);
    if (!local_pawn) { debug_log("ESP: no local pawn"); return; }

    uint8_t local_team = read<uint8_t>(local_pawn + NetVars::m_iTeamNum);
    ViewMatrix vm = read<ViewMatrix>(g_offsets.dwViewMatrix);
    Vector3 local_origin = read<Vector3>(local_pawn + NetVars::m_vOldOrigin);
    int sw = overlay::get_width();
    int sh = overlay::get_height();

    // ── Entity loop ────────────────────────────────────────────────
    std::vector<ESPEntity> entities;

    for (int i = 1; i < 64; ++i) {
        uintptr_t controller = get_entity_from_index(i);
        if (!controller || controller == local_ctrl) continue;

        uint32_t pawn_handle = read<uint32_t>(controller + NetVars::m_hPawn);
        if (!pawn_handle) continue;

        uintptr_t pawn = get_entity_from_handle(pawn_handle);
        if (!pawn || pawn == local_pawn) continue;

        int32_t health = read<int32_t>(pawn + NetVars::m_iHealth);
        uint8_t life   = read<uint8_t>(pawn + NetVars::m_lifeState);
        uint8_t team   = read<uint8_t>(pawn + NetVars::m_iTeamNum);
        Vector3 origin = read<Vector3>(pawn + NetVars::m_vOldOrigin);

        if (health <= 0 || health > 200) continue;
        if (life != 0) continue;
        if (cfg.team_check && team == local_team) continue;

        uintptr_t sn = read<uintptr_t>(pawn + NetVars::m_pGameSceneNode);
        if (!sn) continue;

        debug_log("  Entity[%d] OK: team=%d health=%d", i, team, health);

        Vector2 foot, head2d;

        // 脚底（稳定）
        if (!world_to_screen(origin, foot, vm, sw, sh))
            continue;

        // 头部直接用 origin + 75u 模拟（CS2 100% 稳定）
        Vector3 head3d = origin;
        head3d.z += 75.0f;

        if (!world_to_screen(head3d, head2d, vm, sw, sh))
            continue;

        float dist = local_origin.dist_to(origin);

        ESPEntity ent;
        ent.pawn          = pawn;
        ent.origin        = origin;
        ent.head_pos      = head3d;
        ent.screen_origin = foot;
        ent.screen_head   = head2d;
        ent.health        = health;
        ent.team          = team;
        ent.distance      = dist;
        ent.valid         = true;

        ent.name = read_string(controller + NetVars::m_iszPlayerName, 32);
        ent.weapon_name = cfg.show_weapon ? get_weapon_name(pawn) : "";

        ent.bones_valid = false;
        if (cfg.show_skeleton) {
            uintptr_t ba = 0;
            if (get_bone_array_ptr(pawn, ba)) {
                ent.bones_valid = true;
                for (int b = 0; b < BoneIndex::MAX_BONES; ++b) {
                    auto mat = read<Matrix3x4>(ba + b * 0x20);
                    Vector3 bp = mat.get_position();
                    if (bp.length() < 0.001f) continue;
                    Vector2 sp;
                    if (world_to_screen(bp, sp, vm, sw, sh)) {
                        ent.bone_positions[b * 2 + 0] = (int)sp.x;
                        ent.bone_positions[b * 2 + 1] = (int)sp.y;
                    }
                }
            }
        }

        entities.push_back(ent);
    }

    std::sort(entities.begin(), entities.end(),
        [](auto& a, auto& b) { return a.distance > b.distance; });

    // ── Draw ────────────────────────────────────────────────────────
    for (auto& ent : entities) {
        auto& foot = ent.screen_origin;
        auto& head = ent.screen_head;

        float h = foot.y - head.y;
        float w = std::max(h * 0.5f, 1.0f);
        float x = head.x - w * 0.5f;

        Color col = ent.team == local_team ? cfg.team_color : cfg.enemy_color;
        col.a *= cfg.global_alpha;
        Color shadow = cfg.shadow_color;
        shadow.a *= cfg.shadow_alpha * cfg.global_alpha;

        if (cfg.show_box) {
            switch (cfg.box_type) {
            case 0:
                if (cfg.filled)
                    draw_filled_rect(x, head.y, w, h, Color(col.r, col.g, col.b, 0.15f * cfg.global_alpha));
                draw_rect(x - 1, head.y - 1, w + 2, h + 2, shadow);
                draw_rect(x, head.y, w, h, col);
                break;
            case 1:
                draw_corner_box(x, head.y, w, h, col);
                break;
            case 2:
                draw_rect(x - 1, head.y - 1, w + 2, h + 2, shadow);
                draw_rect(x + 1, head.y + 1, w - 2, h - 2, shadow);
                draw_rect(x, head.y, w, h, col);
                break;
            }
        }

        if (cfg.show_line)
            draw_line((float)sw * 0.5f, (float)sh, foot.x, foot.y, col, 1.5f);

        if (cfg.show_health)
            draw_health_bar(x, head.y, 4, h, ent.health);

        if (cfg.show_name && !ent.name.empty()) {
            std::wstring wname(ent.name.begin(), ent.name.end());
            float tw = get_text_width(wname, 0.7f);
            draw_text_shadow(x + w * 0.5f - tw * 0.5f, head.y - 16,
                             wname, Color(1, 1, 1, 0.9f * cfg.global_alpha), 0.7f);
        }

        if (cfg.show_distance) {
            wchar_t buf[32];
            swprintf(buf, 32, L"%.0fm", ent.distance / 39.37f);
            float tw = get_text_width(buf, 0.6f);
            draw_text_shadow(x + w * 0.5f - tw * 0.5f, foot.y + 4,
                             buf, Color(1, 1, 1, 0.8f * cfg.global_alpha), 0.6f);
        }

        if (cfg.show_weapon && !ent.weapon_name.empty()) {
            std::wstring wname(ent.weapon_name.begin(), ent.weapon_name.end());
            float tw = get_text_width(wname, 0.6f);
            draw_text_shadow(x + w * 0.5f - tw * 0.5f, foot.y + 16,
                             wname, Color(1, 1, 1, 0.8f * cfg.global_alpha), 0.6f);
        }

        if (cfg.show_skeleton && ent.bones_valid) {
            for (int bc = 0; bc < NUM_BONE_CONNS; ++bc) {
                int b0 = BONE_CONNECTIONS[bc][0];
                int b1 = BONE_CONNECTIONS[bc][1];
                if (b0 >= BoneIndex::MAX_BONES || b1 >= BoneIndex::MAX_BONES) continue;
                draw_line((float)ent.bone_positions[b0 * 2], (float)ent.bone_positions[b0 * 2 + 1],
                          (float)ent.bone_positions[b1 * 2], (float)ent.bone_positions[b1 * 2 + 1],
                          Color(1, 1, 1, 0.6f * cfg.global_alpha), 1.0f);
            }
        }
    }
}

} // namespace cs2::esp
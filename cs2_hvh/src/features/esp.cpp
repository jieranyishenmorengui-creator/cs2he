#include "esp.h"
#include "../core/memory.h"
#include "../core/offsets.h"
#include "../core/overlay.h"
#include "../core/renderer.h"
#include "../utils/debug_log.h"
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
#include <unordered_map>
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

// ── Weapon name via item definition index ───────────────────────────
static const char* weapon_id_to_name(uint16_t id) {
    switch (id) {
    case 1:  return "DEAGLE";   case 2:  return "ELITE";
    case 3:  return "FIVESEVEN"; case 4:  return "GLOCK";
    case 7:  return "AK47";     case 8:  return "AUG";
    case 9:  return "AWP";      case 10: return "FAMAS";
    case 11: return "G3SG1";    case 13: return "GALIL";
    case 14: return "M249";     case 16: return "M4A4";
    case 17: return "MAC10";    case 19: return "P90";
    case 23: return "MP5SD";    case 24: return "UMP45";
    case 25: return "XM1014";   case 26: return "BIZON";
    case 27: return "MAG7";     case 28: return "NEGEV";
    case 29: return "SAWEDOFF"; case 30: return "TEC9";
    case 31: return "ZEUS";     case 32: return "P2000";
    case 33: return "MP7";      case 34: return "MP9";
    case 35: return "NOVA";     case 36: return "P250";
    case 38: return "SCAR20";   case 39: return "SG556";
    case 40: return "SSG08";    case 60: return "M4A1S";
    case 61: return "USPS";     case 63: return "CZ75";
    case 64: return "REVOLVER"; default: return "";
    }
}

// Resolve weapon name using a cached entity-list base (avoids re-reading dwEntityList)
static std::string get_weapon_name_cached(uintptr_t pawn, uintptr_t entListBase) {
    using namespace cs2::offsets;
    uintptr_t svc = read<uintptr_t>(pawn + NetVars::m_pWeaponServices);
    if (!svc) return "";
    uint32_t h = read<uint32_t>(svc + NetVars::m_hActiveWeapon);
    if (!h) return "";
    uint32_t idx = h & 0x7FFF;
    if (!idx || !entListBase) return "";

    uintptr_t entry = read<uintptr_t>(entListBase + 8 * (idx >> 9) + 16);
    if (!entry) return "";
    uintptr_t weapon = read<uintptr_t>(entry + 112 * (idx & 0x1FF));
    if (!weapon) return "";

    uint16_t def = read<uint16_t>(weapon + 0x1180 + 0x50 + 0x1BA);
    return weapon_id_to_name(def);
}

// ── Per-entity raw data (no screen coords yet) ────────────────
struct RawEntity {
    uintptr_t pawn;
    Vector3   origin;
    Vector3   headPos;
    int       health;
    int       team;
    float     distance;
    std::string name;
    std::string weapon_name;

    // Cached head bone (used if skeleton is off)
    bool      headFromBone;

    // Skeleton: world-space bone positions (extracted in phase 1)
    int       boneCount;
    Vector3   boneWorld[BoneIndex::MAX_BONES];
};

// ═════════════════════════════════════════════════════════════════════
//  ESP main entry (TWO-PHASE: collect → W2S)
// ═════════════════════════════════════════════════════════════════════
void run(const ESPConfig& cfg) {
    using namespace cs2::renderer;
    if (!cfg.enabled || !overlay::is_ready()) return;
    using namespace cs2::offsets;

    int sw = overlay::get_width();
    int sh = overlay::get_height();

    // ── Cache entity-list base ──────────────────────────────────
    uintptr_t entListBase = read<uintptr_t>(g_offsets.dwEntityList);
    if (!entListBase) return;

    // ── Local player ────────────────────────────────────────────
    uintptr_t local_ctrl = read<uintptr_t>(g_offsets.dwLocalPlayerController);
    if (!local_ctrl) return;

    uint32_t local_handle = read<uint32_t>(local_ctrl + NetVars::m_hPawn);
    if (!local_handle) return;

    uintptr_t local_pawn = get_entity_from_handle(local_handle);
    if (!local_pawn) return;

    uint8_t local_team = read<uint8_t>(local_pawn + NetVars::m_iTeamNum);
    Vector3 local_origin = read<Vector3>(local_pawn + NetVars::m_vOldOrigin);

    // ═════════════════════════════════════════════════════════════
    //  PHASE 1: Collect raw world-space data (NO W2S)
    // ═════════════════════════════════════════════════════════════
    std::vector<RawEntity> rawList;

    for (int i = 1; i < 64; ++i) {
        uintptr_t chunkPtr = read<uintptr_t>(entListBase + 8 * (i >> 9) + 0x10);
        if (!chunkPtr) continue;
        uintptr_t controller = read<uintptr_t>(chunkPtr + 112 * (i & 0x1FF));
        if (!controller || controller == local_ctrl) continue;

        // Batch-read controller: pawnHandle + playerName
        uint8_t ctrlBuf[0x60];
        if (!read(controller + 0x6BC, ctrlBuf, 0x58)) continue;
        uint32_t pawn_handle = *(uint32_t*)(ctrlBuf + 0x00);
        if (!pawn_handle) continue;

        uintptr_t pawn = get_entity_from_handle(pawn_handle);
        if (!pawn || pawn == local_pawn) continue;

        // Batch-read pawn core (0x330 ~ 0x400)
        uint8_t pawnCore[0xD0];
        if (!read(pawn + 0x330, pawnCore, 0xD0)) continue;
        uintptr_t sceneNode = *(uintptr_t*)(pawnCore + 0x00);
        int32_t  health     = *(int32_t*)(pawnCore + 0x1C);
        uint8_t  life       = *(uint8_t*)(pawnCore + 0x24);
        uint8_t  team       = *(uint8_t*)(pawnCore + 0xBB);
        if (health <= 0 || health > 200) continue;
        if (life != 0) continue;
        if (cfg.team_check && team == local_team) continue;

        // Origin
        Vector3 origin;
        if (!read(pawn + NetVars::m_vOldOrigin, &origin, 12)) continue;

        // Head: actual bone position (world-space only)
        Vector3 headPos(origin.x, origin.y, origin.z + 72.0f); // fallback
        bool headFromBone = false;
        if (sceneNode) {
            uintptr_t model_state = sceneNode + NetVars::m_modelState;
            uintptr_t boneArray = read<uintptr_t>(model_state + NetVars::m_pBones);
            if (boneArray) {
                Matrix3x4 headMat = read<Matrix3x4>(boneArray + BoneIndex::HEAD * 0x20);
                Vector3 bp = headMat.get_position();
                if (bp.length() > 1.0f) { headPos = bp; headFromBone = true; }
            }
        }

        // Distance
        float dist = local_origin.dist_to(origin);

        // Name
        std::string entName;
        {
            const char* ns = (const char*)(ctrlBuf + 0x34);
            size_t nl = strnlen(ns, 32);
            if (nl > 0) entName.assign(ns, nl);
        }

        // Weapon
        std::string weaponName;
        if (cfg.show_weapon)
            weaponName = get_weapon_name_cached(pawn, entListBase);

        // ── Skeleton: read all bone matrices in bulk ───────────
        int boneCount = 0;
        Vector3 boneWorld[BoneIndex::MAX_BONES]{};

        if (cfg.show_skeleton && sceneNode) {
            uintptr_t ms = sceneNode + NetVars::m_modelState;
            uintptr_t ba = read<uintptr_t>(ms + NetVars::m_pBones);
            if (ba) {
                Matrix3x4 allBones[BoneIndex::MAX_BONES];
                if (read(ba, allBones, sizeof(allBones))) {
                    boneCount = BoneIndex::MAX_BONES;
                    for (int b = 0; b < BoneIndex::MAX_BONES; ++b)
                        boneWorld[b] = allBones[b].get_position();
                }
            }
        }

        rawList.push_back(RawEntity{
            pawn, origin, headPos, health, team, dist,
            std::move(entName), std::move(weaponName),
            headFromBone, boneCount, {}
        });
        if (boneCount > 0)
            memcpy(rawList.back().boneWorld, boneWorld, sizeof(Vector3) * boneCount);
    }

    // ═════════════════════════════════════════════════════════════
    //  PHASE 2: Read ViewMatrix NOW (freshest possible), then W2S
    // ═════════════════════════════════════════════════════════════
    ViewMatrix vm = read<ViewMatrix>(g_offsets.dwViewMatrix);

    // Smoothing state (persists across frames)
    static std::unordered_map<uintptr_t, Vector2> s_smoothFoot;
    static std::unordered_map<uintptr_t, Vector2> s_smoothHead;
    float alpha = 1.0f - std::clamp(cfg.smooth_factor, 0.0f, 0.95f);

    std::vector<ESPEntity> entities;

    for (auto& raw : rawList) {
        Vector2 foot, head2d;
        if (!world_to_screen(raw.origin, foot, vm, sw, sh)) continue;
        if (!world_to_screen(raw.headPos, head2d, vm, sw, sh)) continue;

        // ── Smoothing (EMA) ──────────────────────────────────
        if (cfg.smooth_factor > 0.0f) {
            auto itF = s_smoothFoot.find(raw.pawn);
            if (itF != s_smoothFoot.end()) {
                foot.x = itF->second.x + (foot.x - itF->second.x) * alpha;
                foot.y = itF->second.y + (foot.y - itF->second.y) * alpha;
            }
            s_smoothFoot[raw.pawn] = foot;

            auto itH = s_smoothHead.find(raw.pawn);
            if (itH != s_smoothHead.end()) {
                head2d.x = itH->second.x + (head2d.x - itH->second.x) * alpha;
                head2d.y = itH->second.y + (head2d.y - itH->second.y) * alpha;
            }
            s_smoothHead[raw.pawn] = head2d;
        }

        ESPEntity ent;
        ent.pawn          = raw.pawn;
        ent.origin        = raw.origin;
        ent.head_pos      = raw.headPos;
        ent.screen_origin = foot;
        ent.screen_head   = head2d;
        ent.health        = raw.health;
        ent.team          = raw.team;
        ent.distance      = raw.distance;
        ent.valid         = true;
        ent.name          = std::move(raw.name);
        ent.weapon_name   = std::move(raw.weapon_name);

        // W2S skeleton bones
        ent.bones_valid = false;
        if (raw.boneCount > 0) {
            ent.bones_valid = true;
            for (int b = 0; b < raw.boneCount; ++b) {
                Vector3& bp = raw.boneWorld[b];
                if (bp.length() < 0.001f) continue;
                Vector2 sp;
                if (world_to_screen(bp, sp, vm, sw, sh)) {
                    ent.bone_positions[b * 2 + 0] = (int)sp.x;
                    ent.bone_positions[b * 2 + 1] = (int)sp.y;
                }
            }
        }

        entities.push_back(ent);
    }

    // ── Clean up smoothing state for dead entities ────────────
    if (cfg.smooth_factor > 0.0f) {
        auto prune = [&](auto& map) {
            for (auto it = map.begin(); it != map.end(); ) {
                bool alive = false;
                for (auto& e : entities)
                    if (e.pawn == it->first) { alive = true; break; }
                if (alive) ++it;
                else       it = map.erase(it);
            }
        };
        prune(s_smoothFoot);
        prune(s_smoothHead);
    }

    std::sort(entities.begin(), entities.end(),
        [](auto& a, auto& b) { return a.distance > b.distance; });

    // ═════════════════════════════════════════════════════════════
    //  DRAW
    // ═════════════════════════════════════════════════════════════
    for (auto& ent : entities) {
        auto& foot = ent.screen_origin;
        auto& head = ent.screen_head;

        float h  = foot.y - head.y;
        float w  = std::max(h * 0.5f, 1.0f);
        float x  = head.x - w * 0.5f;

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
#include "esp.h"
#include "../core/memory.h"
#include "../core/offsets.h"
#include "../core/overlay.h"
#include "../core/renderer.h"
#include <string>
#include <vector>

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
    int bone_positions[32 * 2]; // screen x,y for each bone
    bool bones_valid;
};

static const int BONE_CONNECTIONS[][2] = {
    {6, 5}, {5, 4}, {4, 1}, {1, 0}, // head -> spine -> pelvis -> root
    {6, 7}, {7, 8}, {8, 9}, // right arm
    {6, 10}, {10, 11}, {11, 12}, // left arm
    {1, 13}, {13, 14}, {14, 15}, // right leg
    {1, 16}, {16, 17}, {17, 18}, // left leg
    {5, 19}, {5, 20}, // neck connections
};
static constexpr int NUM_BONE_CONNECTIONS = sizeof(BONE_CONNECTIONS) / sizeof(BONE_CONNECTIONS[0]);

static Vector3 get_bone_position(uintptr_t pawn, int bone_idx) {
    using namespace cs2::offsets;

    uintptr_t scene_node = read<uintptr_t>(pawn + NetVars::m_pGameSceneNode);
    if (!IsRemotePtrValid(scene_node)) return {};

    uintptr_t bone_array = read<uintptr_t>(scene_node + NetVars::m_modelState);
    if (!IsRemotePtrValid(bone_array)) return {};

    uintptr_t bone_addr = bone_array + bone_idx * 32;
    return read<Vector3>(bone_addr);
}

static std::string get_weapon_name(uintptr_t pawn) {
    using namespace cs2::offsets;

    uintptr_t weapon_services = read<uintptr_t>(pawn + NetVars::m_pWeaponServices);
    if (!IsRemotePtrValid(weapon_services)) return "";

    uint32_t weapon_handle = read<uint32_t>(weapon_services + NetVars::m_hActiveWeapon);
    if (!weapon_handle) return "";

    uint32_t list_entry = read<uint32_t>(g_offsets.dwEntityList + 8 * ((weapon_handle & 0x7FFF) >> 9) + 0x10);
    if (!IsRemotePtrValid(list_entry)) return "";
    uintptr_t weapon = list_entry + 120 * (weapon_handle & 0x1FF);
    if (!IsRemotePtrValid(weapon)) return "";

    // Read weapon designer name string
    char buf[256] = {};
    for (int i = 0; i < 255; ++i) {
        if (!read(weapon + 0x40 + i, &buf[i], 1) || buf[i] == '\0') break;
    }
    if (buf[0] == 'w' && buf[1] == 'e' && buf[2] == 'a' && buf[3] == 'p') {
        std::string s(buf + 7);
        for (auto& c : s) c = toupper(c);
        return s;
    }
    return "";
}

void run(const ESPConfig& cfg) {
    using namespace cs2::offsets;
    using namespace cs2::renderer;

    if (!cfg.enabled) return;
    if (!overlay::is_ready()) return;

    // ── Get local player ──────────────────────────────────────────
    uintptr_t local_controller = safe_read<uintptr_t>(g_offsets.dwLocalPlayerController);
    if (!IsRemotePtrValid(local_controller)) return;

    uint32_t local_pawn_handle = read<uint32_t>(local_controller + NetVars::m_hPlayerPawn);
    if (!local_pawn_handle) return;

    uintptr_t local_pawn = get_entity_from_handle(local_pawn_handle);
    if (!IsRemotePtrValid(local_pawn)) return;

    uint8_t local_team = read<uint8_t>(local_pawn + NetVars::m_iTeamNum);
    ViewMatrix view_matrix = read<ViewMatrix>(g_offsets.dwViewMatrix);

    int sw = overlay::get_width();
    int sh = overlay::get_height();

    std::vector<ESPEntity> entities;

    for (int i = 1; i < 64; ++i) {
        uintptr_t pawn = get_entity_from_index(i);
        if (!IsRemotePtrValid(pawn)) continue;
        if (pawn == local_pawn) continue;

        // Health / alive check
        int health = read<int32_t>(pawn + NetVars::m_iHealth);
        if (health <= 0 || health > 100) continue;

        uint8_t life_state = read<uint8_t>(pawn + NetVars::m_lifeState);
        if (life_state != 0) continue;

        uint8_t team = read<uint8_t>(pawn + NetVars::m_iTeamNum);
        if (cfg.team_check && team == local_team) continue;

        bool dormant = read<bool>(pawn + NetVars::m_bDormant);
        if (dormant) continue;

        // Position
        Vector3 origin = read<Vector3>(pawn + NetVars::m_vecOrigin);
        Vector3 head_pos = get_bone_position(pawn, 6);

        float distance = origin.dist_to(read<Vector3>(local_pawn + NetVars::m_vecOrigin));
        if (cfg.max_distance > 0.0f && distance > cfg.max_distance) continue;

        Vector2 screen_origin, screen_head;
        if (!world_to_screen(origin, screen_origin, view_matrix, sw, sh)) continue;
        if (!world_to_screen(head_pos, screen_head, view_matrix, sw, sh)) continue;

        // Build entity
        ESPEntity ent;
        ent.pawn = pawn;
        ent.origin = origin;
        ent.head_pos = head_pos;
        ent.screen_origin = screen_origin;
        ent.screen_head = screen_head;
        ent.health = health;
        ent.team = team;
        ent.distance = distance;
        ent.valid = true;

        // Player name from controller handle
        {
            uint32_t ctrl_handle = read<uint32_t>(pawn + NetVars::m_hController);
            if (ctrl_handle & 0x7FFF) {
                uintptr_t controller = get_entity_from_handle(ctrl_handle);
                if (IsRemotePtrValid(controller))
                    ent.name = read_string(controller + NetVars::m_iszPlayerName, 32);
            }
        }

        ent.weapon_name = cfg.show_weapon ? get_weapon_name(pawn) : "";

        // Bones
        ent.bones_valid = false;
        if (cfg.show_skeleton) {
            ent.bones_valid = true;
            for (int b = 0; b < 21; ++b) {
                Vector3 bp = get_bone_position(pawn, b);
                Vector2 sp;
                if (world_to_screen(bp, sp, view_matrix, sw, sh)) {
                    ent.bone_positions[b * 2 + 0] = (int)sp.x;
                    ent.bone_positions[b * 2 + 1] = (int)sp.y;
                }
            }
        }

        entities.push_back(ent);
    }

    // ── Draw ──────────────────────────────────────────────────────
    for (auto& ent : entities) {
        Vector2& foot = ent.screen_origin;
        Vector2& head = ent.screen_head;

        float h = foot.y - head.y;
        float w = h * 0.5f;
        if (w < 1.0f) w = 1.0f;
        float x = head.x - w / 2.0f;

        Color col = ent.team == local_team ? cfg.team_color : cfg.enemy_color;
        col.a *= cfg.global_alpha;

        Color shadow = cfg.shadow_color;
        shadow.a *= cfg.shadow_alpha * cfg.global_alpha;

        // Box
        if (cfg.show_box) {
            switch (cfg.box_type) {
            case 0: // Rect
                if (cfg.filled) {
                    draw_filled_rect(x, head.y, w, h, Color(col.r, col.g, col.b, 0.15f * cfg.global_alpha));
                }
                draw_rect(x - 1, head.y - 1, w + 2, h + 2, shadow);
                draw_rect(x, head.y, w, h, col);
                break;
            case 1: // Corner
                draw_corner_box(x, head.y, w, h, col, 1.5f, 8.0f);
                break;
            case 2: // 3D rect (with shadow)
                draw_rect(x - 1, head.y - 1, w + 2, h + 2, shadow);
                draw_rect(x + 1, head.y + 1, w - 2, h - 2, shadow);
                draw_rect(x, head.y, w, h, col);
                break;
            }
        }

        // Line to bottom center
        if (cfg.show_line) {
            draw_line((float)sw / 2.0f, (float)sh, foot.x, foot.y, col, 1.5f);
        }

        // Skeleton
        if (cfg.show_skeleton && ent.bones_valid) {
            for (int bc = 0; bc < NUM_BONE_CONNECTIONS; ++bc) {
                int b0 = BONE_CONNECTIONS[bc][0];
                int b1 = BONE_CONNECTIONS[bc][1];
                if (b0 < 21 && b1 < 21) {
                    draw_line(
                        (float)ent.bone_positions[b0 * 2], (float)ent.bone_positions[b0 * 2 + 1],
                        (float)ent.bone_positions[b1 * 2], (float)ent.bone_positions[b1 * 2 + 1],
                        Color(1, 1, 1, 0.6f * cfg.global_alpha), 1.0f
                    );
                }
            }
        }

        // Health bar
        if (cfg.show_health) {
            draw_health_bar(x, head.y, 4, h, ent.health);
        }

        // Name
        if (cfg.show_name && !ent.name.empty()) {
            std::wstring wname(ent.name.begin(), ent.name.end());
            float tw = get_text_width(wname, 0.7f);
            draw_text_shadow(x + w / 2.0f - tw / 2.0f, head.y - 16, wname, Color(1, 1, 1, 0.9f * cfg.global_alpha), 0.7f);
        }

        // Distance
        if (cfg.show_distance) {
            wchar_t buf[32];
            swprintf(buf, 32, L"%.0fm", ent.distance / 39.37f);
            draw_text_shadow(x + w / 2.0f - get_text_width(buf, 0.6f) / 2.0f,
                            foot.y + 4, buf, Color(1, 1, 1, 0.8f * cfg.global_alpha), 0.6f);
        }

        // Weapon
        if (cfg.show_weapon && !ent.weapon_name.empty()) {
            std::wstring wname(ent.weapon_name.begin(), ent.weapon_name.end());
            draw_text_shadow(x + w / 2.0f - get_text_width(wname, 0.6f) / 2.0f,
                            foot.y + 16, wname, Color(1, 1, 1, 0.8f * cfg.global_alpha), 0.6f);
        }
    }
}

} // namespace cs2::esp

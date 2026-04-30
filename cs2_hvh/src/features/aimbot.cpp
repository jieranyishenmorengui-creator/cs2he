#include "aimbot.h"
#include "../core/memory.h"
#include "../core/offsets.h"
#include "../core/overlay.h"
#include <cmath>

namespace cs2::aimbot {

struct EntityInfo {
    uintptr_t pawn;
    Vector3 head_pos;
    Vector3 origin;
    float distance;
    float fov;
    int health;
    int team;
    bool dormants;
};

static float last_toggle_state = false;

static Vector3 get_bone_pos(uintptr_t pawn, int bone_idx) {
    using namespace ::cs2::memory;
    using namespace ::cs2::offsets;

    uintptr_t scene_node = read<uintptr_t>(pawn + NetVars::m_pGameSceneNode);
    if (!IsRemotePtrValid(scene_node)) return {};

    // CModelState embedded at scene_node + m_modelState, m_pBones at +0x80
    uintptr_t model_state_addr = scene_node + NetVars::m_modelState;
    uintptr_t bone_array = read<uintptr_t>(model_state_addr + NetVars::m_pBones);
    if (!IsRemotePtrValid(bone_array)) return {};

    // Each bone is a Matrix3x4 (48 bytes), not a flat Vector3
    Matrix3x4 bone_mat = read<Matrix3x4>(bone_array + bone_idx * sizeof(Matrix3x4));
    return bone_mat.get_position();
}

static Vector3 get_view_angles(uintptr_t player_controller) {
    using namespace ::cs2::memory;
    using namespace ::cs2::offsets;
    uintptr_t pawn_handle = read<uint32_t>(player_controller + NetVars::m_hPawn);
    if (!pawn_handle) return {};

    uintptr_t pawn = get_entity_from_handle(pawn_handle);
    if (!pawn) return {};

    return read<Vector3>(pawn + NetVars::m_angEyeAngles);
}

void run(const AimbotConfig& cfg) {
    using namespace ::cs2::memory;
    using namespace ::cs2::offsets;

    if (!cfg.enabled) return;

    // Check key mode
    bool key_down = false;
    if (cfg.key_mode == 2) {
        key_down = true; // always
    } else if (cfg.key_mode == 0) {
        // hold
        bool key0 = overlay::is_key_down(cfg.key0);
        bool key1 = cfg.key1 ? overlay::is_key_down(cfg.key1) : false;
        key_down = key0 || key1;
    } else if (cfg.key_mode == 1) {
        // toggle
        static bool toggled = false;
        if (overlay::was_key_pressed(cfg.key0) || (cfg.key1 && overlay::was_key_pressed(cfg.key1)))
            toggled = !toggled;
        key_down = toggled;
    }

    if (!key_down) return;

    // Get local player
    uintptr_t local_controller = read<uintptr_t>(g_offsets.dwLocalPlayerController);
    if (!IsRemotePtrValid(local_controller)) return;
    uint32_t local_pawn_handle = read<uint32_t>(local_controller + NetVars::m_hPawn);
    if (!local_pawn_handle) return;
    uintptr_t local_pawn = get_entity_from_handle(local_pawn_handle);
    if (!IsRemotePtrValid(local_pawn)) return;

    uint8_t local_team = read<uint8_t>(local_pawn + NetVars::m_iTeamNum);
    Vector3 local_origin = read<Vector3>(local_pawn + NetVars::m_vOldOrigin);
    Vector3 local_view_offset = read<Vector3>(local_pawn + NetVars::m_vecViewOffset);
    Vector3 local_eye_pos = local_origin + local_view_offset;
    Vector3 view_angles = read<Vector3>(local_pawn + NetVars::m_angEyeAngles);

    // Read view matrix
    ViewMatrix view_matrix = read<ViewMatrix>(g_offsets.dwViewMatrix);

    // Enumerate entities
    std::vector<EntityInfo> targets;

    for (int i = 1; i < 64; ++i) {
        // Get controller → resolve pawn via handle
        uintptr_t controller = get_entity_from_index(i);
        if (!IsRemotePtrValid(controller)) continue;
        if (controller == local_controller) continue;

        uint32_t pawn_handle = read<uint32_t>(controller + NetVars::m_hPawn);
        if (!pawn_handle) continue;

        uintptr_t pawn = get_entity_from_handle(pawn_handle);
        if (!IsRemotePtrValid(pawn) || pawn == local_pawn) continue;

        int health = read<int32_t>(pawn + NetVars::m_iHealth);
        if (health <= 0 || health > 100) continue;

        uint8_t life_state = read<uint8_t>(pawn + NetVars::m_lifeState);
        if (life_state != 0) continue;

        uint8_t team = read<uint8_t>(pawn + NetVars::m_iTeamNum);
        if (cfg.team_check && team == local_team) continue;

        uintptr_t scene_node_dormant = read<uintptr_t>(pawn + NetVars::m_pGameSceneNode);
        bool dormant = IsRemotePtrValid(scene_node_dormant) ? read<bool>(scene_node_dormant + 0x103) : true;
        if (dormant) continue;

        Vector3 origin = read<Vector3>(pawn + NetVars::m_vOldOrigin);
        Vector3 head_pos = get_bone_pos(pawn, cfg.target_bone);

        float distance = local_eye_pos.dist_to(origin);
        if (cfg.max_distance > 0.0f && distance > cfg.max_distance) continue;

        Vector2 screen_pos;
        if (!world_to_screen(head_pos, screen_pos, view_matrix, overlay::get_width(), overlay::get_height()))
            continue;

        float sw = (float)overlay::get_width();
        float sh = (float)overlay::get_height();
        Vector2 screen_center(sw / 2.0f, sh / 2.0f);
        Vector2 delta = screen_pos - screen_center;
        float fov = delta.length();

        if (fov > cfg.fov * (sw / 1920.0f) * 10.0f) continue;

        Vector3 aim_angle = calc_angle(local_eye_pos, head_pos);
        float angle_fov = get_fov(view_angles, aim_angle);

        targets.push_back({ pawn, head_pos, origin, distance, angle_fov, health, team, false });
    }

    if (targets.empty()) return;

    // Sort by mode
    EntityInfo* best = &targets[0];
    for (auto& t : targets) {
        if (cfg.mode == 1) { // closest to crosshair (lowest fov)
            if (t.fov < best->fov) best = &t;
        } else if (cfg.mode == 2) { // closest distance
            if (t.distance < best->distance) best = &t;
        } else { // 3 = fov
            if (t.fov < best->fov) best = &t;
        }
    }

    // Calculate aim angle
    Vector3 aim_angle = calc_angle(local_eye_pos, best->head_pos);
    Vector3 delta_angle = angle_diff(aim_angle, view_angles);

    // Apply smoothing
    if (cfg.smooth > 0.0f) {
        float smooth_factor = 1.0f / cfg.smooth;
        delta_angle.x *= smooth_factor;
        delta_angle.y *= smooth_factor;
    }

    Vector3 new_angle = view_angles + delta_angle;

    // Clamp angles
    while (new_angle.y > 180.0f) new_angle.y -= 360.0f;
    while (new_angle.y < -180.0f) new_angle.y += 360.0f;
    if (new_angle.x > 89.0f) new_angle.x = 89.0f;
    if (new_angle.x < -89.0f) new_angle.x = -89.0f;
    new_angle.z = 0.0f;

    // Write view angles
    write<Vector3>(local_pawn + NetVars::m_angEyeAngles, new_angle);

    // RCS - read current punch and counteract via AimPunchServices
    if (cfg.recoil_control) {
        uintptr_t aimpunch_svc = read<uintptr_t>(local_pawn + NetVars::m_pAimPunchServices);
        if (aimpunch_svc) {
            Vector3 aim_punch = read<Vector3>(aimpunch_svc + NetVars::m_aimPunchAngle);
            if (aim_punch.length() > 0.01f) {
                Vector3 punch_correction = aim_punch * cfg.rcs_scale;
                Vector3 rcs_angle = new_angle - punch_correction;
                write<Vector3>(local_pawn + NetVars::m_angEyeAngles, rcs_angle);
            }
        }
    }
}

} // namespace cs2::aimbot

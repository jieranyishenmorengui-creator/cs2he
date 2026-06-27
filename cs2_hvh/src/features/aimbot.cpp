#include "aimbot.h"
#include "../core/memory.h"
#include "../core/offsets.h"
#include "../core/overlay.h"
#include <cmath>
#include <chrono>
#include <random>

namespace cs2::aimbot {

using namespace ::cs2::memory;
using namespace ::cs2::offsets;

// ═══════════════════════════════════════════════════════════════
//  读骨骼: FutaZone 方式 — 只试 m_modelState+0x80
//  批量读30骨, 位置在每骨entry offset 0 (Vector3, stride 0x20)
// ═══════════════════════════════════════════════════════════════

static Vector3 get_bone_pos(uintptr_t pawn, int bone_idx) {
    uintptr_t sn = read<uintptr_t>(pawn + NetVars::m_pGameSceneNode);
    if (!IsRemotePtrValid(sn)) return {};

    // FutaZone: ReadPointer(gameSceneNode, m_modelState + 0x80)
    uintptr_t ba = read<uintptr_t>(sn + NetVars::m_modelState + 0x80);
    if (!IsRemotePtrValid(ba)) return {};

    // 批量读30骨 × 32字节, 位置在offset 0
    uint8_t raw[30 * 0x20];
    if (!read(ba, raw, sizeof(raw))) return {};

    float* pf = (float*)(raw + bone_idx * 0x20);
    return Vector3(pf[0], pf[1], pf[2]);
}

// calc_angle — 标准公式, 加防NaN
static Vector3 calc_angle_safe(const Vector3& src, const Vector3& dst) {
    Vector3 d = dst - src;
    float len = d.length();
    if (len < 0.01f) return {};
    float clp = std::clamp(d.z / len, -1.f, 1.f);
    return {
        -asinf(clp) * 57.2957795131f,
        atan2f(d.y, d.x) * 57.2957795131f,
        0.f
    };
}

// ═══════════════════════════════════════════════════════════════
//  Aimbot — 写角度 (Write Angles)
// ═══════════════════════════════════════════════════════════════

void run(const AimbotConfig& cfg) {
    if (!cfg.enabled) return;

    // ── 按键 ──────────────────────────────────────────────────
    bool key_down = false;
    if (cfg.key_mode == 2) key_down = true;
    else if (cfg.key_mode == 0)
        key_down = overlay::is_key_down(cfg.key0) || (cfg.key1 && overlay::is_key_down(cfg.key1));
    else if (cfg.key_mode == 1) {
        static bool tog = false;
        if (overlay::was_key_pressed(cfg.key0) || (cfg.key1 && overlay::was_key_pressed(cfg.key1))) tog = !tog;
        key_down = tog;
    }
    if (!key_down) return;

    // ── Local ─────────────────────────────────────────────────
    uintptr_t ctrl = read<uintptr_t>(g_offsets.dwLocalPlayerController);
    if (!IsRemotePtrValid(ctrl)) return;
    uintptr_t lp = get_entity_from_handle(read<uint32_t>(ctrl + NetVars::m_hPawn));
    if (!IsRemotePtrValid(lp)) return;

    uint8_t local_team = read<uint8_t>(lp + NetVars::m_iTeamNum);
    Vector3 eye = read<Vector3>(lp + NetVars::m_vOldOrigin)
                + read<Vector3>(lp + NetVars::m_vecViewOffset);

    ViewMatrix vm = read<ViewMatrix>(g_offsets.dwViewMatrix);
    int sw = overlay::get_width(), sh = overlay::get_height();

    // ── 找最佳目标 ────────────────────────────────────────────
    uintptr_t best = 0;
    float     best_dist = 3.4e38f;

    for (int i = 1; i < 64; ++i) {
        uintptr_t c = get_entity_from_index(i);
        if (!IsRemotePtrValid(c) || c == ctrl) continue;
        uint32_t ph = read<uint32_t>(c + NetVars::m_hPawn);
        if (!ph) continue;
        uintptr_t p = get_entity_from_handle(ph);
        if (!IsRemotePtrValid(p) || p == lp) continue;

        if (read<int32_t>(p + NetVars::m_iHealth) <= 0) continue;
        if (read<uint8_t>(p + NetVars::m_lifeState) != 0) continue;
        if (cfg.team_check && read<uint8_t>(p + NetVars::m_iTeamNum) == local_team) continue;

        uintptr_t sn = read<uintptr_t>(p + NetVars::m_pGameSceneNode);
        if (IsRemotePtrValid(sn) && read<bool>(sn + 0x103)) continue;

        Vector3 origin = read<Vector3>(p + NetVars::m_vOldOrigin);
        if (cfg.max_distance > 0.f && eye.dist_to(origin) > cfg.max_distance) continue;

        // 骨骼位置 / fallback eye pos
        Vector3 bp = get_bone_pos(p, cfg.target_bone);
        if (bp.length() < 0.1f)
            bp = origin + read<Vector3>(p + NetVars::m_vecViewOffset);

        Vector2 sp;
        if (!world_to_screen(bp, sp, vm, sw, sh)) continue;
        float d = (sp - Vector2(sw*0.5f, sh*0.5f)).length();
        if (d > cfg.fov) continue;
        if (d < best_dist) { best = p; best_dist = d; }
    }
    if (!best) return;

    // ── 重新获取目标骨骼 (因为 best 刚确定) ─────────────────
    Vector3 target_origin = read<Vector3>(best + NetVars::m_vOldOrigin);
    Vector3 target_pos = get_bone_pos(best, cfg.target_bone);
    if (target_pos.length() < 0.1f)
        target_pos = target_origin + read<Vector3>(best + NetVars::m_vecViewOffset);

    // ── 移动靶提前量 (velocity 预测) ──────────────────────────
    Vector3 target_vel = read<Vector3>(best + NetVars::m_vecVelocity);
    float speed = target_vel.length();
    if (speed > 10.f) {
        float dist = eye.dist_to(target_pos);
        // CS2步枪弹速 ≈ 30000 u/s, 算出子弹飞行时间
        float bullet_speed = 30000.f;
        float lead_time = std::min(dist / bullet_speed, 0.15f);
        target_pos = target_pos + target_vel * lead_time;
    }

    // ── 计算角度 ──────────────────────────────────────────────
    Vector3 aim = calc_angle_safe(eye, target_pos);
    if (aim.length() < 0.01f) return;

    // ── RCS ───────────────────────────────────────────────────
    if (cfg.recoil_control) {
        uintptr_t as = read<uintptr_t>(lp + NetVars::m_pAimPunchServices);
        if (IsRemotePtrValid(as)) {
            Vector3 punch = read<Vector3>(as + 0x50);
            float pl = punch.length();
            if (pl > 0.01f && pl < 90.f) {
                aim.x -= punch.x * cfg.rcs_scale;
                aim.y -= punch.y * cfg.rcs_scale;
            }
        }
    }

    // ── 当前视角 → 差值 → 平滑 → 写入 ──────────────────────
    Vector3 cur = read<Vector3>(lp + NetVars::m_angEyeAngles);
    Vector3 delta = aim - cur;

    // normalize yaw
    while (delta.y > 180.f) delta.y -= 360.f;
    while (delta.y < -180.f) delta.y += 360.f;

    // 平滑
    if (cfg.smoothness > 0.1f) {
        float f = 1.0f / cfg.smoothness;
        delta.x *= f;
        delta.y *= f;
    }

    Vector3 out = cur + delta;
    // 钳位 pitch
    out.x = std::clamp(out.x, -89.f, 89.f);
    // normalize yaw
    while (out.y > 180.f) out.y -= 360.f;
    while (out.y < -180.f) out.y += 360.f;
    out.z = 0.f;

    // 写入
    if (g_offsets.dwViewAngles) {
        write<Vector3>(g_offsets.dwViewAngles, out);
    }
    uintptr_t ptr = read<uintptr_t>(g_offsets.dwViewAngles);
    if (ptr && ptr != g_offsets.dwViewAngles && IsRemotePtrValid(ptr))
        write<Vector3>(ptr, out);
}

// ═══════════════════════════════════════════════════════════════
//  Triggerbot
// ═══════════════════════════════════════════════════════════════

static constexpr uintptr_t B_ATTACK  = 0x2065A90;
static constexpr uintptr_t B_ATTACK2 = 0x2065B20;
static std::chrono::steady_clock::time_point g_last_shot;

void triggerbot(const TriggerbotConfig& cfg) {
    static bool was_firing = false;
    if (!cfg.enabled) { was_firing = false; return; }
    if (cfg.key && !overlay::is_key_down(cfg.key)) { was_firing = false; return; }

    uintptr_t ctrl = read<uintptr_t>(g_offsets.dwLocalPlayerController);
    if (!IsRemotePtrValid(ctrl)) { was_firing = false; return; }
    uintptr_t lp = get_entity_from_handle(read<uint32_t>(ctrl + NetVars::m_hPawn));
    if (!IsRemotePtrValid(lp)) { was_firing = false; return; }
    uint8_t local_team = read<uint8_t>(lp + NetVars::m_iTeamNum);

    int ent_idx = read<int32_t>(lp + NetVars::m_iIDEntIndex);
    bool valid = false;
    if (ent_idx > 0 && ent_idx <= 63) {
        uintptr_t tc = get_entity_from_index(ent_idx);
        if (IsRemotePtrValid(tc) && tc != ctrl) {
            uint32_t th = read<uint32_t>(tc + NetVars::m_hPawn);
            if (th) {
                uintptr_t tp = get_entity_from_handle(th);
                if (IsRemotePtrValid(tp) && tp != lp &&
                    read<uint8_t>(tp + NetVars::m_lifeState) == 0 &&
                    read<int32_t>(tp + NetVars::m_iHealth) > 0) {
                    if (!cfg.team_check || read<uint8_t>(tp + NetVars::m_iTeamNum) != local_team)
                        valid = true;
                }
            }
        }
    }

    if (!valid) {
        if (was_firing && g_offsets.clientBase) write<int>(g_offsets.clientBase + B_ATTACK, 0);
        was_firing = false;
        return;
    }

    if (cfg.max_velocity > 0.f) {
        Vector3 vel = read<Vector3>(lp + NetVars::m_vecVelocity);
        if (vel.length2d() > cfg.max_velocity) {
            if (was_firing && g_offsets.clientBase) write<int>(g_offsets.clientBase + B_ATTACK, 0);
            was_firing = false; return;
        }
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_last_shot).count();
    int delay = cfg.delay_min;
    if (cfg.delay_max > cfg.delay_min) {
        static std::mt19937 rng((unsigned)now.time_since_epoch().count());
        std::uniform_int_distribution<int> d(cfg.delay_min, cfg.delay_max);
        delay = d(rng);
    }
    if (elapsed < delay) { was_firing = true; return; }

    // 三路开火
    uintptr_t ms = read<uintptr_t>(lp + NetVars::m_pMovementServices);
    if (IsRemotePtrValid(ms)) {
        uint32_t btns = read<uint32_t>(ms + NetVars::m_nButtons);
        btns |= (1 << 0);
        write<uint32_t>(ms + NetVars::m_nButtons, btns);
    }
    if (g_offsets.clientBase) {
        write<int>(g_offsets.clientBase + B_ATTACK, 65537);
        write<int>(g_offsets.clientBase + B_ATTACK2, 0);
    }
    INPUT click[2] = {};
    click[0].type = INPUT_MOUSE; click[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    click[1].type = INPUT_MOUSE; click[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    ::SendInput(2, click, sizeof(INPUT));

    g_last_shot = now;
    was_firing = true;
}

} // namespace cs2::aimbot

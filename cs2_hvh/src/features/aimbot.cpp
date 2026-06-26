#include "aimbot.h"
#include "../core/memory.h"
#include "../core/offsets.h"
#include "../core/overlay.h"
#include <cmath>
#include <chrono>
#include <random>
#include <cstdio>

namespace cs2::aimbot {

using namespace ::cs2::memory;
using namespace ::cs2::offsets;

// ═══════════════════════════════════════════════════════════════
//  工具函数
// ═══════════════════════════════════════════════════════════════

static Vector3 get_bone_pos(uintptr_t pawn, int bone_idx, const Vector3* origin_hint = nullptr) {
    uintptr_t scene_node = read<uintptr_t>(pawn + NetVars::m_pGameSceneNode);
    if (!IsRemotePtrValid(scene_node)) return {};

    uintptr_t ms_addr = scene_node + NetVars::m_modelState;

    // 尝试多组骨指针偏移 (m_pBones 是非 schema 内部偏移, 不同版本可能不同)
    static const int BONE_PTR_OFFSETS[] = { 0x80, 0x70, 0x90, 0x60, 0xA0 };
    uintptr_t bone_array = 0;

    for (int off : BONE_PTR_OFFSETS) {
        uintptr_t candidate = read<uintptr_t>(ms_addr + off);
        if (!IsRemotePtrValid(candidate)) continue;
        // 试读一个骨, 检查是否合理
        Vector3 test = read<Vector3>(candidate + bone_idx * 0x20);
        if (origin_hint && test.length() > 0.1f) {
            float dist = test.dist_to(*origin_hint);
            if (dist > 10.f && dist < 150.f) { // 骨骼应离原点10-150单位
                bone_array = candidate;
                break;
            }
        } else if (test.length() > 0.1f) {
            bone_array = candidate;
            break;
        }
    }

    if (!bone_array) return {};
    return read<Vector3>(bone_array + bone_idx * 0x20);
}

// calc_angle 加 NaN/除零保护
static Vector3 safe_calc_angle(const Vector3& src, const Vector3& dst) {
    Vector3 delta = dst - src;
    float len = delta.length();
    if (len < 0.001f) return {};
    Vector3 ang;
    float dz = delta.z / len;
    if (dz < -1.f) dz = -1.f;
    if (dz >  1.f) dz =  1.f;
    ang.x = -asinf(dz) * 57.2957795131f;
    ang.y = atan2f(delta.y, delta.x) * 57.2957795131f;
    ang.z = 0.f;
    return ang;
}

// ═══════════════════════════════════════════════════════════════
//  方法1: 写角度 + RCS 压枪
// ═══════════════════════════════════════════════════════════════

static void aim_write_angles(const AimbotConfig& cfg,
                              uintptr_t local_pawn,
                              const Vector3& local_eye_pos,
                              uintptr_t target_pawn,
                              int target_bone)
{
    // 读头部骨骼 (如果骨骼数组指针不对就 fallback 到 origin + viewOffset)
    Vector3 to = read<Vector3>(target_pawn + NetVars::m_vOldOrigin);
    Vector3 head_pos = get_bone_pos(target_pawn, target_bone, &to);
    if (head_pos.length() < 0.001f) {
        head_pos = read<Vector3>(target_pawn + NetVars::m_vOldOrigin)
                 + read<Vector3>(target_pawn + NetVars::m_vecViewOffset);
    }

    Vector3 view_angles = read<Vector3>(local_pawn + NetVars::m_angEyeAngles);
    Vector3 aim_angle = safe_calc_angle(local_eye_pos, head_pos);
    if (aim_angle.length() < 0.001f) return;

    // ── 压枪 RCS ──────────────────────────────────────────────
    if (cfg.recoil_control) {
        uintptr_t aimpunch_svc = read<uintptr_t>(local_pawn + NetVars::m_pAimPunchServices);
        if (IsRemotePtrValid(aimpunch_svc)) {
            Vector3 punch = read<Vector3>(aimpunch_svc + 0x50); // CCSPlayer_AimPunchServices::m_aimPunchAngle
            float plen = punch.length();
            if (plen > 0.01f && plen < 90.f) { // 合法范围过滤
                aim_angle.x -= punch.x * cfg.rcs_scale;
                aim_angle.y -= punch.y * cfg.rcs_scale;
            }
        }
    }

    Vector3 delta = angle_diff(aim_angle, view_angles);

    // 平滑
    if (cfg.smoothness > 0.1f) {
        float f = 1.0f / cfg.smoothness;
        delta.x *= f;
        delta.y *= f;
    }

    Vector3 new_angle = view_angles + delta;

    // 钳位
    while (new_angle.y > 180.0f) new_angle.y -= 360.0f;
    while (new_angle.y < -180.0f) new_angle.y += 360.0f;
    if (new_angle.x > 89.0f) new_angle.x = 89.0f;
    if (new_angle.x < -89.0f) new_angle.x = -89.0f;
    new_angle.z = 0.0f;

    // 写视角 (直接写 + 间接写 都试)
    if (g_offsets.dwViewAngles) {
        write<Vector3>(g_offsets.dwViewAngles, new_angle);
    }
    uintptr_t ptr = read<uintptr_t>(g_offsets.dwViewAngles);
    if (ptr && ptr != g_offsets.dwViewAngles && IsRemotePtrValid(ptr)) {
        write<Vector3>(ptr, new_angle);
    }
}

// ═══════════════════════════════════════════════════════════════
//  方法2: SendInput (FutaZone)
// ═══════════════════════════════════════════════════════════════

static std::chrono::steady_clock::time_point g_last_aim_tick;
static std::chrono::steady_clock::time_point g_state_timer;
static bool     g_have_target = false;
static int      g_rand_mode = 0;
static float    g_err_x = 0.f, g_err_y = 0.f;

static void aim_sendinput(const AimbotConfig& cfg, const Vector2& screen_pos) {
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - g_last_aim_tick).count() < 10) return;
    g_last_aim_tick = now;

    if (!g_have_target) { g_state_timer = now; g_have_target = true;
        if (cfg.aim_mode == (int)AimMode::Random) {
            static std::mt19937 rng((unsigned)now.time_since_epoch().count());
            std::uniform_int_distribution<int> d(0,3); g_rand_mode = d(rng);
        }
    }

    float sw = (float)overlay::get_width(), sh = (float)overlay::get_height();
    Vector2 center(sw*0.5f, sh*0.5f);
    float smooth = cfg.smoothness;
    int mode = (cfg.aim_mode == (int)AimMode::Random) ? g_rand_mode : cfg.aim_mode;
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_state_timer).count();
    Vector2 ap = screen_pos;

    switch (mode) {
    case 1: smooth = (elapsed < cfg.speed_change_duration) ? std::max(0.1f,cfg.smoothness*0.5f) : cfg.smoothness*2.f; break;
    case 2: smooth = (elapsed < cfg.speed_change_duration) ? cfg.smoothness*2.f : std::max(0.1f,cfg.smoothness*0.5f); break;
    case 3:
        if (elapsed < cfg.speed_change_duration) {
            smooth = std::max(0.1f,cfg.smoothness*0.3f);
            Vector2 dir = ap - center; float d = dir.length();
            if (d > 1.f) { float e = cfg.overshoot_scale - 1.f; ap = ap + (dir/d)*(d*e + 30.f*e); }
        } else smooth = cfg.smoothness * 1.5f;
        break;
    default: break;
    }

    float dx = (ap.x - center.x) / smooth + g_err_x;
    float dy = (ap.y - center.y) / smooth + g_err_y;
    int mx = (int)dx, my = (int)dy;
    g_err_x = dx - mx; g_err_y = dy - my;
    if (mx || my) { INPUT in{}; in.type=INPUT_MOUSE; in.mi.dx=mx; in.mi.dy=my; in.mi.dwFlags=MOUSEEVENTF_MOVE; ::SendInput(1,&in,sizeof(INPUT)); }
}

// ═══════════════════════════════════════════════════════════════
//  Aimbot 主入口
// ═══════════════════════════════════════════════════════════════

void run(const AimbotConfig& cfg) {
    if (!cfg.enabled) { g_have_target = false; g_err_x = g_err_y = 0.f; return; }

    bool key_down = false;
    if (cfg.key_mode == 2) key_down = true;
    else if (cfg.key_mode == 0) key_down = overlay::is_key_down(cfg.key0) || (cfg.key1 && overlay::is_key_down(cfg.key1));
    else if (cfg.key_mode == 1) { static bool tog = false;
        if (overlay::was_key_pressed(cfg.key0) || (cfg.key1 && overlay::was_key_pressed(cfg.key1))) tog = !tog; key_down = tog; }
    if (!key_down) { g_have_target = false; g_err_x = g_err_y = 0.f; return; }

    uintptr_t ctrl = read<uintptr_t>(g_offsets.dwLocalPlayerController);
    if (!IsRemotePtrValid(ctrl)) return;
    uintptr_t lp = get_entity_from_handle(read<uint32_t>(ctrl + NetVars::m_hPawn));
    if (!IsRemotePtrValid(lp)) return;

    uint8_t local_team = read<uint8_t>(lp + NetVars::m_iTeamNum);
    Vector3 eye = read<Vector3>(lp + NetVars::m_vOldOrigin) + read<Vector3>(lp + NetVars::m_vecViewOffset);
    ViewMatrix vm = read<ViewMatrix>(g_offsets.dwViewMatrix);
    int sw = overlay::get_width(), sh = overlay::get_height();

    uintptr_t best = 0; float best_dist = 3.4e38f; Vector2 best_sp{};

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

        Vector3 o = read<Vector3>(p + NetVars::m_vOldOrigin);
        if (cfg.max_distance > 0.f && eye.dist_to(o) > cfg.max_distance) continue;

        Vector3 bw = get_bone_pos(p, cfg.target_bone, &o);
        if (bw.length() < 0.001f) bw = o + read<Vector3>(p + NetVars::m_vecViewOffset);

        Vector2 sp;
        if (!world_to_screen(bw, sp, vm, sw, sh)) continue;
        float d = (sp - Vector2(sw*0.5f, sh*0.5f)).length();
        if (d > cfg.fov) continue;
        if (d < best_dist) { best = p; best_dist = d; best_sp = sp; }
    }

    if (!best) { g_have_target = false; g_err_x = g_err_y = 0.f; return; }

    if (cfg.input_method == 0) aim_write_angles(cfg, lp, eye, best, cfg.target_bone);
    else                       aim_sendinput(cfg, best_sp);
}

// ═══════════════════════════════════════════════════════════════
//  Triggerbot — 使用正确的 Controller→Pawn 解析
// ═══════════════════════════════════════════════════════════════

static constexpr uintptr_t B_ATTACK  = 0x2065A90;
static constexpr uintptr_t B_ATTACK2 = 0x2065B20;

static std::chrono::steady_clock::time_point g_last_shot;

void triggerbot(const TriggerbotConfig& cfg) {
    static bool was_firing = false;

    if (!cfg.enabled) { was_firing = false; return; }
    if (cfg.key && !overlay::is_key_down(cfg.key)) { was_firing = false; return; }

    // ── 本地玩家 ──────────────────────────────────────────────
    uintptr_t ctrl = read<uintptr_t>(g_offsets.dwLocalPlayerController);
    if (!IsRemotePtrValid(ctrl)) { was_firing = false; return; }
    uintptr_t lp = get_entity_from_handle(read<uint32_t>(ctrl + NetVars::m_hPawn));
    if (!IsRemotePtrValid(lp)) { was_firing = false; return; }
    uint8_t local_team = read<uint8_t>(lp + NetVars::m_iTeamNum);

    // ── 读准星实体索引 ───────────────────────────────────────
    int ent_idx = read<int32_t>(lp + NetVars::m_iIDEntIndex);

    // ── 解析目标: Controller → m_hPawn → Pawn (和 aimbot 一致) ─
    bool valid = false;
    if (ent_idx > 0 && ent_idx <= 63) {
        uintptr_t target_ctrl = get_entity_from_index(ent_idx);
        if (IsRemotePtrValid(target_ctrl) && target_ctrl != ctrl) {
            uint32_t pawn_handle = read<uint32_t>(target_ctrl + NetVars::m_hPawn);
            if (pawn_handle) {
                uintptr_t tp = get_entity_from_handle(pawn_handle);
                if (IsRemotePtrValid(tp) && tp != lp) {
                    if (read<uint8_t>(tp + NetVars::m_lifeState) == 0 &&
                        read<int32_t>(tp + NetVars::m_iHealth) > 0) {
                        if (!cfg.team_check || read<uint8_t>(tp + NetVars::m_iTeamNum) != local_team) {
                            valid = true;
                        }
                    }
                }
            }
        }
    }

    if (!valid) {
        // 释放攻击键
        if (was_firing && g_offsets.clientBase) write<int>(g_offsets.clientBase + B_ATTACK, 0);
        was_firing = false;
        return;
    }

    // ── 速度阈值 ──────────────────────────────────────────────
    if (cfg.max_velocity > 0.f) {
        Vector3 vel = read<Vector3>(lp + NetVars::m_vecVelocity);
        if (vel.length2d() > cfg.max_velocity) {
            if (was_firing && g_offsets.clientBase) write<int>(g_offsets.clientBase + B_ATTACK, 0);
            was_firing = false; return;
        }
    }

    // ── 延迟检测 ──────────────────────────────────────────────
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_last_shot).count();
    int delay = cfg.delay_min;
    if (cfg.delay_max > cfg.delay_min) {
        static std::mt19937 rng((unsigned)now.time_since_epoch().count());
        std::uniform_int_distribution<int> d(cfg.delay_min, cfg.delay_max);
        delay = d(rng);
    }
    if (elapsed < delay) { was_firing = true; return; }

    // ── 三路开火 ──────────────────────────────────────────────

    // ① m_nButtons (同 BHop)
    uintptr_t ms = read<uintptr_t>(lp + NetVars::m_pMovementServices);
    if (IsRemotePtrValid(ms)) {
        uint32_t btns = read<uint32_t>(ms + NetVars::m_nButtons);
        btns |= (1 << 0); // IN_ATTACK
        write<uint32_t>(ms + NetVars::m_nButtons, btns);
    }

    // ② buttons::attack 直接写
    if (g_offsets.clientBase) {
        write<int>(g_offsets.clientBase + B_ATTACK, 65537);
        write<int>(g_offsets.clientBase + B_ATTACK2, 0);
    }

    // ③ SendInput 鼠标左键
    INPUT click[2] = {};
    click[0].type = INPUT_MOUSE; click[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    click[1].type = INPUT_MOUSE; click[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    ::SendInput(2, click, sizeof(INPUT));

    g_last_shot = now;
    was_firing = true;
}

} // namespace cs2::aimbot

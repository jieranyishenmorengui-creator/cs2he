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

// 读骨骼: 多骨联合验证 — 只接受头>骨盆+靠近眼睛的合理位置
static Vector3 get_bone_pos(uintptr_t pawn, int bone_idx, const Vector3* origin_hint = nullptr) {
    uintptr_t scene_node = read<uintptr_t>(pawn + NetVars::m_pGameSceneNode);
    if (!IsRemotePtrValid(scene_node)) return {};

    uintptr_t ms_addr = scene_node + NetVars::m_modelState;
    static const int BONE_PTR_OFFS[] = { 0x80, 0x70, 0x90, 0x60, 0xA0 };
    static const int POS_OFFS[] = { 0, 16 };
    // 期望的眼部高度 (用于验证)
    Vector3 eye_expected;
    if (origin_hint) eye_expected = *origin_hint + Vector3(0, 0, 64.f);

    for (int boff : BONE_PTR_OFFS) {
        uintptr_t cand = read<uintptr_t>(ms_addr + boff);
        if (!IsRemotePtrValid(cand)) continue;

        for (int poff : POS_OFFS) {
            Vector3 bone0 = read<Vector3>(cand + 1 * 0x20 + poff); // pelvis
            Vector3 bone7 = read<Vector3>(cand + 7 * 0x20 + poff); // head
            if (bone0.length() < 0.1f || bone7.length() < 0.1f) continue;

            // 联合验证: 头必须在骨盆上方至少20单位
            if (bone7.z <= bone0.z + 20.f) continue;

            // 如果给了 origin_hint, 检查头离眼睛位置不远
            if (origin_hint) {
                float d = bone7.dist_to(eye_expected);
                if (d < 5.f || d > 100.f) continue; // 离眼睛5-100单位才合理
            }

            // 通过全部验证 → 返回请求的骨骼
            return read<Vector3>(cand + bone_idx * 0x20 + poff);
        }
    }
    return {};
}

// 预测目标未来位置 (简单 velocity 提前量, 改善移动靶)
static Vector3 predict_position(const Vector3& current, uintptr_t pawn, float lead_time = 0.1f) {
    Vector3 vel = read<Vector3>(pawn + NetVars::m_vecVelocity);
    float speed = vel.length();
    if (speed > 5.0f) {
        return current + vel * lead_time;
    }
    return current;
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
    // ── 读骨骼位置 ────────────────────────────────────────────
    Vector3 to = read<Vector3>(target_pawn + NetVars::m_vOldOrigin);
    Vector3 head_pos = get_bone_pos(target_pawn, target_bone, &to);
    if (head_pos.length() < 0.001f) {
        head_pos = to + read<Vector3>(target_pawn + NetVars::m_vecViewOffset);
    }

    // ── 移动靶预测 (提前量) ──────────────────────────────────
    head_pos = predict_position(head_pos, target_pawn, 0.1f);

    // ── 计算角度 ──────────────────────────────────────────────
    Vector3 view_angles = read<Vector3>(local_pawn + NetVars::m_angEyeAngles);
    Vector3 aim_angle = safe_calc_angle(local_eye_pos, head_pos);
    if (aim_angle.length() < 0.001f) return;

    // ── 压枪 RCS ──────────────────────────────────────────────
    if (cfg.recoil_control) {
        uintptr_t aimpunch_svc = read<uintptr_t>(local_pawn + NetVars::m_pAimPunchServices);
        if (IsRemotePtrValid(aimpunch_svc)) {
            Vector3 punch = read<Vector3>(aimpunch_svc + 0x50);
            float plen = punch.length();
            if (plen > 0.01f && plen < 90.f) {
                aim_angle.x -= punch.x * cfg.rcs_scale;
                aim_angle.y -= punch.y * cfg.rcs_scale;
            }
        }
    }

    // ── 平滑: 目标切换时快拉, 大角度快速接近, 小角度标准修正 ─
    static uintptr_t s_last_target = 0;
    static int       s_frames = 0;

    if (target_pawn != s_last_target) {
        s_last_target = target_pawn;
        s_frames = 0;
    }
    s_frames++;

    Vector3 delta = angle_diff(aim_angle, view_angles);
    float angle_err = delta.length();

    float smooth_factor = cfg.smoothness;
    if (s_frames < 3) {
        // 切目标: 前三帧几乎瞬拉 (防左右乱晃)
        smooth_factor = 0.2f;
    } else if (angle_err > 3.0f) {
        // 大角度差: 快速接近
        smooth_factor = std::max(0.3f, cfg.smoothness * 0.3f);
    } else if (angle_err < 0.5f) {
        // 小角度: 标准平滑 (不再加倍)
        smooth_factor = cfg.smoothness;
    }
    // 中间区域: 默认 smoothness

    delta.x /= smooth_factor;
    delta.y /= smooth_factor;

    Vector3 new_angle = view_angles + delta;

    // ── 钳位 + NaN/Inf 过滤 ─────────────────────────────────
    if (std::isnan(new_angle.x) || std::isnan(new_angle.y) ||
        std::isinf(new_angle.x) || std::isinf(new_angle.y)) return;
    if (new_angle.x > 89.0f) new_angle.x = 89.0f;
    if (new_angle.x < -89.0f) new_angle.x = -89.0f;
    while (new_angle.y > 180.0f) new_angle.y -= 360.0f;
    while (new_angle.y < -180.0f) new_angle.y += 360.0f;
    new_angle.z = 0.0f;

    // ── 额外安全检查: 如果角度变化太离谱, 丢弃这帧 ──────────
    Vector3 total_delta = new_angle - view_angles;
    if (std::abs(total_delta.x) > 45.f || std::abs(total_delta.y) > 90.f) return;

    // ── 写视角 ──────────────────────────────────────────────
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

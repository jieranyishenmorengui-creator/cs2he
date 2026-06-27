#include "aimbot.h"
#include "../core/memory.h"
#include "../core/offsets.h"
#include "../core/overlay.h"
#include <cmath>
#include <chrono>
#include <random>
#include <unordered_map>

static constexpr float PIf = 3.14159265358979323846f;

namespace cs2::aimbot {

using namespace ::cs2::memory;
using namespace ::cs2::offsets;

// ═════════════════════════════════════════════════════════════
//  读骨骼
// ═════════════════════════════════════════════════════════════

static Vector3 get_bone_pos(uintptr_t pawn, int bone_idx) {
    uintptr_t sn = read<uintptr_t>(pawn + NetVars::m_pGameSceneNode);
    if (!IsRemotePtrValid(sn)) return {};
    uintptr_t ba = read<uintptr_t>(sn + NetVars::m_modelState + 0x80);
    if (!IsRemotePtrValid(ba)) return {};
    uint8_t raw[30 * 0x20];
    if (!read(ba, raw, sizeof(raw))) return {};
    float* pf = (float*)(raw + bone_idx * 0x20);
    return Vector3(pf[0], pf[1], pf[2]);
}

static Vector3 calc_angle_safe(const Vector3& src, const Vector3& dst) {
    Vector3 d = dst - src;
    float len = d.length();
    if (len < 0.01f) return {};
    float clp = std::clamp(d.z / len, -1.f, 1.f);
    return { -asinf(clp) * 57.2957795131f, atan2f(d.y, d.x) * 57.2957795131f, 0.f };
}

// ═════════════════════════════════════════════════════════════
//  侧身头部补偿 (tiansongyu/cs2_cheat)
// ═════════════════════════════════════════════════════════════

static Vector3 adjust_head_for_facing(const AimbotConfig& cfg,
                                       uintptr_t enemy_pawn,
                                       const Vector3& head_pos,
                                       const Vector3& eye_pos)
{
    if (!cfg.head_offset_enabled) return head_pos;

    // 读敌人朝向
    Vector3 enemy_ang = read<Vector3>(enemy_pawn + NetVars::m_angEyeAngles);
    float enemy_yaw = enemy_ang.y;

    // 算玩家看敌人的角度
    Vector3 delta = enemy_yaw > 0 ? eye_pos - head_pos : head_pos - eye_pos;
    // Actually: calculate angle from player to enemy
    float angle_to_enemy = atan2f(head_pos.y - eye_pos.y, head_pos.x - eye_pos.x)
                         * 57.2957795131f;
    if (angle_to_enemy < 0) angle_to_enemy += 360.f;

    // 敌人朝向和玩家看敌人方向的角度差
    float view_diff = fabsf(enemy_yaw - angle_to_enemy);
    if (view_diff > 180.f) view_diff = 360.f - view_diff;

    // 角度在补偿范围内才补偿
    if (view_diff < cfg.head_offset_angle_min || view_diff > cfg.head_offset_angle_max)
        return head_pos;

    // 补偿量: 最大在90°(正侧面) x [0,1]
    float center = (cfg.head_offset_angle_min + cfg.head_offset_angle_max) / 2.f;
    float range = (cfg.head_offset_angle_max - cfg.head_offset_angle_min) / 2.f;
    float factor = 1.f - fabsf(view_diff - center) / range;
    factor = std::clamp(factor, 0.f, 1.f);

    float yaw_rad = enemy_yaw * static_cast<float>(PIf) / 180.f;
    float offset_x = cosf(yaw_rad) * cfg.head_offset_amount * factor;
    float offset_y = sinf(yaw_rad) * cfg.head_offset_amount * factor;

    return { head_pos.x + offset_x, head_pos.y + offset_y, head_pos.z };
}

// ═════════════════════════════════════════════════════════════
//  EMA 位置平滑
// ═════════════════════════════════════════════════════════════

static std::unordered_map<uintptr_t, Vector3> s_target_cache;

static void clear_ema() { s_target_cache.clear(); }

static Vector3 ema_target(uintptr_t pawn, const Vector3& cur, float smoothness) {
    auto it = s_target_cache.find(pawn);
    if (it == s_target_cache.end()) {
        s_target_cache[pawn] = cur;
        return cur;
    }
    float alpha = (smoothness <= 1.f) ? 0.65f :
                  (smoothness >= 10.f) ? 0.15f :
                  0.65f - (smoothness - 1.f) / 9.f * 0.5f;
    Vector3 s = it->second + (cur - it->second) * alpha;
    s_target_cache[pawn] = s;
    return s;
}

// ═════════════════════════════════════════════════════════════
//  Aimbot
// ═════════════════════════════════════════════════════════════

static uintptr_t g_last = 0;
static int      g_last_hp = 0;
static std::chrono::steady_clock::time_point g_kt;
static bool     g_kcd = false;
static int      g_pc = 0;

void run(const AimbotConfig& cfg) {
    if (!cfg.enabled) { g_last = 0; g_kcd = false; clear_ema(); return; }

    bool key_down = false;
    if (cfg.key_mode == 2) key_down = true;
    else if (cfg.key_mode == 0)
        key_down = overlay::is_key_down(cfg.key0) || (cfg.key1 && overlay::is_key_down(cfg.key1));
    else if (cfg.key_mode == 1) {
        static bool t = false;
        if (overlay::was_key_pressed(cfg.key0) || (cfg.key1 && overlay::was_key_pressed(cfg.key1))) t = !t;
        key_down = t;
    }
    if (!key_down) { g_last = 0; g_kcd = false; clear_ema(); return; }

    if (cfg.kill_delay_ms > 0 && g_kcd) {
        auto e = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - g_kt).count();
        if (e < (uint64_t)cfg.kill_delay_ms) return;
        g_kcd = false;
    }

    uintptr_t ctrl = read<uintptr_t>(g_offsets.dwLocalPlayerController);
    if (!IsRemotePtrValid(ctrl)) return;
    uintptr_t lp = get_entity_from_handle(read<uint32_t>(ctrl + NetVars::m_hPawn));
    if (!IsRemotePtrValid(lp)) return;

    uint8_t local_team = read<uint8_t>(lp + NetVars::m_iTeamNum);
    Vector3 eye = read<Vector3>(lp + NetVars::m_vOldOrigin)
                + read<Vector3>(lp + NetVars::m_vecViewOffset);

    if (cfg.disable_when_flashed) {
        float f = read<float>(lp + NetVars::m_flFlashDuration);
        if (f > 0.1f && f < 200.f && f > cfg.flash_threshold) return;
    }

    ViewMatrix vm = read<ViewMatrix>(g_offsets.dwViewMatrix);
    int sw = overlay::get_width(), sh = overlay::get_height();

    // ── 实体扫描 ────────────────────────────────────────────
    uintptr_t elb = read<uintptr_t>(g_offsets.dwEntityList);
    uintptr_t best = 0;
    float best_score = 3.4e38f;

    for (int i = 1; i < 64; ++i) {
        uintptr_t ch = read<uintptr_t>(elb + 8 * (i >> 9) + 0x10);
        if (!IsRemotePtrValid(ch)) continue;
        uintptr_t c = read<uintptr_t>(ch + 112 * (i & 0x1FF));
        if (!IsRemotePtrValid(c) || c == ctrl) continue;

        uint32_t ph = read<uint32_t>(c + NetVars::m_hPawn);
        if (!ph) continue;
        ch = read<uintptr_t>(elb + 8 * ((ph & 0x7FFF) >> 9) + 0x10);
        if (!IsRemotePtrValid(ch)) continue;
        uintptr_t p = read<uintptr_t>(ch + 112 * (ph & 0x7FFF & 0x1FF));
        if (!IsRemotePtrValid(p) || p == lp) continue;

        int hp = read<int32_t>(p + NetVars::m_iHealth);
        if (hp <= 0) continue;
        if (read<uint8_t>(p + NetVars::m_lifeState) != 0) continue;
        if (cfg.team_check && read<uint8_t>(p + NetVars::m_iTeamNum) == local_team) continue;

        uintptr_t sn = read<uintptr_t>(p + NetVars::m_pGameSceneNode);
        if (IsRemotePtrValid(sn) && read<bool>(sn + 0x103)) continue;
        if (cfg.visible_check && !read<bool>(p + NetVars::m_entitySpottedState + NetVars::m_bSpotted)) continue;

        Vector3 o = read<Vector3>(p + NetVars::m_vOldOrigin);
        float wd = eye.dist_to(o);
        if (cfg.max_distance > 0.f && wd > cfg.max_distance) continue;

        // 骨骼 + 侧身头部补偿
        Vector3 bp = get_bone_pos(p, cfg.target_bone);
        if (bp.length() < 0.1f) bp = o + read<Vector3>(p + NetVars::m_vecViewOffset);
        bp = adjust_head_for_facing(cfg, p, bp, eye);

        Vector2 sp;
        if (!world_to_screen(bp, sp, vm, sw, sh)) continue;
        float fd = (sp - Vector2(sw*0.5f, sh*0.5f)).length();
        if (fd > cfg.fov) continue;

        // 评分: 0=FOV  1=距离  2=血量
        float score;
        if (cfg.aim_priority == 1)
            score = wd;                     // 距离优先
        else if (cfg.aim_priority == 2)
            score = (float)hp + wd * 0.001f; // 血量优先 (残血=低分, 距离做tiebreaker)
        else
            score = fd + wd * 0.01f;        // FOV优先 + 距离微调

        if (score < best_score) {
            best = p; best_score = score;
        }
    }

    if (++g_pc > 30) { g_pc = 0; for (auto it = s_target_cache.begin(); it != s_target_cache.end();) {
        int h = read<int32_t>(it->first + NetVars::m_iHealth);
        if (h <= 0 || h > 200) it = s_target_cache.erase(it); else ++it;
    }}

    // 滞回: 上帧目标还在且分差<15%, 不换
    if (g_last && g_last != best) {
        int old_hp = read<int32_t>(g_last + NetVars::m_iHealth);
        if (old_hp > 0 && read<uint8_t>(g_last + NetVars::m_lifeState) == 0) {
            Vector3 old_o = read<Vector3>(g_last + NetVars::m_vOldOrigin);
            Vector3 old_bp = get_bone_pos(g_last, cfg.target_bone);
            if (old_bp.length() < 0.1f) old_bp = old_o + read<Vector3>(g_last + NetVars::m_vecViewOffset);
            Vector2 old_sp;
            if (world_to_screen(old_bp, old_sp, vm, sw, sh)) {
                float old_fd = (old_sp - Vector2(sw*0.5f, sh*0.5f)).length();
                float old_wd = eye.dist_to(old_o);
                float old_score;
                if (cfg.aim_priority == 1) old_score = old_wd;
                else if (cfg.aim_priority == 2) old_score = (float)old_hp + old_wd * 0.001f;
                else old_score = old_fd + old_wd * 0.01f;
                if (old_score <= best_score * 1.15f) best = g_last;
            }
        }
    }

    if (!best) { g_last = 0; return; }

    // 击杀检测
    int chp = read<int32_t>(best + NetVars::m_iHealth);
    if (g_last == best) {
        if (g_last_hp > 0 && chp <= 0) { g_kcd = true; g_kt = std::chrono::steady_clock::now(); g_last = 0; return; }
    } else if (g_last && g_last_hp > 0) {
        int oh = read<int32_t>(g_last + NetVars::m_iHealth);
        if (oh <= 0) { g_kcd = true; g_kt = std::chrono::steady_clock::now(); g_last = 0; return; }
    }
    g_last = best; g_last_hp = chp;
    if (chp <= 0) return;

    // 目标骨骼 + 侧身补偿 + EMA
    Vector3 to = read<Vector3>(best + NetVars::m_vOldOrigin);
    Vector3 rp = get_bone_pos(best, cfg.target_bone);
    if (rp.length() < 0.1f) rp = to + read<Vector3>(best + NetVars::m_vecViewOffset);
    rp = adjust_head_for_facing(cfg, best, rp, eye);
    Vector3 tp = ema_target(best, rp, cfg.smoothness);

    // 提前量
    Vector3 ap = tp, ae = eye;
    if (cfg.lead_time > 0.f) {
        Vector3 tv = read<Vector3>(best + NetVars::m_vecVelocity);
        Vector3 lv = read<Vector3>(lp + NetVars::m_vecVelocity);
        if ((tv - lv).length() > 10.f) {
            float lt = std::min(cfg.lead_time, 0.15f);
            ap = tp + tv * lt; ae = eye + lv * lt;
        }
    }

    Vector3 aim = calc_angle_safe(ae, ap);
    if (aim.length() < 0.01f) return;

    // RCS
    if (cfg.recoil_control) {
        uintptr_t as = read<uintptr_t>(lp + NetVars::m_pAimPunchServices);
        if (IsRemotePtrValid(as)) {
            Vector3 pu = read<Vector3>(as + 0x50);
            if (pu.length() > 0.01f && pu.length() < 90.f) {
                aim.x -= pu.x * cfg.rcs_scale;
                aim.y -= pu.y * cfg.rcs_scale;
            }
        }
    }

    Vector3 cur = read<Vector3>(lp + NetVars::m_angEyeAngles);
    Vector3 delta = aim - cur;
    while (delta.y > 180.f) delta.y -= 360.f;
    while (delta.y < -180.f) delta.y += 360.f;
    if (fabsf(delta.x) < 0.05f && fabsf(delta.y) < 0.05f) return;

    if (cfg.smoothness > 0.1f) { delta.x /= cfg.smoothness; delta.y /= cfg.smoothness; }

    Vector3 out = cur + delta;
    out.x = std::clamp(out.x, -89.f, 89.f);
    while (out.y > 180.f) out.y -= 360.f;
    while (out.y < -180.f) out.y += 360.f;
    out.z = 0.f;

    if (g_offsets.dwViewAngles) write<Vector3>(g_offsets.dwViewAngles, out);
    uintptr_t pr = read<uintptr_t>(g_offsets.dwViewAngles);
    if (pr && pr != g_offsets.dwViewAngles && IsRemotePtrValid(pr)) write<Vector3>(pr, out);
}

// ═════════════════════════════════════════════════════════════
//  Triggerbot — 支持两种模式
//  mode=0: m_iIDEntIndex (原)   mode=1: FOV角度检测
// ═════════════════════════════════════════════════════════════

static constexpr uintptr_t B_ATTACK  = 0x2065A90;
static constexpr uintptr_t B_ATTACK2 = 0x2065B20;
static std::chrono::steady_clock::time_point g_last_shot;

// FOV模式辅助
static float fov_angle(const Vector3& a, const Vector3& b) {
    float dy = b.y - a.y;
    while (dy > 180.f) dy -= 360.f; while (dy < -180.f) dy += 360.f;
    float dp = b.x - a.x;
    return sqrtf(dp*dp + dy*dy);
}

void triggerbot(const TriggerbotConfig& cfg) {
    static bool wf = false;
    if (!cfg.enabled) { wf = false; return; }
    if (cfg.key && !overlay::is_key_down(cfg.key)) { wf = false; return; }

    uintptr_t ctrl = read<uintptr_t>(g_offsets.dwLocalPlayerController);
    if (!IsRemotePtrValid(ctrl)) { wf = false; return; }
    uintptr_t lp = get_entity_from_handle(read<uint32_t>(ctrl + NetVars::m_hPawn));
    if (!IsRemotePtrValid(lp)) { wf = false; return; }
    uint8_t lt = read<uint8_t>(lp + NetVars::m_iTeamNum);

    bool valid = false;

    if (cfg.mode == 0) {
        // 模式0: m_iIDEntIndex
        int ei = read<int32_t>(lp + NetVars::m_iIDEntIndex);
        if (ei > 0 && ei <= 63) {
            uintptr_t tc = get_entity_from_index(ei);
            if (IsRemotePtrValid(tc) && tc != ctrl) {
                uint32_t th = read<uint32_t>(tc + NetVars::m_hPawn);
                if (th) {
                    uintptr_t tp = get_entity_from_handle(th);
                    if (IsRemotePtrValid(tp) && tp != lp &&
                        read<uint8_t>(tp + NetVars::m_lifeState) == 0 &&
                        read<int32_t>(tp + NetVars::m_iHealth) > 0)
                        if (!cfg.team_check || read<uint8_t>(tp + NetVars::m_iTeamNum) != lt)
                            valid = true;
                }
            }
        }
    } else {
        // 模式1: FOV角度检测
        Vector3 va = read<Vector3>(lp + NetVars::m_angEyeAngles);
        Vector3 eye = read<Vector3>(lp + NetVars::m_vOldOrigin)
                    + read<Vector3>(lp + NetVars::m_vecViewOffset);
        uintptr_t elb = read<uintptr_t>(g_offsets.dwEntityList);

        for (int i = 1; i < 64 && !valid; ++i) {
            uintptr_t ch = read<uintptr_t>(elb + 8*(i>>9) + 0x10);
            if (!IsRemotePtrValid(ch)) continue;
            uintptr_t c = read<uintptr_t>(ch + 112*(i&0x1FF));
            if (!IsRemotePtrValid(c) || c == ctrl) continue;

            uint32_t ph = read<uint32_t>(c + NetVars::m_hPawn);
            if (!ph) continue;
            ch = read<uintptr_t>(elb + 8*((ph&0x7FFF)>>9) + 0x10);
            if (!IsRemotePtrValid(ch)) continue;
            uintptr_t p = read<uintptr_t>(ch + 112*(ph&0x7FFF&0x1FF));
            if (!IsRemotePtrValid(p) || p == lp) continue;

            if (read<int32_t>(p + NetVars::m_iHealth) <= 0) continue;
            if (read<uint8_t>(p + NetVars::m_lifeState) != 0) continue;
            if (cfg.team_check && read<uint8_t>(p + NetVars::m_iTeamNum) == lt) continue;

            uintptr_t sn = read<uintptr_t>(p + NetVars::m_pGameSceneNode);
            if (IsRemotePtrValid(sn) && read<bool>(sn+0x103)) continue;

            Vector3 bp = get_bone_pos(p, 7);
            if (bp.length() < 0.1f)
                bp = read<Vector3>(p+NetVars::m_vOldOrigin) + read<Vector3>(p+NetVars::m_vecViewOffset);
            Vector3 aa = calc_angle_safe(eye, bp);
            if (fov_angle(va, aa) <= cfg.fov_threshold) valid = true;
        }
    }

    if (!valid) {
        if (wf && g_offsets.clientBase) write<int>(g_offsets.clientBase + B_ATTACK, 0);
        wf = false; return;
    }

    if (cfg.max_velocity > 0.f) {
        Vector3 vl = read<Vector3>(lp + NetVars::m_vecVelocity);
        if (vl.length2d() > cfg.max_velocity) {
            if (wf && g_offsets.clientBase) write<int>(g_offsets.clientBase + B_ATTACK, 0);
            wf = false; return;
        }
    }

    auto now = std::chrono::steady_clock::now();
    auto el = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_last_shot).count();
    int dly = cfg.delay_min;
    if (cfg.delay_max > cfg.delay_min) {
        static std::mt19937 rng((unsigned)now.time_since_epoch().count());
        std::uniform_int_distribution<int> d(cfg.delay_min, cfg.delay_max);
        dly = d(rng);
    }
    if (el < dly) { wf = true; return; }

    uintptr_t ms = read<uintptr_t>(lp + NetVars::m_pMovementServices);
    if (IsRemotePtrValid(ms)) {
        uint32_t b = read<uint32_t>(ms + NetVars::m_nButtons); b |= (1<<0); write<uint32_t>(ms + NetVars::m_nButtons, b);
    }
    if (g_offsets.clientBase) { write<int>(g_offsets.clientBase + B_ATTACK, 65537); write<int>(g_offsets.clientBase + B_ATTACK2, 0); }
    INPUT clk[2] = {};
    clk[0].type = INPUT_MOUSE; clk[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    clk[1].type = INPUT_MOUSE; clk[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    ::SendInput(2, clk, sizeof(INPUT));
    g_last_shot = now; wf = true;
}

} // namespace cs2::aimbot

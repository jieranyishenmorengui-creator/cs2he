#include "aimbot.h"
#include "../core/memory.h"
#include "../core/offsets.h"
#include "../core/overlay.h"
#include <cmath>
#include <chrono>
#include <random>
#include <unordered_map>

namespace cs2::aimbot {

using namespace ::cs2::memory;
using namespace ::cs2::offsets;

// ══════════════════════════════════════════════════════════════
//  读骨骼
// ══════════════════════════════════════════════════════════════

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

// ══════════════════════════════════════════════════════════════
//  EMA 位置平滑 (和 ESP 框同算法)
// ══════════════════════════════════════════════════════════════

// 平滑因子: 和 ESP 同步, 从 cfg.smoothness 映射
// smooth_factor 越大越平滑越跟的紧
static float ema_alpha(float smoothness) {
    // smoothness 1-10 映射到 alpha 0-1
    // 1.0 → alpha 0.65 (快)   10.0 → alpha 0.15 (慢)
    if (smoothness <= 1.f) return 0.65f;
    if (smoothness >= 10.f) return 0.15f;
    return 0.65f - (smoothness - 1.f) / 9.f * 0.5f;
}

static std::unordered_map<uintptr_t, Vector3> s_target_pos_cache;

static Vector3 ema_smooth_target(uintptr_t pawn, const Vector3& cur_pos, float smoothness) {
    auto it = s_target_pos_cache.find(pawn);
    if (it == s_target_pos_cache.end()) {
        s_target_pos_cache[pawn] = cur_pos;
        return cur_pos;  // 新目标: 不延迟, 第一帧直接用
    }
    float alpha = ema_alpha(smoothness);
    Vector3 smoothed = it->second + (cur_pos - it->second) * alpha;
    s_target_pos_cache[pawn] = smoothed;
    return smoothed;
}

// 清理死实体的缓存 (每次遍历后)
static void prune_ema_cache() {
    for (auto it = s_target_pos_cache.begin(); it != s_target_pos_cache.end();) {
        int hp = read<int32_t>(it->first + NetVars::m_iHealth);
        if (hp <= 0 || hp > 200) it = s_target_pos_cache.erase(it);
        else ++it;
    }
}

// ══════════════════════════════════════════════════════════════
//  Aimbot
// ══════════════════════════════════════════════════════════════

static uintptr_t          g_last_target_pawn = 0;
static int                g_last_target_hp   = 0;
static std::chrono::steady_clock::time_point g_kill_time;
static bool               g_kill_cooldown    = false;
static int                g_prune_counter    = 0;

void run(const AimbotConfig& cfg) {
    if (!cfg.enabled) {
        g_last_target_pawn = 0;
        g_kill_cooldown = false;
        s_target_pos_cache.clear();
        return;
    }

    // ── 按键 ─────────────────────────────────────────────────
    bool key_down = false;
    if (cfg.key_mode == 2) key_down = true;
    else if (cfg.key_mode == 0)
        key_down = overlay::is_key_down(cfg.key0) || (cfg.key1 && overlay::is_key_down(cfg.key1));
    else if (cfg.key_mode == 1) {
        static bool tog = false;
        if (overlay::was_key_pressed(cfg.key0) || (cfg.key1 && overlay::was_key_pressed(cfg.key1))) tog = !tog;
        key_down = tog;
    }
    if (!key_down) {
        g_last_target_pawn = 0;
        g_kill_cooldown = false;
        s_target_pos_cache.clear();  // 松键清缓存, 下次按键第一帧直接锁
        return;
    }

    // ── 击杀冷却 ─────────────────────────────────────────────
    if (cfg.kill_delay_ms > 0 && g_kill_cooldown) {
        auto e = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - g_kill_time).count();
        if (e < (uint64_t)cfg.kill_delay_ms) return;
        g_kill_cooldown = false;
    }

    // ── Local ────────────────────────────────────────────────
    uintptr_t ctrl = read<uintptr_t>(g_offsets.dwLocalPlayerController);
    if (!IsRemotePtrValid(ctrl)) return;
    uintptr_t lp = get_entity_from_handle(read<uint32_t>(ctrl + NetVars::m_hPawn));
    if (!IsRemotePtrValid(lp)) return;

    uint8_t local_team = read<uint8_t>(lp + NetVars::m_iTeamNum);
    Vector3 eye = read<Vector3>(lp + NetVars::m_vOldOrigin)
                + read<Vector3>(lp + NetVars::m_vecViewOffset);

    ViewMatrix vm = read<ViewMatrix>(g_offsets.dwViewMatrix);
    int sw = overlay::get_width(), sh = overlay::get_height();

    // ── 闪白检测 ─────────────────────────────────────────────
    if (cfg.disable_when_flashed) {
        float flash = read<float>(lp + NetVars::m_flFlashDuration);
        // 只有合理范围才拦截 (>0且<200, 防止offset错误读垃圾值)
        if (flash > 0.1f && flash < 200.f && flash > cfg.flash_threshold) return;
    }

    // ── 遍历找目标 (加权评分: FOV + 距离×0.01) ──────────────
    uintptr_t best = 0;
    float     best_score = 3.4e38f;

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

        // Dormant
        uintptr_t sn = read<uintptr_t>(p + NetVars::m_pGameSceneNode);
        if (IsRemotePtrValid(sn) && read<bool>(sn + 0x103)) continue;

        // 可见性检查 (m_bSpotted)
        if (cfg.visible_check) {
            bool spotted = read<bool>(p + NetVars::m_entitySpottedState + NetVars::m_bSpotted);
            if (!spotted) continue;
        }

        Vector3 origin = read<Vector3>(p + NetVars::m_vOldOrigin);
        float world_dist = eye.dist_to(origin);
        if (cfg.max_distance > 0.f && world_dist > cfg.max_distance) continue;

        // 骨骼/fallback → EMA 平滑 (和 ESP 框同算法)
        Vector3 raw_bp = get_bone_pos(p, cfg.target_bone);
        if (raw_bp.length() < 0.1f)
            raw_bp = origin + read<Vector3>(p + NetVars::m_vecViewOffset);
        Vector3 bp = ema_smooth_target(p, raw_bp, cfg.smoothness);

        // W2S
        Vector2 sp;
        if (!world_to_screen(bp, sp, vm, sw, sh)) continue;
        float fov_dist = (sp - Vector2(sw*0.5f, sh*0.5f)).length();
        if (fov_dist > cfg.fov) continue;

        // ── 加权评分: FOV主导 + 距离微量修正 ───────────────
        float score = fov_dist + world_dist * 0.01f;
        if (score < best_score) { best = p; best_score = score; }
    }

    // 定期清理缓存
    if (++g_prune_counter > 30) { g_prune_counter = 0; prune_ema_cache(); }

    if (!best) { g_last_target_pawn = 0; return; }

    // ── 击杀检测 ─────────────────────────────────────────────
    auto trigger_cooldown = [&]() {
        g_kill_cooldown = true;
        g_kill_time = std::chrono::steady_clock::now();
        g_last_target_pawn = 0;
    };
    int cur_hp = read<int32_t>(best + NetVars::m_iHealth);
    if (g_last_target_pawn == best) {
        if (g_last_target_hp > 0 && cur_hp <= 0) { trigger_cooldown(); return; }
    } else if (g_last_target_pawn && g_last_target_hp > 0) {
        int old_hp = read<int32_t>(g_last_target_pawn + NetVars::m_iHealth);
        if (old_hp <= 0) { trigger_cooldown(); return; }
    }
    g_last_target_pawn = best;
    g_last_target_hp = cur_hp;
    if (cur_hp <= 0) return;

    // ── 目标骨骼 (EMA 已平滑) ──────────────────────────────
    Vector3 target_origin = read<Vector3>(best + NetVars::m_vOldOrigin);
    Vector3 raw_pos = get_bone_pos(best, cfg.target_bone);
    if (raw_pos.length() < 0.1f)
        raw_pos = target_origin + read<Vector3>(best + NetVars::m_vecViewOffset);
    Vector3 target_pos = ema_smooth_target(best, raw_pos, cfg.smoothness);

    // ── 提前量 ────────────────────────────────────────────────
    Vector3 aim_pos = target_pos;
    Vector3 aim_eye = eye;
    if (cfg.lead_time > 0.f) {
        Vector3 tvel = read<Vector3>(best + NetVars::m_vecVelocity);
        Vector3 lvel = read<Vector3>(lp + NetVars::m_vecVelocity);
        if ((tvel - lvel).length() > 10.f) {
            float lt = std::min(cfg.lead_time, 0.15f);
            aim_pos = target_pos + tvel * lt;
            aim_eye = eye + lvel * lt;
        }
    }

    // ── 计算角度 ────────────────────────────────────────────
    Vector3 aim = calc_angle_safe(aim_eye, aim_pos);
    if (aim.length() < 0.01f) return;

    // ── RCS ──────────────────────────────────────────────────
    if (cfg.recoil_control) {
        uintptr_t as = read<uintptr_t>(lp + NetVars::m_pAimPunchServices);
        if (IsRemotePtrValid(as)) {
            Vector3 punch = read<Vector3>(as + 0x50);
            if (punch.length() > 0.01f && punch.length() < 90.f) {
                aim.x -= punch.x * cfg.rcs_scale;
                aim.y -= punch.y * cfg.rcs_scale;
            }
        }
    }

    // ── 差值 → 平滑 → 写入 ────────────────────────────────
    Vector3 cur = read<Vector3>(lp + NetVars::m_angEyeAngles);
    Vector3 delta = aim - cur;
    while (delta.y > 180.f) delta.y -= 360.f;
    while (delta.y < -180.f) delta.y += 360.f;

    // 死区
    if (fabsf(delta.x) < 0.05f && fabsf(delta.y) < 0.05f) return;

    if (cfg.smoothness > 0.1f) {
        float f = 1.0f / cfg.smoothness;
        delta.x *= f;
        delta.y *= f;
    }

    Vector3 out = cur + delta;
    out.x = std::clamp(out.x, -89.f, 89.f);
    while (out.y > 180.f) out.y -= 360.f;
    while (out.y < -180.f) out.y += 360.f;
    out.z = 0.f;

    if (g_offsets.dwViewAngles) write<Vector3>(g_offsets.dwViewAngles, out);
    uintptr_t ptr = read<uintptr_t>(g_offsets.dwViewAngles);
    if (ptr && ptr != g_offsets.dwViewAngles && IsRemotePtrValid(ptr))
        write<Vector3>(ptr, out);
}

// ══════════════════════════════════════════════════════════════
//  Triggerbot
// ══════════════════════════════════════════════════════════════

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
        was_firing = false; return;
    }
    if (cfg.max_velocity > 0.f) {
        Vector3 vel = read<Vector3>(lp + NetVars::m_vecVelocity);
        if (vel.length2d() > cfg.max_velocity) {
            if (was_firing && g_offsets.clientBase) write<int>(g_offsets.clientBase + B_ATTACK, 0);
            was_firing = false; return;
        }
    }

    auto now = std::chrono::steady_clock::now();
    auto e = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_last_shot).count();
    int delay = cfg.delay_min;
    if (cfg.delay_max > cfg.delay_min) {
        static std::mt19937 rng((unsigned)now.time_since_epoch().count());
        std::uniform_int_distribution<int> d(cfg.delay_min, cfg.delay_max);
        delay = d(rng);
    }
    if (e < delay) { was_firing = true; return; }

    uintptr_t ms = read<uintptr_t>(lp + NetVars::m_pMovementServices);
    if (IsRemotePtrValid(ms)) { uint32_t b = read<uint32_t>(ms+NetVars::m_nButtons); b|=(1<<0); write<uint32_t>(ms+NetVars::m_nButtons, b); }
    if (g_offsets.clientBase) { write<int>(g_offsets.clientBase + B_ATTACK, 65537); write<int>(g_offsets.clientBase + B_ATTACK2, 0); }
    INPUT click[2] = {};
    click[0].type = INPUT_MOUSE; click[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    click[1].type = INPUT_MOUSE; click[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    ::SendInput(2, click, sizeof(INPUT));
    g_last_shot = now; was_firing = true;
}

} // namespace cs2::aimbot

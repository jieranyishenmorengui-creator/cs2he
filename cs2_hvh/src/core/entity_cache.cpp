#include "entity_cache.h"
#include "memory.h"
#include "offsets.h"
#include <mutex>
#include <cstring>
#include <cstdio>
#include <unordered_map>

namespace cs2::entity_cache {

using namespace ::cs2::memory;
using namespace ::cs2::offsets;

// ── Forward-declare internal structures ──────────────────────
struct BoneCacheVal { Vector3 bones[30]; int count; };
struct ObsCacheVal { uintptr_t services; uint32_t target; };
static std::unordered_map<uintptr_t, BoneCacheVal> s_bone_pool;
static std::unordered_map<uintptr_t, ObsCacheVal>  s_obs_cache;

// ── Internal state ──────────────────────────────────────────

static CacheSnapshot s_cache;       // written by update(), read by read()
static std::mutex    s_mutex;

// Tiered throttling counters (game thread runs ~1000Hz)
static int  s_tier2 = 0;   // bones          → every 3  scans (~333Hz)
static int  s_tier3 = 0;   // weapon names   → every 144 scans (~7Hz)
static constexpr int TIER2_INTERVAL = 3;
static constexpr int TIER3_INTERVAL = 144;

// ── Scan implementation ─────────────────────────────────────

void update() {
    // Advance throttle counters
    s_tier2 = (s_tier2 + 1) % TIER2_INTERVAL;
    s_tier3 = (s_tier3 + 1) % TIER3_INTERVAL;
    bool tier2 = (s_tier2 == 0);
    bool tier3 = (s_tier3 == 0);

    CacheSnapshot snap;

    // ── Local player ───────────────────────────────────────────
    uintptr_t lc = read<uintptr_t>(g_offsets.dwLocalPlayerController);
    if (IsRemotePtrValid(lc)) {
        snap.local_controller = lc;
        uint32_t lh = read<uint32_t>(lc + NetVars::m_hPawn);
        if (lh) {
            snap.local_pawn = get_entity_from_handle(lh);
            if (IsRemotePtrValid(snap.local_pawn)) {
                snap.local_team = read<uint8_t>(snap.local_pawn + NetVars::m_iTeamNum);
                snap.local_origin      = read<Vector3>(snap.local_pawn + NetVars::m_vOldOrigin);
                snap.local_velocity    = read<Vector3>(snap.local_pawn + NetVars::m_vecVelocity);
                snap.local_view_offset = read<Vector3>(snap.local_pawn + NetVars::m_vecViewOffset);
                snap.local_flash_duration = read<float>(snap.local_pawn + NetVars::m_flFlashDuration);
            }
        }
    }

    // ── Entity list base ───────────────────────────────────────
    uintptr_t elb = read<uintptr_t>(g_offsets.dwEntityList);
    uintptr_t local_pawn = snap.local_pawn;

    // ── Scan entities 1..63 ────────────────────────────────────
    if (IsRemotePtrValid(elb)) {
    std::unordered_map<uintptr_t, bool> seen;
    for (int i = 1; i < 64; ++i) {
        uintptr_t ch = read<uintptr_t>(elb + 8 * (i >> 9) + 0x10);
        if (!IsRemotePtrValid(ch)) continue;

        uintptr_t controller = read<uintptr_t>(ch + 112 * (i & 0x1FF));
        if (!IsRemotePtrValid(controller)) continue;

        // Track local controller index for spotted-by-mask bit check
        if (controller == snap.local_controller) {
            snap.local_controller_index = i;
            continue;
        }

        // Batch read controller: pawnHandle + name
        uint8_t ctrlBuf[0x60];
        if (!read(controller + 0x6BC, ctrlBuf, 0x58)) continue;
        uint32_t ph = *(uint32_t*)(ctrlBuf + 0x00);
        if (!ph) continue;

        // Resolve pawn via entity list
        uintptr_t pawn = 0;
        {
            uint32_t pIdx = ph & 0x7FFF;
            uintptr_t pChunk = read<uintptr_t>(elb + 8 * (pIdx >> 9) + 0x10);
            if (IsRemotePtrValid(pChunk))
                pawn = read<uintptr_t>(pChunk + 112 * (pIdx & 0x1FF));
        }
        if (!IsRemotePtrValid(pawn) || pawn == local_pawn) continue;

        // Batch read pawn core (0x330 ~ 0x400)
        uint8_t pc[0xD0];
        if (!read(pawn + 0x330, pc, 0xD0)) continue;
        uintptr_t sceneNode = *(uintptr_t*)(pc + 0x00);
        int32_t   health    = *(int32_t*)(pc + 0x1C);
        uint8_t   life      = *(uint8_t*)(pc + 0x24);
        uint8_t   team      = *(uint8_t*)(pc + 0xBB);
        bool alive = (health > 0 && health <= 200 && life == 0);

        CachedEntity ent;
        ent.pawn       = pawn;
        ent.controller = controller;
        ent.health     = health;
        ent.team       = team;
        ent.alive      = alive;
        ent.dormant    = true;
        if (IsRemotePtrValid(sceneNode))
            ent.dormant = read<uint8_t>(sceneNode + 0x103) != 0;

        if (alive) {
            ent.origin      = read<Vector3>(pawn + NetVars::m_vOldOrigin);
            ent.velocity    = read<Vector3>(pawn + NetVars::m_vecVelocity);
            ent.view_offset = read<Vector3>(pawn + NetVars::m_vecViewOffset);
            ent.flash_duration = read<float>(pawn + NetVars::m_flFlashDuration);
            uintptr_t ss = pawn + NetVars::m_entitySpottedState;
            ent.spotted          = read<uint8_t>(ss + NetVars::m_bSpotted) != 0;
            ent.spotted_by_mask = read<uint32_t>(ss + NetVars::m_bSpottedByMask);
            ent.weapon_services    = read<uintptr_t>(pawn + NetVars::m_pWeaponServices);
            ent.aim_punch_services = read<uintptr_t>(pawn + NetVars::m_pAimPunchServices);
            ent.movement_services  = read<uintptr_t>(pawn + NetVars::m_pMovementServices);

            // Tier 2: bones (every ~3 scans)
            if (tier2 && IsRemotePtrValid(sceneNode)) {
                uintptr_t ba = read<uintptr_t>(sceneNode + NetVars::m_modelState + NetVars::m_pBones);
                if (IsRemotePtrValid(ba)) {
                    uint8_t raw[30 * 0x20];
                    if (read(ba, raw, sizeof(raw))) {
                        ent.bone_count = 30;
                        for (int b = 0; b < 30; ++b) {
                            float* pf = (float*)(raw + b * 0x20);
                            ent.bones[b] = Vector3(pf[0], pf[1], pf[2]);
                        }
                        s_bone_pool[pawn] = {};
                        memcpy(s_bone_pool[pawn].bones, ent.bones, sizeof(ent.bones));
                        s_bone_pool[pawn].count = 30;
                    }
                }
            } else {
                auto bit = s_bone_pool.find(pawn);
                if (bit != s_bone_pool.end()) {
                    memcpy(ent.bones, bit->second.bones, sizeof(Vector3) * bit->second.count);
                    ent.bone_count = bit->second.count;
                }
            }

            if (ent.bone_count > 0 && BoneIndex::HEAD < ent.bone_count &&
                ent.bones[BoneIndex::HEAD].length() > 1.0f)
                ent.head_pos = ent.bones[BoneIndex::HEAD];
            else
                ent.head_pos = ent.origin + Vector3(0, 0, 72.0f);
        } // end if (alive)

        // ── Observer services (read for ALL, alive or dead — needed by spectator) ──
        // Tier 2 refresh, cached between frames
        {
            auto oit = s_obs_cache.find(pawn);
            if (tier2) {
                uintptr_t obs = read<uintptr_t>(pawn + NetVars::m_pObserverServices);
                if (IsRemotePtrValid(obs)) {
                    ent.observer_services = obs;
                    ent.observer_target   = read<uint32_t>(obs + NetVars::m_hObserverTarget);
                    s_obs_cache[pawn] = {obs, ent.observer_target};
                }
            } else if (oit != s_obs_cache.end()) {
                ent.observer_services = oit->second.services;
                ent.observer_target   = oit->second.target;
            }
        }

        const char* ns = (const char*)(ctrlBuf + 0x38);
        size_t nl = strnlen(ns, sizeof(ent.name) - 1);
        if (nl > 0) { memcpy(ent.name, ns, nl); ent.name[nl] = 0; }

        snap.entities.push_back(std::move(ent));
        seen[pawn] = true;
    }

    // ── Prune dead entities from bone pool ─────────────────────
    for (auto it = s_bone_pool.begin(); it != s_bone_pool.end(); )
        if (!seen.count(it->first)) it = s_bone_pool.erase(it); else ++it;
    for (auto it = s_obs_cache.begin(); it != s_obs_cache.end(); )
        if (!seen.count(it->first)) it = s_obs_cache.erase(it); else ++it;
    } // end if (elb)

    // ── Swap into cache ────────────────────────────────────
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_cache = std::move(snap);
    }
    g_version.fetch_add(1, std::memory_order_release);
}

CacheSnapshot fetch() {
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_cache;
}

} // namespace cs2::entity_cache

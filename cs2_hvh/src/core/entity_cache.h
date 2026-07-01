#pragma once
#include <cstdint>
#include <vector>
#include <atomic>
#include "../utils/sdk.h"

namespace cs2::entity_cache {

// Per-entity snapshot — everything any feature could need
struct CachedEntity {
    uintptr_t pawn = 0;          // C_CSPlayerPawn*
    uintptr_t controller = 0;    // CCSPlayerController*
    Vector3 origin;
    Vector3 velocity;
    Vector3 view_offset;         // m_vecViewOffset
    Vector3 head_pos;            // from bones (or origin+72 fallback)
    int    health = 0;
    int    team = 0;
    bool   alive = false;        // health > 0 && lifeState == 0
    bool   dormant = true;       // sceneNode + 0x103
    bool   spotted = false;      // EntitySpottedState_t::m_bSpotted
    float  flash_duration = 0;   // m_flFlashDuration

    // Skeleton (world-space positions, 30 bones max)
    Vector3 bones[30];
    int     bone_count = 0;

    // Component pointers (used by aimbot)
    uintptr_t weapon_services = 0;      // m_pWeaponServices
    uintptr_t aim_punch_services = 0;   // m_pAimPunchServices
    uintptr_t movement_services = 0;    // m_pMovementServices

    // Observer (for spectator list)
    uintptr_t observer_services = 0;
    uint32_t  observer_target = 0;

    // Name (char[32] — m_iszPlayerName at 0x6F4)
    char name[32]{};
};

// Full frame snapshot
struct CacheSnapshot {
    std::vector<CachedEntity> entities;
    uintptr_t local_pawn = 0;
    uintptr_t local_controller = 0;
    int       local_team = 0;
    Vector3   local_origin;
    Vector3   local_velocity;
    Vector3   local_view_offset;
    float     local_flash_duration = 0;

    // Quick helpers
    const CachedEntity* find_by_pawn(uintptr_t p) const {
        for (auto& e : entities)
            if (e.pawn == p) return &e;
        return nullptr;
    }
};

// Called every game-loop iteration (1000Hz) — populates the cache
void update();

// Thread-safe copy of the latest snapshot (render thread, 144Hz)
CacheSnapshot fetch();

// Version bumps on each write; render thread can compare to skip copies
inline std::atomic<uint64_t> g_version{0};

} // namespace cs2::entity_cache

#pragma once
#include <cstdint>
#include "memory.h"

namespace cs2::offsets {

// Aligned with https://github.com/a2x/cs2-dumper/tree/main/output
// All offsets are RVAs from client.dll base and pattern-scanned at runtime.
extern struct Offsets {
    // ===== client.dll globals (pattern-scanned) =====
    uintptr_t dwEntityList = 0;              // entity list pointer
    uintptr_t dwLocalPlayerController = 0;   // CCSPlayerController (persists across respawns)
    uintptr_t dwLocalPlayerPawn = 0;         // C_CSPlayerPawn (direct pawn pointer)
    uintptr_t dwViewMatrix = 0;              // 4x4 view-projection matrix
    uintptr_t dwViewAngles = 0;              // camera view angles pointer
    uintptr_t dwGlobalVars = 0;              // CGlobalVarsBase
    uintptr_t dwGameEntitySystem = 0;        // CGameEntitySystem (entity iteration)
    uintptr_t dwPlantedC4 = 0;              // C_PlantedC4 pointer
    uintptr_t dwCSGOInput = 0;              // CCSGOInput
    uintptr_t dwBuildNumber = 0;             // game build number
    uintptr_t dwGameRules = 0;              // C_CSGameRules

    // ===== Module info =====
    uintptr_t clientBase = 0;
    uintptr_t clientSize = 0;
} g_offsets;

// Pattern scan helpers
uintptr_t pattern_scan(const uint8_t* data, size_t size, const char* signature);
uintptr_t pattern_scan_module(uintptr_t base, size_t size, const char* signature);
uintptr_t resolve_rip(uintptr_t addr, uint32_t offset, uint32_t instr_size = 7);

// Scan all required offsets from the game module
bool scan_offsets();

// ── Entity list traversal (CS2 external: 3-level indirection) ─────
// dwEntityList → [read] → entListBase → +8*chunk+0x10 → [read] → chunkPtr → +112*slot → [read] → entity
inline uintptr_t get_entity_from_index(int index) {
    uintptr_t entListBase = cs2::memory::read<uintptr_t>(g_offsets.dwEntityList);
    if (!cs2::memory::IsRemotePtrValid(entListBase)) return 0;

    uintptr_t chunkPtr = cs2::memory::read<uintptr_t>(entListBase + 8 * (index >> 9) + 0x10);
    if (!cs2::memory::IsRemotePtrValid(chunkPtr)) return 0;

    uintptr_t entity = cs2::memory::read<uintptr_t>(chunkPtr + 112 * (index & 0x1FF));
    if (!cs2::memory::IsRemotePtrValid(entity)) return 0;

    return entity;
}

inline uintptr_t get_entity_from_handle(uint32_t handle) {
    if (!handle) return 0;
    return get_entity_from_index(handle & 0x7FFF);
}

} // namespace cs2::offsets

#pragma once
#include <cstdint>

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

} // namespace cs2::offsets

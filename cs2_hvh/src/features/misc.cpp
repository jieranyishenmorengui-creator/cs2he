#include "misc.h"
#include "../utils/sdk.h"
#include "../core/overlay.h"
#include "../core/memory.h"
#include "../core/offsets.h"
#include <chrono>

namespace cs2::misc {

using namespace ::cs2::memory;
using namespace ::cs2::offsets;

static auto last_frame = std::chrono::high_resolution_clock::now();

void run(const MiscConfig& cfg) {
    // FPS limiter
    if (cfg.max_fps > 0) {
        auto now = std::chrono::high_resolution_clock::now();
        float elapsed = std::chrono::duration<float>(now - last_frame).count();
        float target_ms = 1000.0f / cfg.max_fps;
        if (elapsed < target_ms / 1000.0f) {
            Sleep((DWORD)((target_ms / 1000.0f - elapsed) * 1000.0f));
        }
        last_frame = std::chrono::high_resolution_clock::now();
    }

    // Auto jump (bunny hop)
    if (cfg.auto_jump && overlay::is_key_down(cfg.auto_jump_key)) {
        uintptr_t local_controller = read<uintptr_t>(g_offsets.dwLocalPlayerController);
        if (!IsRemotePtrValid(local_controller)) return;

        uint32_t pawn_handle = read<uint32_t>(local_controller + NetVars::m_hPlayerPawn);
        if (!pawn_handle) return;

        uintptr_t pawn = get_entity_from_handle(pawn_handle);
        if (!IsRemotePtrValid(pawn)) return;

        uint32_t flags = read<uint32_t>(pawn + NetVars::m_fFlags);
        if (flags & 1) { // FL_ONGROUND
            uintptr_t move_svc = read<uintptr_t>(pawn + NetVars::m_pMovementServices);
            if (IsRemotePtrValid(move_svc)) {
                uint32_t buttons = read<uint32_t>(move_svc + NetVars::m_nButtons);
                buttons |= (1 << 1); // IN_JUMP
                write<uint32_t>(move_svc + NetVars::m_nButtons, buttons);
            }
        }
    }
}

} // namespace cs2::misc

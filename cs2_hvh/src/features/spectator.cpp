#include "spectator.h"
#include "../core/memory.h"
#include "../core/offsets.h"
#include "../utils/sdk.h"
#include "../imgui/imgui.h"
#include "../core/overlay.h"
#include <string>
#include <vector>
#include <chrono>

namespace cs2::spectator {

using namespace ::cs2::memory;
using namespace ::cs2::offsets;

static std::vector<std::string> s_spectators;
static std::chrono::steady_clock::time_point s_last_update;
static constexpr int UPDATE_INTERVAL_MS = 250;

void update() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_last_update).count();
    if (elapsed < UPDATE_INTERVAL_MS) return;
    s_last_update = now;

    s_spectators.clear();

    uintptr_t ctrl = read<uintptr_t>(g_offsets.dwLocalPlayerController);
    if (!IsRemotePtrValid(ctrl)) return;
    uintptr_t local_pawn = get_entity_from_handle(read<uint32_t>(ctrl + NetVars::m_hPawn));
    if (!IsRemotePtrValid(local_pawn)) return;

    uintptr_t elb = read<uintptr_t>(g_offsets.dwEntityList);
    if (!IsRemotePtrValid(elb)) return;

    for (int i = 1; i < 64; ++i) {
        uintptr_t ch = read<uintptr_t>(elb + 8 * (i >> 9) + 0x10);
        if (!IsRemotePtrValid(ch)) continue;
        uintptr_t entity = read<uintptr_t>(ch + 112 * (i & 0x1FF));
        if (!IsRemotePtrValid(entity) || entity == ctrl) continue;

        uint32_t ph = read<uint32_t>(entity + NetVars::m_hPawn);
        if (!ph) continue;
        uintptr_t pawn = get_entity_from_handle(ph);
        if (!IsRemotePtrValid(pawn)) continue;

        int hp = read<int32_t>(pawn + NetVars::m_iHealth);
        if (hp > 0 && read<uint8_t>(pawn + NetVars::m_lifeState) == 0) continue;

        uintptr_t obs = read<uintptr_t>(pawn + NetVars::m_pObserverServices);
        if (!IsRemotePtrValid(obs)) continue;
        uint32_t target_handle = read<uint32_t>(obs + NetVars::m_hObserverTarget);
        if (!target_handle) continue;

        uintptr_t target_pawn = get_entity_from_handle(target_handle);
        if (target_pawn == local_pawn) {
            char name[128] = {};
            read(entity + NetVars::m_iszPlayerName, name, sizeof(name));
            name[sizeof(name) - 1] = 0;
            if (name[0]) s_spectators.emplace_back(name);
        }
    }
}

void draw() {
    if (s_spectators.empty()) return;

    int sw = overlay::get_width();
    int sh = overlay::get_height();

    ImGui::SetNextWindowPos(ImVec2((float)sw - 220.f, (float)sh * 0.3f), ImGuiCond_Once);
    ImGui::SetNextWindowBgAlpha(0.75f);
    ImGui::Begin("##spectators", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);

    if (s_spectators.size() == 1)
        ImGui::TextColored(ImVec4(1,1,0,1), "1 spectator");
    else
        ImGui::TextColored(ImVec4(1,1,0,1), "%zu spectators", s_spectators.size());

    ImGui::Separator();
    for (const auto& name : s_spectators) {
        ImGui::Text("%s", name.c_str());
    }
    ImGui::End();
}

} // namespace cs2::spectator

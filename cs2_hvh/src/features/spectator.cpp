#include "spectator.h"
#include "../core/entity_cache.h"
#include "../core/memory.h"
#include "../core/offsets.h"
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

    auto snap = entity_cache::fetch();
    uintptr_t local_pawn = snap.local_pawn;
    if (!IsRemotePtrValid(local_pawn)) return;

    for (auto& ent : snap.entities) {
        if (ent.alive) continue; // only dead players spectate
        if (!ent.observer_services) continue;
        if (ent.observer_target == 0) continue;

        uintptr_t target_pawn = get_entity_from_handle(ent.observer_target);
        if (target_pawn == local_pawn) {
            const char* name = ent.name;
            if (name[0])
                s_spectators.emplace_back(name);
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

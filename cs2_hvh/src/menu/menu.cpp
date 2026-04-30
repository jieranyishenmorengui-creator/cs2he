#include "menu.h"
#include "../core/overlay.h"
#include "../config/config.h"
#include "../imgui/imgui.h"
#include <cstdio>

namespace cs2::menu {

static bool g_open = false;

bool is_open() { return g_open; }
void set_open(bool open) {
    g_open = open;
    overlay::set_menu_open(open);
}
void toggle() { set_open(!g_open); }

// ── Key binding helper ──────────────────────────────────────────
static int g_key_bind_target = -1;   // which bind is listening, -1 = none

static const char* key_name(int key) {
    switch (key) {
    case 0:           return "None";
    case VK_LBUTTON:  return "LMB";
    case VK_RBUTTON:  return "RMB";
    case VK_MBUTTON:  return "M3";
    case VK_XBUTTON1: return "M4";
    case VK_XBUTTON2: return "M5";
    case VK_SHIFT:    return "Shift";
    case VK_CONTROL:  return "Ctrl";
    case VK_MENU:     return "Alt";
    case VK_SPACE:    return "Space";
    case VK_RETURN:   return "Enter";
    case VK_TAB:      return "Tab";
    case VK_CAPITAL:  return "Caps";
    case VK_ESCAPE:   return "Esc";
    case VK_INSERT:   return "Insert";
    case VK_DELETE:   return "Delete";
    case VK_HOME:     return "Home";
    case VK_END:      return "End";
    case VK_PRIOR:    return "PgUp";
    case VK_NEXT:     return "PgDn";
    case VK_UP:       return "Up";
    case VK_DOWN:     return "Down";
    case VK_LEFT:     return "Left";
    case VK_RIGHT:    return "Right";
    default:
        if (key >= VK_F1 && key <= VK_F12) {
            static char buf[8];
            snprintf(buf, sizeof(buf), "F%d", key - VK_F1 + 1);
            return buf;
        }
        if (key >= 'A' && key <= 'Z') { static char b[2]{}; b[0] = (char)key; return b; }
        if (key >= '0' && key <= '9') { static char b[2]{}; b[0] = (char)key; return b; }
        return "?";
    }
}

static void key_bind_widget(const char* label, int* key, int bind_id) {
    if (g_key_bind_target == bind_id) {
        // Listening for a key press
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.1f, 0.1f, 0.9f));
        if (ImGui::Button("[ ... ]", ImVec2(-1, 0))) {
            g_key_bind_target = -1;
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextUnformatted(label);

        // Capture next key
        for (int k = 0; k < 256; k++) {
            if (overlay::was_key_pressed(k)) {
                if (k == VK_ESCAPE) {
                    g_key_bind_target = -1;  // cancel
                } else {
                    *key = k;
                    g_key_bind_target = -1;
                }
                break;
            }
        }
    } else {
        char buf[64];
        snprintf(buf, sizeof(buf), "[ %s ]", key_name(*key));
        if (ImGui::Button(buf, ImVec2(-1, 0))) {
            g_key_bind_target = bind_id;
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(label);
    }
}

// ── Tab content ─────────────────────────────────────────────────

static void tab_aimbot() {
    auto& cfg = config::get().aimbot;

    ImGui::Checkbox("Enable Aimbot", &cfg.enabled);

    key_bind_widget("Aim Key 1", &cfg.key0, 0);
    key_bind_widget("Aim Key 2", &cfg.key1, 1);

    const char* key_modes[] = { "Hold", "Toggle", "Always" };
    ImGui::Combo("Key Mode", &cfg.key_mode, key_modes, 3);

    const char* aim_modes[] = { "Crosshair", "Distance", "FOV" };
    ImGui::Combo("Aim Mode", &cfg.mode, aim_modes, 3);

    const char* bones[] = { "Pelvis", "Spine", "Neck", "Head" };
    int bone_idx = (cfg.target_bone == 1) ? 0 : (cfg.target_bone == 3) ? 1
                  : (cfg.target_bone == 6) ? 2 : 3;
    if (ImGui::Combo("Target Bone", &bone_idx, bones, 4)) {
        constexpr int map[] = { 1, 3, 6, 7 };
        cfg.target_bone = map[bone_idx];
    }

    ImGui::SliderFloat("Aim FOV", &cfg.fov, 0.0f, 30.0f, "%.1f deg");
    ImGui::SliderFloat("Smooth", &cfg.smooth, 0.0f, 20.0f, "%.1f");
    ImGui::SliderFloat("Max Distance", &cfg.max_distance, 0.0f, 500.0f, "%.0f m");

    ImGui::Separator();
    ImGui::Checkbox("Visible Check", &cfg.visible_check);
    ImGui::Checkbox("Team Check", &cfg.team_check);
    ImGui::Checkbox("Show FOV Circle", &cfg.show_fov);
    ImGui::Checkbox("Rainbow FOV", &cfg.rainbow_fov);
    ImGui::Checkbox("Recoil Control", &cfg.recoil_control);

    if (cfg.recoil_control) {
        ImGui::Indent();
        ImGui::SliderFloat("RCS Scale", &cfg.rcs_scale, 0.0f, 1.0f, "%.2f");
        ImGui::Unindent();
    }
}

static void tab_visuals() {
    auto& cfg = config::get().esp;

    ImGui::Checkbox("Enable ESP", &cfg.enabled);
    ImGui::Checkbox("Team Check", &cfg.team_check);
    ImGui::Separator();

    ImGui::Checkbox("Show Box", &cfg.show_box);
    if (cfg.show_box) {
        ImGui::Indent();
        const char* box_types[] = { "Rectangle", "Corner", "3D" };
        ImGui::Combo("Box Type", &cfg.box_type, box_types, 3);
        ImGui::Checkbox("Filled Box", &cfg.filled);
        ImGui::Unindent();
    }

    ImGui::Checkbox("Show Line", &cfg.show_line);
    ImGui::Checkbox("Show Skeleton", &cfg.show_skeleton);
    ImGui::Checkbox("Show Health", &cfg.show_health);
    ImGui::Checkbox("Show Distance", &cfg.show_distance);
    ImGui::Checkbox("Show Name", &cfg.show_name);
    ImGui::Checkbox("Show Weapon", &cfg.show_weapon);

    ImGui::Separator();
    ImGui::SliderFloat("Max Distance", &cfg.max_distance, 0.0f, 500.0f, "%.0f m");
    ImGui::SliderFloat("Global Alpha", &cfg.global_alpha, 0.0f, 1.0f, "%.2f");

    ImGui::Separator();
    ImGui::TextUnformatted("Colors");
    ImGui::ColorEdit4("Team",      (float*)&cfg.team_color,   ImGuiColorEditFlags_NoInputs);
    ImGui::ColorEdit4("Enemy",     (float*)&cfg.enemy_color,  ImGuiColorEditFlags_NoInputs);
    ImGui::ColorEdit4("Aim Target", (float*)&cfg.aim_target_color, ImGuiColorEditFlags_NoInputs);
    ImGui::ColorEdit4("Shadow",    (float*)&cfg.shadow_color, ImGuiColorEditFlags_NoInputs);
}

static void tab_crosshair() {
    auto& cfg = config::get().crosshair;

    ImGui::Checkbox("Enable Crosshair", &cfg.enabled);

    const char* types[] = { "Cross", "Circle", "Dot" };
    ImGui::Combo("Type", &cfg.type, types, 3);

    ImGui::SliderFloat("Size", &cfg.size, 1.0f, 50.0f, "%.0f px");
    ImGui::SliderFloat("Thickness", &cfg.thickness, 0.5f, 5.0f, "%.1f");

    ImGui::ColorEdit4("Color", (float*)&cfg.color, ImGuiColorEditFlags_NoInputs);
}

static void tab_misc() {
    auto& cfg = config::get().misc;

    ImGui::SliderInt("Max FPS", &cfg.max_fps, 30, 500);
    ImGui::Checkbox("Auto Jump (Bunny Hop)", &cfg.auto_jump);
}

static void tab_config() {
    auto& cfg = config::get();

    if (ImGui::Button("Save Config", ImVec2(140, 28))) {
        cfg.save("config.json");
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Config", ImVec2(140, 28))) {
        cfg.load("config.json");
    }

    ImGui::Separator();
    ImGui::Text("Menu Key:  INSERT");
    ImGui::Text("Panic Key: END  (unloads the cheat)");
    ImGui::Separator();
    ImGui::TextDisabled("Config: config.json");
}

// ── Public API ──────────────────────────────────────────────────

void render() {
    if (!g_open || !overlay::is_ready()) return;

    int sw = overlay::get_width();
    int sh = overlay::get_height();

    ImGui::SetNextWindowPos(ImVec2((float)sw * 0.5f, (float)sh * 0.5f),
                            ImGuiCond_Once, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(560, 420), ImGuiCond_Once);

    ImGui::Begin("CS2 HvH", nullptr,
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoCollapse);

    if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Aimbot")) {
            if (ImGui::BeginChild("##aimbot", ImVec2(0, 0), false))
                tab_aimbot();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Visuals")) {
            if (ImGui::BeginChild("##visuals", ImVec2(0, 0), false))
                tab_visuals();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Crosshair")) {
            if (ImGui::BeginChild("##crosshair", ImVec2(0, 0), false))
                tab_crosshair();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Misc")) {
            if (ImGui::BeginChild("##misc", ImVec2(0, 0), false))
                tab_misc();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Config")) {
            if (ImGui::BeginChild("##config", ImVec2(0, 0), false))
                tab_config();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

} // namespace cs2::menu

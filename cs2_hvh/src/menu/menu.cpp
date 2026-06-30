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
    auto& cfg_all = config::get();
    auto& cfg = cfg_all.aimbot;

    ImGui::Checkbox("Enable Aimbot", &cfg.enabled);
    key_bind_widget("Toggle Aimbot", &cfg_all.aimbot_toggle_key, 11);

    ImGui::SeparatorText("Controls");
    key_bind_widget("Aim Key 1", &cfg.key0, 0);
    key_bind_widget("Aim Key 2", &cfg.key1, 1);

    const char* key_modes[] = { "Hold", "Toggle", "Always" };
    ImGui::Combo("Key Mode", &cfg.key_mode, key_modes, 3);

    ImGui::SeparatorText("Input Method");
    const char* input_methods[] = { "Write Angles (reliable)", "SendInput Mouse (FutaZone)" };
    ImGui::Combo("Aim Input", &cfg.input_method, input_methods, 2);
    if (cfg.input_method == 0) {
        ImGui::TextDisabled("写 dwViewAngles — 不受 m_rawinput 影响");
    } else {
        ImGui::TextDisabled("模拟鼠标移动 — m_rawinput 1 时可能被忽略");
    }

    ImGui::SeparatorText("Targeting");
    const char* bones[] = { "Pelvis", "Spine", "Neck", "Head" };
    int bone_idx = (cfg.target_bone == 1) ? 0 : (cfg.target_bone == 3) ? 1
                  : (cfg.target_bone == 6) ? 2 : 3;
    if (ImGui::Combo("Target Bone", &bone_idx, bones, 4)) {
        constexpr int map[] = { 1, 3, 6, 7 };
        cfg.target_bone = map[bone_idx];
    }

    ImGui::SliderFloat("FOV (pixels)", &cfg.fov, 10.0f, 800.0f, "%.0f px");
    ImGui::SliderFloat("Smoothness", &cfg.smoothness, 0.5f, 20.0f, "%.1f");
    ImGui::SliderFloat("Max Distance", &cfg.max_distance, 0.0f, 500.0f, "%.0f m");

    ImGui::SeparatorText("Target Priority");
    const char* prio[] = { "FOV (crosshair)", "Distance (closest)", "Health (finish low HP)" };
    ImGui::Combo("Priority", &cfg.aim_priority, prio, 3);

    ImGui::Separator();
    ImGui::SliderInt("Kill Cooldown (ms)", &cfg.kill_delay_ms, 0, 3000, "%d ms");
    ImGui::SliderFloat("Velocity Lead (0=off)", &cfg.lead_time, 0.0f, 0.15f, "%.3f s");
    ImGui::Checkbox("Visible Check", &cfg.visible_check);

    ImGui::SeparatorText("Head Offset (side-facing fix)");
    ImGui::Checkbox("Enable Head Offset", &cfg.head_offset_enabled);
    ImGui::SliderFloat("Offset Amount", &cfg.head_offset_amount, 2.0f, 20.0f, "%.1f");
    ImGui::SliderFloat("Angle Min", &cfg.head_offset_angle_min, 30.0f, 90.0f, "%.0f deg");
    ImGui::SliderFloat("Angle Max", &cfg.head_offset_angle_max, 90.0f, 150.0f, "%.0f deg");
    ImGui::Checkbox("Team Check", &cfg.team_check);
    ImGui::Checkbox("Disable When Flashed", &cfg.disable_when_flashed);

    ImGui::SeparatorText("Advanced");
    ImGui::Checkbox("Hard Lock (锁定后不换目标)", &cfg.hard_lock);
    ImGui::TextDisabled("  锁定后新敌人出现不切换，只有目标死亡才解锁");
    if (cfg.visible_check) {
        ImGui::SliderInt("Spotted Timeout (ms)", &cfg.spotted_timeout_ms, 300, 5000, "%d ms");
        ImGui::TextDisabled("spotted后记忆时间，越长越宽松");
    }

    ImGui::Separator();
    ImGui::Checkbox("RCS Recoil Control (压枪)", &cfg.recoil_control);
    if (cfg.recoil_control) {
        ImGui::Indent();
        ImGui::SliderFloat("RCS Scale", &cfg.rcs_scale, 0.0f, 1.0f, "%.2f");
        ImGui::Unindent();
    }

    ImGui::SeparatorText("Aim Mode (FutaZone)");
    const char* aim_modes_list[] = { "Linear", "FastThenSlow", "SlowThenFast", "Overshoot", "Random" };
    ImGui::Combo("Aim Pattern", &cfg.aim_mode, aim_modes_list, 5);

    ImGui::Checkbox("Randomize Speed", &cfg.randomize_speed);
    ImGui::SliderInt("Phase Duration", &cfg.speed_change_duration, 100, 2000, "%d ms");
    ImGui::SliderFloat("Overshoot Scale", &cfg.overshoot_scale, 1.0f, 2.0f, "%.1f");
}

static void tab_visuals() {
    auto& cfg_all = config::get();
    auto& cfg = cfg_all.esp;

    ImGui::Checkbox("Enable ESP", &cfg.enabled);
    key_bind_widget("Toggle ESP", &cfg_all.esp_toggle_key, 10);
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
    ImGui::SliderFloat("Text Scale", &cfg.text_scale, 0.5f, 2.0f, "%.1f");
    ImGui::SliderFloat("Max Distance", &cfg.max_distance, 0.0f, 500.0f, "%.0f m");
    ImGui::SliderFloat("Global Alpha", &cfg.global_alpha, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Smooth Factor", &cfg.smooth_factor, 0.0f, 0.9f, "%.2f");

    ImGui::Checkbox("Head Circle", &cfg.show_head_circle);
    ImGui::SliderFloat("HC Thick", &cfg.head_circle_thickness, 0.5f, 5.0f, "%.1f");
    ImGui::SliderFloat("Skel Thick", &cfg.skeleton_thickness, 0.5f, 5.0f, "%.1f");

    ImGui::Separator();
    ImGui::TextUnformatted("Colors");
    ImGui::ColorEdit4("Team",       (float*)&cfg.team_color,        ImGuiColorEditFlags_NoInputs);
    ImGui::ColorEdit4("Enemy",      (float*)&cfg.enemy_color,       ImGuiColorEditFlags_NoInputs);
    ImGui::ColorEdit4("Aim Target", (float*)&cfg.aim_target_color,  ImGuiColorEditFlags_NoInputs);
    ImGui::ColorEdit4("Shadow",     (float*)&cfg.shadow_color,      ImGuiColorEditFlags_NoInputs);
    ImGui::ColorEdit4("Head Circ",  (float*)&cfg.head_circle_color, ImGuiColorEditFlags_NoInputs);
    ImGui::ColorEdit4("Skeleton",   (float*)&cfg.skeleton_color,    ImGuiColorEditFlags_NoInputs);
}

static void tab_crosshair() {
    auto& cfg_all = config::get();
    auto& cfg = cfg_all.crosshair;

    ImGui::Checkbox("Enable Crosshair", &cfg.enabled);
    key_bind_widget("Toggle Crosshair", &cfg_all.crosshair_toggle_key, 12);

    const char* types[] = { "Cross", "Circle", "Dot" };
    ImGui::Combo("Type", &cfg.type, types, 3);

    ImGui::SliderFloat("Size", &cfg.size, 1.0f, 50.0f, "%.0f px");
    ImGui::SliderFloat("Thickness", &cfg.thickness, 0.5f, 5.0f, "%.1f");

    ImGui::ColorEdit4("Color", (float*)&cfg.color, ImGuiColorEditFlags_NoInputs);
}

static void tab_triggerbot() {
    auto& cfg = config::get().triggerbot;

    ImGui::Checkbox("Enable Triggerbot", &cfg.enabled);
    key_bind_widget("Active Key", &cfg.key, 20);

    const char* tmodes[] = { "m_iIDEntIndex", "FOV Angle (precise)" };
    ImGui::Combo("Detection Mode", &cfg.mode, tmodes, 2);
    if (cfg.mode == 1)
        ImGui::SliderFloat("FOV Threshold", &cfg.fov_threshold, 0.5f, 10.0f, "%.1f deg");

    ImGui::SliderInt("Delay Min", &cfg.delay_min, 0, 200);
    ImGui::SliderInt("Delay Max", &cfg.delay_max, 0, 200);
    ImGui::Checkbox("Team Check", &cfg.team_check);
    ImGui::SliderFloat("Max Velocity", &cfg.max_velocity, 0.0f, 50.0f, "%.0f u/s");
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

// ── Menu window rect (for hit-test passthrough) ─────────────────
static float g_menuX = 0, g_menuY = 0, g_menuW = 0, g_menuH = 0;

// ── Public API ──────────────────────────────────────────────────

bool is_point_over(float cx, float cy) {
    if (!g_open) return false;
    return cx >= g_menuX && cx <= g_menuX + g_menuW &&
           cy >= g_menuY && cy <= g_menuY + g_menuH;
}

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

    // Save menu rect for hit-test (used by overlay WM_NCHITTEST)
    {
        ImVec2 p = ImGui::GetWindowPos();
        ImVec2 s = ImGui::GetWindowSize();
        g_menuX = p.x; g_menuY = p.y; g_menuW = s.x; g_menuH = s.y;
    }

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
        if (ImGui::BeginTabItem("Triggerbot")) {
            if (ImGui::BeginChild("##trig", ImVec2(0, 0), false))
                tab_triggerbot();
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

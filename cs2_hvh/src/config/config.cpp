#include "config.h"
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace cs2::config {

Config g_config;

// Minimal JSON writer
static void write_json_string(FILE* f, const char* key, const char* val) {
    fprintf(f, "  \"%s\": \"%s\"", key, val);
}

static void write_json_bool(FILE* f, const char* key, bool val) {
    fprintf(f, "  \"%s\": %s", key, val ? "true" : "false");
}

static void write_json_int(FILE* f, const char* key, int val) {
    fprintf(f, "  \"%s\": %d", key, val);
}

static void write_json_float(FILE* f, const char* key, float val) {
    fprintf(f, "  \"%s\": %.4f", key, val);
}

static void write_json_color(FILE* f, const char* key, const Color& c) {
    fprintf(f, "  \"%s\": [%.4f, %.4f, %.4f, %.4f]", key, c.r, c.g, c.b, c.a);
}

// Minimal JSON parser helpers
static const char* json_find(const char* json, const char* key) {
    static char buf[4096];
    snprintf(buf, sizeof(buf), "\"%s\"", key);
    const char* p = strstr(json, buf);
    return p;
}

static bool json_get_bool(const char* json, const char* key, bool def = false) {
    const char* p = json_find(json, key);
    if (!p) return def;
    p = strchr(p, ':');
    if (!p) return def;
    while (*p && (*p == ':' || *p == ' ' || *p == '\t')) ++p;
    return strncmp(p, "true", 4) == 0;
}

static int json_get_int(const char* json, const char* key, int def = 0) {
    const char* p = json_find(json, key);
    if (!p) return def;
    p = strchr(p, ':');
    if (!p) return def;
    return atoi(p + 1);
}

static float json_get_float(const char* json, const char* key, float def = 0.0f) {
    const char* p = json_find(json, key);
    if (!p) return def;
    p = strchr(p, ':');
    if (!p) return def;
    return (float)atof(p + 1);
}

static Color json_get_color(const char* json, const char* key, const Color& def = Color()) {
    const char* p = json_find(json, key);
    if (!p) return def;
    p = strchr(p, '[');
    if (!p) return def;
    float v[4];
    for (int i = 0; i < 4; ++i) {
        v[i] = (float)atof(p + 1);
        p = strchr(p + 1, ',');
        if (!p && i < 3) return def;
    }
    return Color(v[0], v[1], v[2], v[3]);
}

static std::string json_get_string(const char* json, const char* key, const std::string& def = "") {
    const char* p = json_find(json, key);
    if (!p) return def;
    p = strchr(p, '"');
    if (!p) return def;
    p = strchr(p + 1, '"');
    if (!p) return def;
    const char* start = p + 1;
    p = strchr(start, '"');
    if (!p) return def;
    return std::string(start, p - start);
}

static char* read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return nullptr;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = new char[sz + 1];
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

bool Config::load(const std::string& path) {
    char* json = read_file(path.c_str());
    if (!json) return false;

    // Menu
    menu_key = json_get_int(json, "MenuKey", VK_INSERT);
    panic_key = json_get_int(json, "PanicKey", VK_END);

    // Toggle keys
    esp_toggle_key = json_get_int(json, "ESPToggleKey", 0);
    aimbot_toggle_key = json_get_int(json, "AimbotToggleKey", 0);
    crosshair_toggle_key = json_get_int(json, "CrosshairToggleKey", 0);

    // Aimbot
    aimbot.enabled = json_get_bool(json, "AimBotEnable", true);
    aimbot.key0 = json_get_int(json, "AimKey0", 0x06);
    aimbot.key1 = json_get_int(json, "AimKey1", 0);
    aimbot.key_mode = json_get_int(json, "AimKeyMode", 0);
    aimbot.target_bone = json_get_int(json, "AimTargetBone", 7);
    aimbot.fov = json_get_float(json, "AimFOV_Pixels", 800.0f);
    aimbot.smoothness = json_get_float(json, "AimSmoothness", 1.0f);
    aimbot.max_distance = json_get_float(json, "AimMaxDistance", 0.0f);
    aimbot.visible_check = json_get_bool(json, "AimVisibleCheck", false);
    aimbot.team_check = json_get_bool(json, "AimTeamCheck", true);
    aimbot.disable_when_flashed = json_get_bool(json, "AimDisableFlashed", true);
    aimbot.flash_threshold = json_get_float(json, "AimFlashThreshold", 5.0f);
    aimbot.lead_time = json_get_float(json, "AimLeadTime", 0.051f);
    aimbot.aim_priority = json_get_int(json, "AimPriority", 0);
    aimbot.kill_delay_ms = json_get_int(json, "AimKillDelay", 0);
    aimbot.input_method = json_get_int(json, "AimInputMethod", 0);
    aimbot.recoil_control = json_get_bool(json, "AimRCS", true);
    aimbot.rcs_scale = json_get_float(json, "AimRCS_Scale", 0.54f);
    aimbot.aim_mode = json_get_int(json, "AimMode", 0);
    aimbot.randomize_speed = json_get_bool(json, "AimRandomSpeed", false);
    aimbot.speed_change_duration = json_get_int(json, "AimSpeedDuration", 500);
    aimbot.overshoot_scale = json_get_float(json, "AimOvershootScale", 1.2f);
    aimbot.head_offset_enabled = json_get_bool(json, "AimHeadOffset", true);
    aimbot.head_offset_amount = json_get_float(json, "AimHeadOffsetAmt", 3.6f);
    aimbot.head_offset_angle_min = json_get_float(json, "AimHeadOffMinAng", 45.0f);
    aimbot.head_offset_angle_max = json_get_float(json, "AimHeadOffMaxAng", 135.0f);

    // VisCheck
    vis_map = json_get_string(json, "VisMap");
    printf("[VisCheck] Config vis_map = '%s'\n", vis_map.c_str());

    // ESP
    esp.enabled = json_get_bool(json, "VisualEnable", true);
    esp.team_check = json_get_bool(json, "ESP_Team", true);
    esp.show_box = json_get_bool(json, "bBox", true);
    esp.box_type = json_get_int(json, "ESP_BoxType", 0);
    esp.filled = json_get_bool(json, "bFilled", false);
    esp.show_line = json_get_bool(json, "bLine", false);
    esp.show_skeleton = json_get_bool(json, "bSkeleton", false);
    esp.show_health = json_get_bool(json, "bHealth", true);
    esp.show_distance = json_get_bool(json, "bDistance", true);
    esp.show_name = json_get_bool(json, "bName", true);
    esp.show_weapon = json_get_bool(json, "bWeapon", false);
    esp.max_distance = json_get_float(json, "ESP_MaxDistance", 200.0f);
    esp.global_alpha = json_get_float(json, "GlobalAlpha", 1.0f);
    esp.shadow_alpha = json_get_float(json, "ShadowAlpha", 0.5f);
    esp.smooth_factor = json_get_float(json, "SmoothFactor", 0.0f);
    esp.text_scale = json_get_float(json, "ESP_TextScale", 1.0f);
    esp.show_head_circle = json_get_bool(json, "ShowHeadCircle", true);
    esp.head_circle_thickness = json_get_float(json, "HeadCircleThickness", 1.5f);
    esp.head_circle_color = json_get_color(json, "Col_HeadCircle", Color(1,1,1,1));
    esp.skeleton_thickness = json_get_float(json, "SkeletonThickness", 1.5f);
    esp.skeleton_color = json_get_color(json, "Col_Skeleton", Color(1,1,1,0.6f));
    esp.team_color = json_get_color(json, "Col_ESP_Team", Color(0.2f, 0.8f, 1.0f, 1.0f));
    esp.enemy_color = json_get_color(json, "Col_ESP_Enemy", Color(1.0f, 0.3f, 0.2f, 1.0f));
    esp.aim_target_color = json_get_color(json, "Col_ESP_AimTarget", Color(1.0f, 1.0f, 0.0f, 1.0f));
    esp.shadow_color = json_get_color(json, "Col_ESP_Shadow", Color(0, 0, 0, 0.5f));

    // Crosshair
    crosshair.enabled = json_get_bool(json, "CrosshairEnable", false);
    crosshair.type = json_get_int(json, "CrosshairType", 0);
    crosshair.size = json_get_float(json, "CrosshairSize", 10.0f);

    // Triggerbot
    triggerbot.enabled = json_get_bool(json, "TrigEnable", true);
    triggerbot.key = json_get_int(json, "TrigKey", 0x4C);
    triggerbot.delay_min = json_get_int(json, "TrigDelayMin", 10);
    triggerbot.delay_max = json_get_int(json, "TrigDelayMax", 25);
    triggerbot.team_check = json_get_bool(json, "TrigTeamCheck", false);
    triggerbot.max_velocity = json_get_float(json, "TrigMaxVelocity", 18.0f);
    triggerbot.mode = json_get_int(json, "TrigMode", 1);
    triggerbot.fov_threshold = json_get_float(json, "TrigFovThresh", 1.5f);

    // Misc
    misc.max_fps = json_get_int(json, "MaxFramerate", 144);

    delete[] json;
    return true;
}

bool Config::save(const std::string& path) {
    FILE* f = fopen(path.c_str(), "w");
    if (!f) return false;

    fprintf(f, "{\n");

    // Menu
    write_json_int(f, "MenuKey", menu_key);
    fprintf(f, ",\n");
    write_json_int(f, "PanicKey", panic_key);
    fprintf(f, ",\n");

    // Toggle keys
    write_json_int(f, "ESPToggleKey", esp_toggle_key);
    fprintf(f, ",\n");
    write_json_int(f, "AimbotToggleKey", aimbot_toggle_key);
    fprintf(f, ",\n");
    write_json_int(f, "CrosshairToggleKey", crosshair_toggle_key);
    fprintf(f, ",\n");

    // Aimbot
    write_json_bool(f, "AimBotEnable", aimbot.enabled);
    fprintf(f, ",\n");
    write_json_int(f, "AimKey0", aimbot.key0);
    fprintf(f, ",\n");
    write_json_int(f, "AimKey1", aimbot.key1);
    fprintf(f, ",\n");
    write_json_int(f, "AimKeyMode", aimbot.key_mode);
    fprintf(f, ",\n");
    write_json_int(f, "AimTargetBone", aimbot.target_bone);
    fprintf(f, ",\n");
    write_json_float(f, "AimFOV_Pixels", aimbot.fov);
    fprintf(f, ",\n");
    write_json_float(f, "AimSmoothness", aimbot.smoothness);
    fprintf(f, ",\n");
    write_json_float(f, "AimMaxDistance", aimbot.max_distance);
    fprintf(f, ",\n");
    write_json_bool(f, "AimVisibleCheck", aimbot.visible_check);
    fprintf(f, ",\n");
    write_json_bool(f, "AimTeamCheck", aimbot.team_check);
    fprintf(f, ",\n");
    write_json_bool(f, "AimDisableFlashed", aimbot.disable_when_flashed);
    fprintf(f, ",\n");
    write_json_float(f, "AimFlashThreshold", aimbot.flash_threshold);
    fprintf(f, ",\n");
    write_json_float(f, "AimLeadTime", aimbot.lead_time);
    fprintf(f, ",\n");
    write_json_int(f, "AimPriority", aimbot.aim_priority);
    fprintf(f, ",\n");
    write_json_int(f, "AimKillDelay", aimbot.kill_delay_ms);
    fprintf(f, ",\n");
    write_json_int(f, "AimInputMethod", aimbot.input_method);
    fprintf(f, ",\n");
    write_json_bool(f, "AimRCS", aimbot.recoil_control);
    fprintf(f, ",\n");
    write_json_float(f, "AimRCS_Scale", aimbot.rcs_scale);
    fprintf(f, ",\n");
    write_json_int(f, "AimMode", aimbot.aim_mode);
    fprintf(f, ",\n");
    write_json_bool(f, "AimRandomSpeed", aimbot.randomize_speed);
    fprintf(f, ",\n");
    write_json_int(f, "AimSpeedDuration", aimbot.speed_change_duration);
    fprintf(f, ",\n");
    write_json_float(f, "AimOvershootScale", aimbot.overshoot_scale);
    fprintf(f, ",\n");
    write_json_bool(f, "AimHeadOffset", aimbot.head_offset_enabled);
    fprintf(f, ",\n");
    write_json_float(f, "AimHeadOffsetAmt", aimbot.head_offset_amount);
    fprintf(f, ",\n");
    write_json_float(f, "AimHeadOffMinAng", aimbot.head_offset_angle_min);
    fprintf(f, ",\n");
    write_json_float(f, "AimHeadOffMaxAng", aimbot.head_offset_angle_max);
    fprintf(f, ",\n");

    // ESP
    write_json_string(f, "VisMap", vis_map.c_str());
    fprintf(f, ",\n");
    write_json_bool(f, "VisualEnable", esp.enabled);
    fprintf(f, ",\n");
    write_json_bool(f, "ESP_Team", esp.team_check);
    fprintf(f, ",\n");
    write_json_bool(f, "bBox", esp.show_box);
    fprintf(f, ",\n");
    write_json_int(f, "ESP_BoxType", esp.box_type);
    fprintf(f, ",\n");
    write_json_bool(f, "bFilled", esp.filled);
    fprintf(f, ",\n");
    write_json_bool(f, "bLine", esp.show_line);
    fprintf(f, ",\n");
    write_json_bool(f, "bSkeleton", esp.show_skeleton);
    fprintf(f, ",\n");
    write_json_bool(f, "bHealth", esp.show_health);
    fprintf(f, ",\n");
    write_json_bool(f, "bDistance", esp.show_distance);
    fprintf(f, ",\n");
    write_json_bool(f, "bName", esp.show_name);
    fprintf(f, ",\n");
    write_json_bool(f, "bWeapon", esp.show_weapon);
    fprintf(f, ",\n");
    write_json_float(f, "ESP_MaxDistance", esp.max_distance);
    fprintf(f, ",\n");
    write_json_float(f, "GlobalAlpha", esp.global_alpha);
    fprintf(f, ",\n");
    write_json_float(f, "ShadowAlpha", esp.shadow_alpha);
    fprintf(f, ",\n");
    write_json_color(f, "Col_ESP_Team", esp.team_color);
    fprintf(f, ",\n");
    write_json_color(f, "Col_ESP_Enemy", esp.enemy_color);
    fprintf(f, ",\n");
    write_json_color(f, "Col_ESP_AimTarget", esp.aim_target_color);
    fprintf(f, ",\n");
    write_json_color(f, "Col_ESP_Shadow", esp.shadow_color);
    fprintf(f, ",\n");
    write_json_float(f, "SmoothFactor", esp.smooth_factor);
    fprintf(f, ",\n");
    write_json_float(f, "ESP_TextScale", esp.text_scale);
    fprintf(f, ",\n");

    write_json_bool(f, "ShowHeadCircle", esp.show_head_circle);
    fprintf(f, ",\n");
    write_json_float(f, "HeadCircleThickness", esp.head_circle_thickness);
    fprintf(f, ",\n");
    write_json_color(f, "Col_HeadCircle", esp.head_circle_color);
    fprintf(f, ",\n");
    write_json_float(f, "SkeletonThickness", esp.skeleton_thickness);
    fprintf(f, ",\n");
    write_json_color(f, "Col_Skeleton", esp.skeleton_color);
    fprintf(f, ",\n");

    // Crosshair
    write_json_bool(f, "CrosshairEnable", crosshair.enabled);
    fprintf(f, ",\n");
    write_json_int(f, "CrosshairType", crosshair.type);
    fprintf(f, ",\n");
    write_json_float(f, "CrosshairSize", crosshair.size);
    fprintf(f, ",\n");

    // Triggerbot
    write_json_bool(f, "TrigEnable", triggerbot.enabled);
    fprintf(f, ",\n");
    write_json_int(f, "TrigKey", triggerbot.key);
    fprintf(f, ",\n");
    write_json_int(f, "TrigDelayMin", triggerbot.delay_min);
    fprintf(f, ",\n");
    write_json_int(f, "TrigDelayMax", triggerbot.delay_max);
    fprintf(f, ",\n");
    write_json_bool(f, "TrigTeamCheck", triggerbot.team_check);
    fprintf(f, ",\n");
    write_json_float(f, "TrigMaxVelocity", triggerbot.max_velocity);
    fprintf(f, ",\n");
    write_json_int(f, "TrigMode", triggerbot.mode);
    fprintf(f, ",\n");
    write_json_float(f, "TrigFovThresh", triggerbot.fov_threshold);
    fprintf(f, ",\n");

    // Misc
    write_json_int(f, "MaxFramerate", misc.max_fps);
    fprintf(f, "\n");

    fprintf(f, "}\n");
    fclose(f);
    return true;
}

void Config::set_defaults() {
    menu_key = VK_INSERT;
    panic_key = VK_END;

    esp_toggle_key = 0;
    aimbot_toggle_key = 0;
    crosshair_toggle_key = 0;

    aimbot = aimbot::AimbotConfig();       // 默认参数在 aimbot.h 中
    triggerbot = aimbot::TriggerbotConfig();
    esp = esp::ESPConfig();
    crosshair = crosshair::CrosshairConfig();
    misc = misc::MiscConfig();
}

Config& get() { return g_config; }

} // namespace cs2::config

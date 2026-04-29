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

    // Aimbot
    aimbot.enabled = json_get_bool(json, "AimBotEnable", false);
    aimbot.key0 = json_get_int(json, "AimKey0", VK_LBUTTON);
    aimbot.key1 = json_get_int(json, "AimKey1", 0);
    aimbot.mode = json_get_int(json, "AimMode", 0);
    aimbot.key_mode = json_get_int(json, "AimKeyMode", 0);
    aimbot.target_bone = json_get_int(json, "AimTargetBone", 6);
    aimbot.fov = json_get_float(json, "AimFOV", 5.0f);
    aimbot.smooth = json_get_float(json, "AimSmooth", 2.0f);
    aimbot.max_distance = json_get_float(json, "AimMaxDistance", 0.0f);
    aimbot.show_fov = json_get_bool(json, "bShowFOV", true);
    aimbot.rainbow_fov = json_get_bool(json, "bRainbowFOV", false);
    aimbot.recoil_control = json_get_bool(json, "bRCS", false);
    aimbot.rcs_scale = json_get_float(json, "R_SCALE", 0.5f);

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
    esp.team_color = json_get_color(json, "Col_ESP_Team", Color(0.2f, 0.8f, 1.0f, 1.0f));
    esp.enemy_color = json_get_color(json, "Col_ESP_Enemy", Color(1.0f, 0.3f, 0.2f, 1.0f));
    esp.aim_target_color = json_get_color(json, "Col_ESP_AimTarget", Color(1.0f, 1.0f, 0.0f, 1.0f));
    esp.shadow_color = json_get_color(json, "Col_ESP_Shadow", Color(0, 0, 0, 0.5f));

    // Crosshair
    crosshair.enabled = json_get_bool(json, "CrosshairEnable", false);
    crosshair.type = json_get_int(json, "CrosshairType", 0);
    crosshair.size = json_get_float(json, "CrosshairSize", 10.0f);

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

    // Aimbot
    write_json_bool(f, "AimBotEnable", aimbot.enabled);
    fprintf(f, ",\n");
    write_json_int(f, "AimKey0", aimbot.key0);
    fprintf(f, ",\n");
    write_json_int(f, "AimKey1", aimbot.key1);
    fprintf(f, ",\n");
    write_json_int(f, "AimMode", aimbot.mode);
    fprintf(f, ",\n");
    write_json_int(f, "AimKeyMode", aimbot.key_mode);
    fprintf(f, ",\n");
    write_json_int(f, "AimTargetBone", aimbot.target_bone);
    fprintf(f, ",\n");
    write_json_float(f, "AimFOV", aimbot.fov);
    fprintf(f, ",\n");
    write_json_float(f, "AimSmooth", aimbot.smooth);
    fprintf(f, ",\n");
    write_json_float(f, "AimMaxDistance", aimbot.max_distance);
    fprintf(f, ",\n");
    write_json_bool(f, "bShowFOV", aimbot.show_fov);
    fprintf(f, ",\n");
    write_json_bool(f, "bRainbowFOV", aimbot.rainbow_fov);
    fprintf(f, ",\n");
    write_json_bool(f, "bRCS", aimbot.recoil_control);
    fprintf(f, ",\n");
    write_json_float(f, "R_SCALE", aimbot.rcs_scale);
    fprintf(f, ",\n");

    // ESP
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

    // Crosshair
    write_json_bool(f, "CrosshairEnable", crosshair.enabled);
    fprintf(f, ",\n");
    write_json_int(f, "CrosshairType", crosshair.type);
    fprintf(f, ",\n");
    write_json_float(f, "CrosshairSize", crosshair.size);
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

    aimbot = aimbot::AimbotConfig();
    esp = esp::ESPConfig();
    crosshair = crosshair::CrosshairConfig();
    misc = misc::MiscConfig();
}

Config& get() { return g_config; }

} // namespace cs2::config

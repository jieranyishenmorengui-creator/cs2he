#pragma once
#include <string>
#include "../features/aimbot.h"
#include "../features/esp.h"
#include "../features/crosshair.h"
#include "../features/misc.h"

namespace cs2::config {

struct Config {
    // Menu settings
    int menu_key = VK_INSERT;
    int panic_key = VK_END;

    // Feature configs
    aimbot::AimbotConfig aimbot;
    esp::ESPConfig esp;
    crosshair::CrosshairConfig crosshair;
    misc::MiscConfig misc;

    // Load from JSON file
    bool load(const std::string& path);
    bool save(const std::string& path);

    // Default settings
    void set_defaults();
};

extern Config g_config;

// Access the global config
Config& get();

} // namespace cs2::config

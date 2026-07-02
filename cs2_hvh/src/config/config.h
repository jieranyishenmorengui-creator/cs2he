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

    // Feature toggle keys (0 = disabled / not bound)
    int esp_toggle_key = 0;
    int aimbot_toggle_key = 0;
    int crosshair_toggle_key = 0;

    // VisCheck map (empty = auto-detect, otherwise e.g. "de_mirage")
    std::string vis_map;

    // Feature configs
    aimbot::AimbotConfig aimbot;
    aimbot::TriggerbotConfig triggerbot;
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

#pragma once
#include "../utils/sdk.h"
#include "../features/aimbot.h"
#include "../features/esp.h"
#include "../features/crosshair.h"
#include "../features/misc.h"

namespace cs2::menu {

// Check if menu is currently open
bool is_open();
void set_open(bool open);

// Toggle menu visibility
void toggle();

// Render the menu overlay
void render();

} // namespace cs2::menu

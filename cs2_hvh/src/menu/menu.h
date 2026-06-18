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

// Hit-test: is a client-area point over the ImGui menu window?
// Used by overlay to decide mouse passthrough.
bool is_point_over(float cx, float cy);

} // namespace cs2::menu

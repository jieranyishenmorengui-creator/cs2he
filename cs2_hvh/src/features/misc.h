#pragma once
#include <windows.h>

namespace cs2::misc {

struct MiscConfig {
    int max_fps = 144;
    float global_alpha = 1.0f;
    float shadow_alpha = 0.5f;
    bool anti_afk = false;
    bool auto_jump = false;
    int auto_jump_key = VK_SPACE;
};

void run(const MiscConfig& cfg);

} // namespace cs2::misc

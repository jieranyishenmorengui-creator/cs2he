#pragma once
#include "../utils/sdk.h"

namespace cs2::esp {

struct ESPConfig {
    bool enabled = true;
    bool team_check = true;
    bool show_box = true;
    int box_type = 0;       // 0=rect, 1=corner, 2=3D
    bool filled = false;
    bool show_line = false;
    bool show_skeleton = false;
    bool show_health = true;
    bool show_distance = true;
    bool show_name = true;
    bool show_weapon = false;
    float max_distance = 200.0f;
    float global_alpha = 1.0f;
    float shadow_alpha = 0.5f;

    Color team_color     = Color(0.2f, 0.8f, 1.0f, 1.0f);
    Color enemy_color    = Color(1.0f, 0.3f, 0.2f, 1.0f);
    Color aim_target_color = Color(1.0f, 1.0f, 0.0f, 1.0f);
    Color shadow_color   = Color(0, 0, 0, 0.5f);
};

void run(const ESPConfig& cfg);

} // namespace cs2::esp

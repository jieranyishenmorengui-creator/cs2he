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
    bool show_health = false;
    bool show_distance = false;
    bool show_name = true;
    bool show_weapon = true;
    float text_scale = 1.0f;        // ESP文字大小 (1.0=16px, 1.5=24px)
    float max_distance = 200.0f;
    float global_alpha = 1.0f;
    float shadow_alpha = 0.5f;
    float smooth_factor = 0.09f;  // 0=无平滑, >0=插值强度

    bool show_head_circle = true;
    float head_circle_thickness = 1.5f;
    Color head_circle_color = Color(1, 1, 1, 1.0f);

    float skeleton_thickness = 1.5f;
    Color skeleton_color = Color(1, 1, 1, 0.6f);

    bool show_visible_color = true; // spotted→绿色框
    Color visible_color   = Color(0.0f, 1.0f, 0.3f, 1.0f); // 可见敌人绿色
    Color team_color      = Color(0.2f, 0.8f, 1.0f, 1.0f);
    Color enemy_color     = Color(1.0f, 0.3f, 0.2f, 1.0f);
    Color aim_target_color = Color(1.0f, 1.0f, 0.0f, 1.0f);
    Color shadow_color    = Color(0, 0, 0, 0.5f);
};

void run(const ESPConfig& cfg);

} // namespace cs2::esp

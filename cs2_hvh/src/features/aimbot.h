#pragma once
#include "../utils/sdk.h"

namespace cs2::aimbot {

struct AimbotConfig {
    bool enabled = false;
    int key0 = VK_LBUTTON;  // primary key
    int key1 = 0;            // secondary key
    int mode = 0;            // 0=off, 1=closest to crosshair, 2=distance, 3=fov
    int key_mode = 0;        // 0=hold, 1=toggle, 2=always
    int target_bone = 7;     // 7=head, 6=neck, 3=spine, 1=pelvis
    float fov = 5.0f;
    float smooth = 2.0f;
    float max_distance = 0.0f; // 0 = no limit
    bool visible_check = true;
    bool team_check = true;
    bool show_fov = true;
    bool rainbow_fov = false;
    bool recoil_control = false;
    float rcs_scale = 0.5f;
};

// Update aimbot (called every frame from game thread)
void run(const AimbotConfig& cfg);

} // namespace cs2::aimbot

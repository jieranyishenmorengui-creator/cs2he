#pragma once
#include "../utils/sdk.h"

namespace cs2::aimbot {

// FutaZone-style aim modes
enum class AimMode : int {
    Linear      = 0,
    FastThenSlow,
    SlowThenFast,
    Overshoot,
    Random
};

struct AimbotConfig {
    bool enabled = false;
    int key0 = VK_SHIFT;          // 激活键
    int key1 = 0;
    int key_mode = 0;             // 0=按住, 1=切换, 2=始终

    int target_bone = 7;          // 7=头部
    float fov = 250.0f;           // 屏幕 FOV (像素)
    float smoothness = 1.5f;      // 平滑度 (默认1.5, 越小越快)
    float max_distance = 0.0f;

    bool visible_check = true;
    bool team_check = true;
    bool disable_when_flashed = false;
    float flash_threshold = 5.0f;

    // 输入方式
    int input_method = 0;         // 0=写角度  1=SendInput

    // 压枪 RCS
    bool recoil_control = false;
    float rcs_scale = 0.5f;

    // FutaZone 高级瞄准模式
    int aim_mode = 0;
    bool randomize_speed = false;
    int speed_change_duration = 500;
    float overshoot_scale = 1.2f;
};

struct TriggerbotConfig {
    bool enabled = false;
    int key = VK_MENU;               // ALT
    int delay_min = 10;
    int delay_max = 25;
    bool team_check = true;
    float max_velocity = 18.0f;
};

void run(const AimbotConfig& cfg);
void triggerbot(const TriggerbotConfig& cfg);

} // namespace cs2::aimbot

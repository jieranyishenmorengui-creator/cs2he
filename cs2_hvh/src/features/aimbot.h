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
    bool enabled = true;
    int key0 = VK_RBUTTON;        // 激活键 (鼠标右键)
    int key1 = 0;
    int key_mode = 0;             // 0=按住, 1=切换, 2=始终

    int target_bone = 7;          // 7=头部
    float fov = 268.0f;           // 屏幕 FOV (像素)
    float smoothness = 1.0f;      // 平滑度 (越小越快)
    float max_distance = 0.0f;

    bool visible_check = false;  // 默认关: m_bSpotted有1-2秒延迟
    bool team_check = true;
    bool disable_when_flashed = true;
    float flash_threshold = 5.0f;

    // 速度提前量 (0=关闭, >0=提前秒数)
    float lead_time = 0.051f;

    // 输入方式
    int input_method = 0;         // 0=写角度  1=SendInput

    // 压枪 RCS
    bool recoil_control = true;
    float rcs_scale = 0.73f;

    // FutaZone 高级瞄准模式
    int aim_mode = 0;
    bool randomize_speed = false;
    int speed_change_duration = 500;
    float overshoot_scale = 1.2f;

    // 目标优先级: 0=FOV(准星最近)  1=距离(最近目标)
    int aim_priority = 0;

    // 击杀冷却: 杀死一个后等待多少ms再锁下一个
    int kill_delay_ms = 223;
};

struct TriggerbotConfig {
    bool enabled = false;
    int key = 0x4C;                  // L键 (76)
    int delay_min = 10;
    int delay_max = 25;
    bool team_check = false;
    float max_velocity = 18.0f;
};

void run(const AimbotConfig& cfg);
void triggerbot(const TriggerbotConfig& cfg);

} // namespace cs2::aimbot

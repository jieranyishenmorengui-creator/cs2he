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
    int key0 = 0x06;               // 鼠标侧键
    int key1 = 0;
    int key_mode = 0;             // 0=按住, 1=切换, 2=始终

    int target_bone = 7;          // 7=头部
    float fov = 800.0f;           // 屏幕 FOV (像素)
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
    float rcs_scale = 0.54f;

    // FutaZone 高级瞄准模式
    int aim_mode = 0;
    bool randomize_speed = false;
    int speed_change_duration = 500;
    float overshoot_scale = 1.2f;

    // 目标优先级: 0=FOV(准星最近)  1=距离(最近)  2=血量(补枪)
    int aim_priority = 0;

    // 击杀冷却
    int kill_delay_ms = 0;

    // 目标锁定: 锁定后不因新敌人出现（出生）而切换，只有目标死亡才解锁
    bool hard_lock = true;

    // visible_check 超时: 记住最后 spotted 时间，此窗口内认为可见 (ms)
    int spotted_timeout_ms = 1500;

    // 侧身头部补偿: 敌人侧身时头骨不在正上方
    bool head_offset_enabled = true;
    float head_offset_amount = 3.6f;      // 补偿量 (单位)
    float head_offset_angle_min = 45.0f;  // 开始补偿的最小角度
    float head_offset_angle_max = 135.0f; // 最大角度
};

struct TriggerbotConfig {
    bool enabled = false;
    int key = 0x4C;                  // L键 (76)
    int delay_min = 10;
    int delay_max = 25;
    bool team_check = false;
    float max_velocity = 18.0f;
    // 检测模式: 0=m_iIDEntIndex  1=FOV(角度, 更精确)
    int mode = 1;                    // FOV角度检测
    float fov_threshold = 1.5f;      // FOV模式触发阈值(度)
};

void run(const AimbotConfig& cfg);
void triggerbot(const TriggerbotConfig& cfg);

// Shared visible-set for ESP (populated by aimbot raycast, read by render thread)
struct VisibleSet {
    uintptr_t pawns[64];
    int count = 0;
};
extern VisibleSet g_visible_set;

} // namespace cs2::aimbot

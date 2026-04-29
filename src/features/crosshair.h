#pragma once
#include "../utils/sdk.h"

namespace cs2::crosshair {

struct CrosshairConfig {
    bool enabled = false;
    int type = 0;       // 0=cross, 1=circle, 2=dot
    float size = 10.0f;
    Color color = Color(1.0f, 0.0f, 0.0f, 0.8f);
    float thickness = 1.5f;
};

void run(const CrosshairConfig& cfg);

} // namespace cs2::crosshair

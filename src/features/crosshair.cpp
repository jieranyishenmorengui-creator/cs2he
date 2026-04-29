#include "crosshair.h"
#include "../core/overlay.h"
#include "../core/renderer.h"

namespace cs2::crosshair {

using namespace ::cs2::renderer;

void run(const CrosshairConfig& cfg) {
    if (!cfg.enabled || !overlay::is_ready()) return;

    int sw = overlay::get_width();
    int sh = overlay::get_height();
    float cx = sw / 2.0f;
    float cy = sh / 2.0f;

    switch (cfg.type) {
    case 0: { // Cross
        float s = cfg.size;
        draw_line(cx - s, cy, cx - s / 3.0f, cy, cfg.color, cfg.thickness);
        draw_line(cx + s / 3.0f, cy, cx + s, cy, cfg.color, cfg.thickness);
        draw_line(cx, cy - s, cx, cy - s / 3.0f, cfg.color, cfg.thickness);
        draw_line(cx, cy + s / 3.0f, cx, cy + s, cfg.color, cfg.thickness);
        break;
    }
    case 1: { // Circle
        draw_circle(cx, cy, cfg.size, cfg.color, 32, cfg.thickness);
        break;
    }
    case 2: { // Dot
        draw_filled_rect(cx - 2, cy - 2, 4, 4, cfg.color);
        break;
    }
    }
}

} // namespace cs2::crosshair

#pragma once
#include <d3d11.h>
#include <string>
#include "../utils/sdk.h"

namespace cs2::renderer {

// Initialize rendering resources (shaders, buffers, font)
bool initialize();
void shutdown();

// Drawing primitives
void draw_line(float x1, float y1, float x2, float y2, const Color& color, float thickness = 1.0f);
void draw_rect(float x, float y, float w, float h, const Color& color, float thickness = 1.0f);
void draw_filled_rect(float x, float y, float w, float h, const Color& color);
void draw_outlined_rect(float x, float y, float w, float h, const Color& color, const Color& outline);
void draw_circle(float cx, float cy, float radius, const Color& color, int segments = 32, float thickness = 1.0f);
void draw_text(float x, float y, const std::wstring& text, const Color& color, float scale = 1.0f);

// Convenience: draw text with centering
void draw_text_shadow(float x, float y, const std::wstring& text, const Color& color, float scale = 1.0f);

// Text size estimation
float get_text_width(const std::wstring& text, float scale = 1.0f);
float get_text_height(float scale = 1.0f);

// Health bar
void draw_health_bar(float x, float y, float w, float h, int health, float max_h = 100.0f);

// Corner box (for ESP)
void draw_corner_box(float x, float y, float w, float h, const Color& color, float line_w = 1.5f, float corner_len = 6.0f);

} // namespace cs2::renderer

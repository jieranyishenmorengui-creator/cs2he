#include "renderer.h"
#include "overlay.h"
#include "../imgui/imgui.h"
#include <string>
#include <vector>
#include <cstdlib>

namespace cs2::renderer {

// === Color conversion ===
static ImU32 to_u32(const Color& c) {
    return IM_COL32(
        (int)(c.r * 255.0f + 0.5f),
        (int)(c.g * 255.0f + 0.5f),
        (int)(c.b * 255.0f + 0.5f),
        (int)(c.a * 255.0f + 0.5f)
    );
}

static ImVec2 to_v2(float x, float y) { return ImVec2(x, y); }

// Wstring → UTF-8 for ImGui's AddText
static std::string w_to_u8(const std::wstring& w) {
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string u8(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), u8.data(), len, nullptr, nullptr);
    return u8;
}

// === Drawing primitives ===

void draw_line(float x1, float y1, float x2, float y2, const Color& color, float thickness) {
    auto* dl = ImGui::GetForegroundDrawList();
    dl->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), to_u32(color), thickness);
}

void draw_rect(float x, float y, float w, float h, const Color& color, float thickness) {
    auto* dl = ImGui::GetForegroundDrawList();
    dl->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), to_u32(color), 0.0f, 0, thickness);
}

void draw_filled_rect(float x, float y, float w, float h, const Color& color) {
    auto* dl = ImGui::GetForegroundDrawList();
    dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), to_u32(color));
}

void draw_outlined_rect(float x, float y, float w, float h, const Color& color, const Color& outline) {
    auto* dl = ImGui::GetForegroundDrawList();
    ImU32 c = to_u32(color);
    ImU32 o = to_u32(outline);
    dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), c);
    dl->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), o, 0.0f, 0, 1.0f);
}

void draw_circle(float cx, float cy, float radius, const Color& color, int segments, float thickness) {
    auto* dl = ImGui::GetForegroundDrawList();
    dl->AddCircle(ImVec2(cx, cy), radius, to_u32(color), segments, thickness);
}

void draw_text(float x, float y, const std::wstring& text, const Color& color, float scale) {
    auto* dl = ImGui::GetForegroundDrawList();
    auto u8 = w_to_u8(text);
    float font_size = ImGui::GetFontSize() * scale;
    dl->AddText(ImGui::GetFont(), font_size, ImVec2(x, y), to_u32(color), u8.c_str());
}

void draw_text_shadow(float x, float y, const std::wstring& text, const Color& color, float scale) {
    auto* dl = ImGui::GetForegroundDrawList();
    auto u8 = w_to_u8(text);
    float font_size = ImGui::GetFontSize() * scale;
    ImU32 shadow = IM_COL32(0, 0, 0, (int)(color.a * 255.0f));
    dl->AddText(ImGui::GetFont(), font_size, ImVec2(x + 1, y + 1), shadow, u8.c_str());
    dl->AddText(ImGui::GetFont(), font_size, ImVec2(x, y), to_u32(color), u8.c_str());
}

float get_text_width(const std::wstring& text, float scale) {
    auto u8 = w_to_u8(text);
    return ImGui::CalcTextSize(u8.c_str()).x;
}

float get_text_height(float scale) {
    return ImGui::GetFontSize() * scale;
}

void draw_health_bar(float x, float y, float w, float h, int health, float max_h) {
    float ratio = (float)health / max_h;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    auto* dl = ImGui::GetForegroundDrawList();

    // Background
    dl->AddRectFilled(ImVec2(x - 5, y), ImVec2(x - 1, y + h), IM_COL32(26, 26, 26, 204));

    // Health fill — gradient green to red
    float r = ratio < 0.5f ? 1.0f : 2.0f * (1.0f - ratio);
    float g = ratio > 0.5f ? 1.0f : 2.0f * ratio;
    float fill_h = h * ratio;
    dl->AddRectFilled(ImVec2(x - 5, y + h - fill_h), ImVec2(x - 1, y + h),
                      IM_COL32((int)(r * 255), (int)(g * 255), 0, 204));

    // Outline
    dl->AddRect(ImVec2(x - 5, y), ImVec2(x - 1, y + h), IM_COL32(0, 0, 0, 230));
}

void draw_corner_box(float x, float y, float w, float h, const Color& color, float line_w, float corner_len) {
    auto* dl = ImGui::GetForegroundDrawList();
    ImU32 c = to_u32(color);
    float cl = corner_len;

    // Top-left
    dl->AddRectFilled(ImVec2(x - 1, y - 1), ImVec2(x - 1 + cl, y + 2), c);
    dl->AddRectFilled(ImVec2(x - 1, y - 1), ImVec2(x + 2, y - 1 + cl), c);

    // Top-right
    dl->AddRectFilled(ImVec2(x + w - cl + 1, y - 1), ImVec2(x + w + 2, y + 2), c);
    dl->AddRectFilled(ImVec2(x + w - 2, y - 1), ImVec2(x + w + 1, y - 1 + cl), c);

    // Bottom-left
    dl->AddRectFilled(ImVec2(x - 1, y + h - 2), ImVec2(x - 1 + cl, y + h + 1), c);
    dl->AddRectFilled(ImVec2(x - 1, y + h - cl + 1), ImVec2(x + 2, y + h + 2), c);

    // Bottom-right
    dl->AddRectFilled(ImVec2(x + w - cl + 1, y + h - 2), ImVec2(x + w + 2, y + h + 1), c);
    dl->AddRectFilled(ImVec2(x + w - 2, y + h - cl + 1), ImVec2(x + w + 1, y + h + 2), c);
}

bool initialize() {
    // Nothing to do — ImGui is initialized by overlay::initialize()
    return true;
}

void shutdown() {
    // Nothing to do — ImGui is shutdown by overlay::shutdown()
}

} // namespace cs2::renderer

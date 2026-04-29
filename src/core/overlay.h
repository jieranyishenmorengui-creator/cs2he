#pragma once
#include <windows.h>
#include <d3d11.h>
#include <cstdint>

namespace cs2::overlay {

// Initialize overlay window (transparent, click-through, topmost)
bool initialize(HINSTANCE hInstance, HWND targetWnd);

// Shutdown and clean resources
void shutdown();

// Message loop tick — returns false on quit
bool pump_messages();

// Get overlay dimensions (matches game window)
int get_width();
int get_height();

// Check if overlay is ready to render
bool is_ready();

// Toggle overlay visibility
void set_visible(bool visible);
bool is_visible();

// Key state for menu toggle
bool is_key_down(int vk);
bool was_key_pressed(int vk);

// ImGui menu state — toggles WS_EX_TRANSPARENT so mouse reaches the overlay
void set_menu_open(bool open);
bool is_menu_open();

// Render target
extern ID3D11Device*           g_device;
extern ID3D11DeviceContext*    g_context;
extern ID3D11RenderTargetView* g_rtv;

// Begin/end frame
void begin_frame();
void end_frame();

} // namespace cs2::overlay

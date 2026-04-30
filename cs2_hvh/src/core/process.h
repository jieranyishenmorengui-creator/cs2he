#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <cstdint>

namespace cs2::process {

// Find the CS2 process window and get handles
bool initialize(const wchar_t* process_name = L"cs2.exe",
                const wchar_t* window_class = L"SDL_app");

// Initialize with a specific window (for testing without CS2)
bool initialize(HWND target_hwnd);

// Cleanup handles
void shutdown();

// Get game window info
HWND get_game_window();
RECT get_game_rect();

// Process info
DWORD get_process_id();
bool is_process_running();

} // namespace cs2::process

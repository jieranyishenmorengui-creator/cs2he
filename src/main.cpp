#include <windows.h>
#include <cstdio>
#include <cstring>
#include <thread>
#include <atomic>
#include <chrono>

#include "core/memory.h"
#include "core/process.h"
#include "core/overlay.h"
#include "core/renderer.h"
#include "core/offsets.h"

#include "config/config.h"
#include "menu/menu.h"
#include "features/aimbot.h"
#include "features/esp.h"
#include "features/crosshair.h"
#include "features/misc.h"
#include "utils/debug_log.h"
#include "imgui/imgui.h"

static bool g_running = true;
static std::atomic<const char*> g_init_status{"Initializing..."};

static void game_thread() {
    using namespace cs2;

    printf("[*] Game thread started\n");
    debug_log("Game thread started");

    // Wait for process to be ready
    g_init_status = "Waiting for CS2 process...";
    while (g_running && !process::is_process_running()) {
        Sleep(1000);
    }
    if (!g_running) return;

    // Scan offsets
    g_init_status = "Scanning offsets...";
    printf("[*] Scanning offsets...\n");
    debug_log("Scanning offsets...");
    if (!offsets::scan_offsets()) {
        printf("[!] Failed to scan offsets\n");
        debug_log("FAILED: scan_offsets");
        MessageBoxA(nullptr, "Failed to scan offsets. Game may have updated.", "Error", MB_ICONERROR);
        g_running = false;
        return;
    }
    printf("[+] All offsets found\n");
    debug_log("Offsets scanned OK");

    // Load config
    debug_log("Loading config...");
    if (!config::get().load("config.json")) {
        printf("[!] No config found, using defaults\n");
        debug_log("No config file, using defaults");
        config::get().set_defaults();
        config::get().save("config.json");
    }
    printf("[+] Config loaded\n");
    debug_log("Config loaded OK");

    g_init_status = "Ready - Press INSERT for menu";

    g_init_status = "Ready - Press INSERT for menu";

    // Main game loop
    auto last_time = std::chrono::high_resolution_clock::now();

    while (g_running) {
        if (!process::is_process_running()) {
            printf("[!] Game process closed\n");
            g_running = false;
            break;
        }

        auto& cfg = config::get();

        // Run features
        aimbot::run(cfg.aimbot);
        // ESP runs in render thread for drawing
        // Crosshair also in render thread

        // Panic key
        if (overlay::was_key_pressed(cfg.panic_key)) {
            printf("[!] Panic key pressed - unloading\n");
            g_running = false;
            break;
        }

        // Menu toggle
        if (overlay::was_key_pressed(cfg.menu_key)) {
            menu::toggle();
        }

        // Tick at ~200Hz (5ms)
        Sleep(5);
    }
}

static void render_thread() {
    using namespace cs2;

    printf("[*] Render thread started\n");
    debug_log("Render thread started");

    int frame_count = 0;
    while (g_running) {
        if (!overlay::is_ready()) {
            Sleep(10);
            continue;
        }

        if (!overlay::pump_messages()) {
            g_running = false;
            break;
        }

        // Check if game window still exists
        HWND game_wnd = process::get_game_window();
        if (game_wnd && !IsWindow(game_wnd)) {
            printf("[!] Game window destroyed\n");
            g_running = false;
            break;
        }

        // Render frame
        overlay::begin_frame();

        // Always-visible debug overlay (tests ImGui pipeline + shows status)
        {
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
            ImGui::SetNextWindowBgAlpha(0.75f);
            ImGui::Begin("##dbg", nullptr,
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);
            ImGui::TextColored(ImVec4(0,1,0,1), "CS2 HvH Active");
            ImGui::Text("Menu: INSERT  |  Status: %s", g_init_status.load());
            ImGui::End();
        }

        auto& cfg = config::get();

        // Draw features
        if (!menu::is_open() || true) {
            esp::run(cfg.esp);
            crosshair::run(cfg.crosshair);
        }

        // Draw menu on top
        menu::render();

        // Debug overlay (FPS + init status)
        {
            using namespace cs2::renderer;

            // FPS counter
            static int g_frame_count = 0;
            static auto g_fps_timer = std::chrono::high_resolution_clock::now();
            static int g_current_fps = 0;

            g_frame_count++;
            auto fps_now = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float>(fps_now - g_fps_timer).count();
            if (dt >= 1.0f) {
                g_current_fps = g_frame_count;
                g_frame_count = 0;
                g_fps_timer = fps_now;
            }

            wchar_t fps_buf[32];
            swprintf(fps_buf, 32, L"FPS: %d", g_current_fps);
            draw_text_shadow(8, 8, fps_buf, Color(0, 1, 0, 1), 0.7f);

            // Init status (yellow, disappears after full init)
            const char* status = g_init_status.load();
            if (status && status[0]) {
                std::wstring wstatus(status, status + strlen(status));
                draw_text_shadow(8, 28, wstatus, Color(1, 1, 0, 1), 0.7f);
            }
        }

        overlay::end_frame();

        // Cap render thread at ~144 FPS to avoid saturating the GPU
        Sleep(7);
        frame_count++;
    }
}

static void init_console() {
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    printf("===== CS2 HvH =====\n");
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    // Clear previous debug log
    FILE* f = fopen("cs2_hvh_debug.txt", "w");
    if (f) fclose(f);

    debug_log("=== CS2 HvH Starting ===");
    init_console();

    printf("[*] Initializing...\n");
    debug_log("Console initialized");

    // Find CS2 process
    debug_log("Looking for CS2 process...");
    if (!cs2::process::initialize(L"cs2.exe", L"SDL_app")) {
        printf("[!] Failed to find CS2\n");
        debug_log("FAILED: CS2 process not found");
        MessageBoxA(nullptr, "Could not find CS2. Please start the game first.", "CS2 HvH", MB_ICONINFORMATION);
        return 1;
    }
    printf("[+] CS2 process found (PID: %u)\n", cs2::process::get_process_id());
    debug_log("CS2 process found OK");

    // Create overlay
    debug_log("Creating overlay + D3D11 device...");
    HWND game_wnd = cs2::process::get_game_window();
    if (!cs2::overlay::initialize(hInstance, game_wnd)) {
        printf("[!] Failed to create overlay\n");
        debug_log("FAILED: Overlay creation");
        cs2::process::shutdown();
        return 1;
    }
    printf("[+] Overlay created (%dx%d)\n", cs2::overlay::get_width(), cs2::overlay::get_height());
    debug_log("Overlay D3D11 OK");

    // Initialize renderer
    debug_log("Initializing renderer (shaders, font)...");
    if (!cs2::renderer::initialize()) {
        printf("[!] Failed to initialize renderer\n");
        debug_log("FAILED: Renderer init");
        cs2::overlay::shutdown();
        cs2::process::shutdown();
        return 1;
    }
    printf("[+] Renderer initialized\n");
    debug_log("Renderer OK");

    printf("[*] Starting threads...\n");
    debug_log("Starting threads...");

    // Start threads
    std::thread t_game(game_thread);
    std::thread t_render(render_thread);

    t_game.join();
    t_render.join();

    // Cleanup
    printf("[*] Shutting down...\n");
    cs2::config::get().save("config.json");
    cs2::renderer::shutdown();
    cs2::overlay::shutdown();
    cs2::process::shutdown();

    printf("[*] Goodbye\n");
    FreeConsole();
    return 0;
}

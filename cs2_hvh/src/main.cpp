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

static std::atomic<bool> g_running{true};
static bool g_test_mode = false;
static std::atomic<const char*> g_init_status{"Initializing..."};

// Atomic flags for cross-thread key events
static std::atomic<bool> g_panic_key{false};
static std::atomic<bool> g_menu_toggle{false};

static void game_thread() {
    using namespace cs2;

    printf("[*] Game thread started\n");
    debug_log("Game thread started");

    if (g_test_mode) {
        printf("[*] Test mode: skipping CS2 init\n");
        g_init_status = "TEST MODE - No CS2";

        while (g_running) {
            if (g_panic_key.exchange(false)) {
                printf("[!] Panic key - unloading\n");
                g_running = false;
                break;
            }
            if (g_menu_toggle.exchange(false)) {
                menu::toggle();
            }
            Sleep(5);
        }
        return;
    }

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

    // Main game loop
    while (g_running) {
        if (!process::is_process_running()) {
            printf("[!] Game process closed\n");
            g_running = false;
            break;
        }

        auto& cfg = config::get();
        aimbot::run(cfg.aimbot);

        // Read atomic key events set by render thread
        if (g_panic_key.exchange(false)) {
            printf("[!] Panic key pressed - unloading\n");
            g_running = false;
            break;
        }
        if (g_menu_toggle.exchange(false)) {
            menu::toggle();
        }

        Sleep(5);
    }
}

static void render_thread_logic() {
    using namespace cs2;

    printf("[*] Render thread started\n");
    debug_log("Render thread started");

    int frame_count = 0;
    while (g_running) {
        if (!overlay::is_ready()) {
            Sleep(10);
            continue;
        }

        // Pump messages for overlay window (same thread that created it)
        if (!overlay::pump_messages()) {
            g_running = false;
            break;
        }

        // Check if game window still exists
        HWND game_wnd = process::get_game_window();
        if (!IsWindow(game_wnd)) {
            printf("[!] Game window destroyed\n");
            g_running = false;
            break;
        }

        // All ImGui calls in the same thread
        overlay::begin_frame();

        // Debug overlay
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

        // Read keys and set atomic flags for game thread
        if (overlay::was_key_pressed(cfg.panic_key))
            g_panic_key = true;
        if (overlay::was_key_pressed(cfg.menu_key))
            g_menu_toggle = true;

        // Draw features
        if (g_test_mode) {
            ImGui::GetForegroundDrawList()->AddRectFilled(
                ImVec2(0, 0),
                ImVec2((float)overlay::get_width(), (float)overlay::get_height()),
                IM_COL32(255, 0, 0, 80));
            ImGui::GetForegroundDrawList()->AddText(
                ImGui::GetFont(), 24.0f,
                ImVec2(overlay::get_width()/2.0f - 100, overlay::get_height()/2.0f),
                IM_COL32(255, 255, 255, 255), "OVERLAY TEST");
            crosshair::run(cfg.crosshair);
        } else {
            if (!menu::is_open() || true) {
                esp::run(cfg.esp);
                crosshair::run(cfg.crosshair);
            }
        }

        menu::render();

        // FPS counter
        {
            using namespace cs2::renderer;
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

            const char* status = g_init_status.load();
            if (status && status[0]) {
                std::wstring wstatus(status, status + strlen(status));
                draw_text_shadow(8, 28, wstatus, Color(1, 1, 0, 1), 0.7f);
            }
        }

        overlay::end_frame();
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
    FILE* f = fopen("cs2_hvh_debug.txt", "w");
    if (f) fclose(f);

    debug_log("=== CS2 HvH Starting ===");
    init_console();

    printf("[*] Initializing...\n");
    debug_log("Console initialized");

    // Find CS2 process
    debug_log("Looking for CS2 process...");
    if (!cs2::process::initialize(L"cs2.exe", L"SDL_app")) {
        printf("[!] CS2 not found, entering TEST MODE\n");
        debug_log("CS2 not found, entering test mode");
        g_test_mode = true;

        const wchar_t TEST_CLASS[] = L"CS2_Test_Target";
        WNDCLASSEXW tc{};
        tc.cbSize        = sizeof(WNDCLASSEXW);
        tc.lpfnWndProc   = DefWindowProcW;
        tc.hInstance     = hInstance;
        tc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        tc.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
        tc.lpszClassName = TEST_CLASS;
        RegisterClassExW(&tc);

        HWND test_target = CreateWindowExW(
            0, TEST_CLASS, L"CS2 HvH Test Window",
            WS_OVERLAPPEDWINDOW,
            200, 100, 800, 600,
            nullptr, nullptr, hInstance, nullptr
        );
        ShowWindow(test_target, SW_SHOW);
        UpdateWindow(test_target);

        if (!cs2::process::initialize(test_target)) {
            printf("[!] Failed to init test window\n");
            return 1;
        }
        printf("[*] Test mode: overlay on test window (800x600)\n");
        debug_log("Test mode overlay target created");
    } else {
        printf("[+] CS2 process found (PID: %u)\n", cs2::process::get_process_id());
        debug_log("CS2 process found OK");
    }

    // Start game logic thread
    std::thread t_game(game_thread);

    // Render thread: create overlay and run ImGui in this thread
    std::thread t_render([hInstance]() {
        using namespace cs2;

        // Overlay + D3D11 + ImGui — all in the render thread
        debug_log("Creating overlay + D3D11 device...");
        HWND game_wnd = process::get_game_window();
        if (!overlay::initialize(hInstance, game_wnd)) {
            printf("[!] Failed to create overlay\n");
            debug_log("FAILED: Overlay creation");
            process::shutdown();
            g_running = false;
            return;
        }
        printf("[+] Overlay created (%dx%d)\n", overlay::get_width(), overlay::get_height());
        debug_log("Overlay D3D11 OK");

        debug_log("Initializing renderer...");
        if (!renderer::initialize()) {
            printf("[!] Failed to initialize renderer\n");
            debug_log("FAILED: Renderer init");
            overlay::shutdown();
            process::shutdown();
            g_running = false;
            return;
        }
        printf("[+] Renderer initialized\n");
        debug_log("Renderer OK");

        // Run render loop (message pump + ImGui in this same thread)
        render_thread_logic();

        // Cleanup
        printf("[*] Shutting down render thread...\n");
        renderer::shutdown();
        overlay::shutdown();
    });

    t_game.join();
    t_render.join();

    // Cleanup
    printf("[*] Shutting down...\n");
    cs2::config::get().save("config.json");
    cs2::process::shutdown();

    printf("[*] Goodbye\n");
    FreeConsole();
    return 0;
}

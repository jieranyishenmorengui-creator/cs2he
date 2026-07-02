#include <windows.h>
#include <cstdio>
#include <cstring>
#include <thread>
#include <atomic>
#include <chrono>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#include "core/memory.h"
#include "core/process.h"
#include "core/overlay.h"
#include "core/renderer.h"
#include "core/offsets.h"
#include "core/vischeck.h"

#include "config/config.h"
#include "menu/menu.h"
#include "features/aimbot.h"
#include "features/esp.h"
#include "features/spectator.h"
#include "features/crosshair.h"
#include "features/misc.h"
#include "utils/debug_log.h"
#include "imgui/imgui.h"

static std::atomic<bool> g_running{true};
static bool g_test_mode = false;
static std::atomic<const char*> g_init_status{"Initializing..."};
static cs2::vischeck::VisCheck g_vischeck;
static std::string g_current_map;
static char g_vis_status[64] = "VisCheck: no map data";
static constexpr const char* MAP_DIR = "data/";

// Map name → .opt file loader
static void try_load_map(const std::string& map_name) {
    static bool s_last_fail = false; // only log one "no .opt" per map
    if (map_name.empty() || map_name == g_current_map) { s_last_fail = false; return; }

    std::string path = std::string(MAP_DIR) + map_name + ".opt";
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        if (!s_last_fail) printf("[VisCheck] No .opt for '%s'\n", map_name.c_str());
        s_last_fail = true;
        g_current_map.clear();
        g_pVisCheck = nullptr;
        return;
    }
    s_last_fail = false;
    fclose(f);

    if (g_vischeck.load_map(path)) {
        g_current_map = map_name;
        g_pVisCheck = &g_vischeck;
        printf("[VisCheck] Map '%s' loaded\n", map_name.c_str());
        snprintf(g_vis_status, sizeof(g_vis_status), "VisCheck: %s", map_name.c_str());
    } else {
        printf("[VisCheck] Failed to load %s\n", path.c_str());
        snprintf(g_vis_status, sizeof(g_vis_status), "VisCheck: load failed");
    }
}

static uintptr_t get_engine2_base() {
    DWORD pid = cs2::process::get_process_id();
    if (!pid) return 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    uintptr_t base = 0;
    MODULEENTRY32W me{};
    me.dwSize = sizeof(me);
    if (Module32FirstW(snap, &me)) {
        do {
            if (_wcsicmp(me.szModule, L"engine2.dll") == 0) { base = (uintptr_t)me.modBaseAddr; break; }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return base;
}

static std::string get_map_name() {
    static std::string s_cached;
    static bool s_done = false;
    if (s_done) return s_cached;
    s_done = true;

    // ── Window title (quickest, no module deps) ─────
    wchar_t wt[256]{};
    HWND gw = cs2::process::get_game_window();
    if (gw && GetWindowTextW(gw, wt, 256)) {
        char t[64]{}; wcstombs(t, wt, 64);
        const char* de = strstr(t, "de_");
        if (!de) de = strstr(t, "ar_");
        if (!de) de = strstr(t, "cs_");
        if (de) {
            int j = 3; while (j < 24 && de[j] >= 'a' && de[j] <= 'z') j++;
            if (j > 5) { s_cached = std::string(de, de + j); return s_cached; }
        }
    }

        // ── CNetworkGameClient at engine2+0x90A1A0 ──
    uintptr_t e2 = get_engine2_base();
    if (!e2) return {};
    uintptr_t ngc = cs2::memory::read<uintptr_t>(e2 + 0x90A1A0);
    if (ngc && cs2::memory::IsRemotePtrValid(ngc)) {
        int signon = cs2::memory::read<int>(ngc + 0x230);
        if (signon == 6) {
            char chunk[0x10000];
            if (cs2::memory::read(ngc, chunk, sizeof(chunk))) {
                for (int i = 0; i < (int)sizeof(chunk) - 32; ++i) {
                    if (chunk[i] != 'd' || chunk[i+1] != 'e' || chunk[i+2] != '_') continue;
                    int len = 3;
                    while (len < 28 && i + len < (int)sizeof(chunk) && ((chunk[i+len] >= 'a' && chunk[i+len] <= 'z') || chunk[i+len] == '_' || (chunk[i+len] >= '0' && chunk[i+len] <= '9'))) len++;
                    if (len < 6 || len > 24) continue;
                    std::string map(chunk + i, len);
                    std::string opt = std::string(MAP_DIR) + map + ".opt";
                    FILE* f = fopen(opt.c_str(), "rb");
                    if (f) { fclose(f); s_cached = map; printf("[VisCheck] Map: %s\n", map.c_str()); return s_cached; }
                }
            }
        } else {
            printf("[VisCheck] signOnState=%d (expected 6)\n", signon);
        }
    }
}

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

    // Try loading map for vischeck
    try_load_map(get_map_name());

    // Main game loop
    while (g_running) {
        if (!process::is_process_running()) {
            printf("[!] Game process closed\n");
            g_running = false;
            break;
        }

        // Periodically check map change (every ~100 iterations)
        static int map_check = 0;
        if (++map_check % 100 == 0)
            try_load_map(get_map_name());

        auto& cfg = config::get();
        aimbot::run(cfg.aimbot);
        aimbot::triggerbot(cfg.triggerbot);

        // Idle sleep when no game features active (省 CPU)
        if (!cfg.aimbot.enabled && !cfg.triggerbot.enabled)
            Sleep(50);

        // Read atomic key events set by render thread
        if (g_panic_key.exchange(false)) {
            printf("[!] Panic key pressed - unloading\n");
            g_running = false;
            break;
        }
        if (g_menu_toggle.exchange(false)) {
            menu::toggle();
        }

        Sleep(1);  // 高更新率 ~1000次/s, 跟枪更平滑
    }
}

static void render_thread_logic() {
    using namespace cs2;

    printf("[*] Render thread started\n");
    debug_log("Render thread started");

    using clock = std::chrono::high_resolution_clock;
    auto last_frame = clock::now();

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

        auto& cfg = config::get();

        // ── Feature toggle hotkeys ─────────────────────────────
        if (cfg.esp_toggle_key && overlay::was_key_pressed(cfg.esp_toggle_key))
            cfg.esp.enabled = !cfg.esp.enabled;
        if (cfg.aimbot_toggle_key && overlay::was_key_pressed(cfg.aimbot_toggle_key))
            cfg.aimbot.enabled = !cfg.aimbot.enabled;
        if (cfg.crosshair_toggle_key && overlay::was_key_pressed(cfg.crosshair_toggle_key))
            cfg.crosshair.enabled = !cfg.crosshair.enabled;

        // All ImGui calls in the same thread
        overlay::begin_frame();

        // FPS counter (updated each frame)
        {
            using namespace std::chrono;
            static int g_frame_count = 0;
            static auto g_fps_timer = high_resolution_clock::now();
            static int g_current_fps = 0;

            g_frame_count++;
            auto fps_now = high_resolution_clock::now();
            float dt = duration<float>(fps_now - g_fps_timer).count();
            if (dt >= 1.0f) {
                g_current_fps = g_frame_count;
                g_frame_count = 0;
                g_fps_timer = fps_now;
            }

            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
            ImGui::SetNextWindowBgAlpha(0.75f);
            ImGui::Begin("##dbg", nullptr,
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);
            ImGui::TextColored(ImVec4(0,1,0,1), "CS2 HvH Active  |  FPS: %d", g_current_fps);
            ImGui::Text("Menu: INSERT  |  Status: %s", g_init_status.load());
            ImGui::TextColored(g_pVisCheck ? ImVec4(0.3f,1,0.3f,1) : ImVec4(1,0.3f,0,1), "%s", g_vis_status);
            ImGui::End();
        }

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
                spectator::update();
                spectator::draw();
                crosshair::run(cfg.crosshair);
            }
        }

        menu::render();

        overlay::end_frame();

        // ── Frame limiter (high precision) ────────────────────
        // Sleep has ~15ms granularity by default, so for small waits
        // we spin-wait the final 1ms after a coarse Sleep.
        {
            auto now = clock::now();
            double elapsed_ms = std::chrono::duration<double, std::milli>(now - last_frame).count();
            double target_ms = 1000.0 / std::max(cfg.misc.max_fps, 1);
            if (elapsed_ms < target_ms) {
                double remain_ms = target_ms - elapsed_ms;
                if (remain_ms > 2.0) {
                    Sleep(DWORD(remain_ms - 1.0));  // coarse wait
                }
                // Spin-wait for final precision (avoids Sleep(1)≈15ms)
                while (std::chrono::duration<double, std::milli>(
                           clock::now() - last_frame).count() < target_ms) {
                    Sleep(0);  // yield timeslice
                }
            }
            last_frame = clock::now();
        }
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
    // High-resolution timer for Sleep() precision
    timeBeginPeriod(1);

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
    timeEndPeriod(1);
    FreeConsole();
    return 0;
}

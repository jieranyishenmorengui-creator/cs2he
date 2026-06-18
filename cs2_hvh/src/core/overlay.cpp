#include "overlay.h"
#include "process.h"
#include "../utils/debug_log.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx11.h"
#include "../imgui/imgui_impl_win32.h"
#include <dwmapi.h>
#include <dxgi1_3.h>
#include <cstdio>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwmapi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace cs2::overlay {

ID3D11Device*            g_device = nullptr;
ID3D11DeviceContext*     g_context = nullptr;
ID3D11RenderTargetView*  g_rtv = nullptr;

static HWND                   g_overlayWnd = nullptr;
static HWND                   g_targetWnd = nullptr;
static WNDCLASSEXW            g_wc{};
static bool                   g_visible = false;
static bool                   g_ready = false;
static int                    g_width = 0;
static int                    g_height = 0;
static uint8_t                g_keyState[256] = {};
static uint8_t                g_keyPrev[256] = {};
static bool                   g_menu_open = false;

// DXGI swap chain (replaces old GDI UpdateLayeredWindow + staging approach)
static IDXGISwapChain*       g_swapChain = nullptr;

void set_menu_open(bool open) {
    g_menu_open = open;
    if (g_overlayWnd) {
        LONG style = GetWindowLongW(g_overlayWnd, GWL_EXSTYLE);
        if (open)
            style &= ~WS_EX_TRANSPARENT;
        else
            style |= WS_EX_TRANSPARENT;
        SetWindowLongW(g_overlayWnd, GWL_EXSTYLE, style);
        SetWindowPos(g_overlayWnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}

bool is_menu_open() { return g_menu_open; }

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (g_menu_open && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp))
        return 1;

    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        g_keyState[wp] = 1;
        return 0;
    case WM_KEYUP:
        g_keyState[wp] = 0;
        return 0;
    case WM_SYSKEYDOWN:
        if (wp == VK_MENU) g_keyState[wp] = 1;
        return 0;
    case WM_SYSKEYUP:
        if (wp == VK_MENU) g_keyState[wp] = 0;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ── Create D3D11 device + swap chain ───────────────────────────

static bool create_d3d11_resources() {
    D3D_FEATURE_LEVEL feats[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    HRESULT hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        feats, 3, D3D11_SDK_VERSION,
        &g_device, nullptr, &g_context);

    if (FAILED(hr)) {
        debug_log("D3D11 HARDWARE failed (0x%08X), trying WARP...", (unsigned)hr);
        hr = D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            feats, 3, D3D11_SDK_VERSION,
            &g_device, nullptr, &g_context);
    }
    if (FAILED(hr)) {
        debug_log("D3D11 ALL FAILED (0x%08X)", (unsigned)hr);
        return false;
    }
    debug_log("D3D11 device OK");

    // ── Create DXGI swap chain (FLIP_DISCARD + premultiplied alpha) ─
    // FLIP_DISCARD + DXGI_ALPHA_MODE_PREMULTIPLIED lets DWM composite
    // per-pixel transparency directly from the buffer's alpha channel.
    // Window is NOT WS_EX_LAYERED — no chroma-key needed.
    IDXGIDevice*   pDXGIDevice = nullptr;
    IDXGIAdapter*  pAdapter    = nullptr;
    IDXGIFactory2* pFactory2   = nullptr;

    g_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDXGIDevice);
    pDXGIDevice->GetAdapter(&pAdapter);
    pAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&pFactory2);

    DXGI_SWAP_CHAIN_DESC1 sd1{};
    sd1.Width             = g_width;
    sd1.Height            = g_height;
    sd1.Format            = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd1.SampleDesc.Count  = 1;
    sd1.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd1.BufferCount       = 2;       // flip model requires ≥2
    sd1.SwapEffect        = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd1.AlphaMode         = DXGI_ALPHA_MODE_PREMULTIPLIED;
    sd1.Flags             = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    IDXGISwapChain1* swap1 = nullptr;
    hr = pFactory2->CreateSwapChainForHwnd(g_device, g_overlayWnd,
                                           &sd1, nullptr, nullptr, &swap1);
    if (FAILED(hr)) {
        debug_log("CreateSwapChainForHwnd FAILED (0x%08X)", (unsigned)hr);
        pDXGIDevice->Release();
        pAdapter->Release();
        pFactory2->Release();
        return false;
    }
    g_swapChain = swap1;   // IDXGISwapChain1 → base IDXGISwapChain

    pDXGIDevice->Release();
    pAdapter->Release();
    pFactory2->Release();

    if (FAILED(hr)) {
        debug_log("CreateSwapChain FAILED (0x%08X)", (unsigned)hr);
        return false;
    }
    debug_log("Swap chain created (%dx%d)", g_width, g_height);

    // ── Get back-buffer and create RTV ───────────────────────────
    ID3D11Texture2D* backBuffer = nullptr;
    if (FAILED(g_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer))) {
        debug_log("GetBuffer FAILED");
        return false;
    }
    if (FAILED(g_device->CreateRenderTargetView(backBuffer, nullptr, &g_rtv))) {
        debug_log("CreateRenderTargetView FAILED");
        backBuffer->Release();
        return false;
    }
    backBuffer->Release();

    debug_log("D3D11 + swap chain ready (%dx%d)", g_width, g_height);
    return true;
}

static void destroy_d3d11_resources() {
    // Unbind RTV from the pipeline first
    if (g_context) g_context->OMSetRenderTargets(0, nullptr, nullptr);
    if (g_rtv)       { g_rtv->Release();       g_rtv       = nullptr; }
    if (g_swapChain) { g_swapChain->Release();  g_swapChain = nullptr; }
}

static bool init_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplWin32_Init(g_overlayWnd)) {
        debug_log("ImGui_ImplWin32_Init failed");
        return false;
    }
    if (!ImGui_ImplDX11_Init(g_device, g_context)) {
        debug_log("ImGui_ImplDX11_Init failed");
        return false;
    }
    debug_log("ImGui OK");
    return true;
}

// ── Present via flip-model swap chain ─────────────────────────
// FLIP_DISCARD + ALLOW_TEARING = zero-copy, bypass DWM queue.
static void present_frame() {
    if (!g_ready || !g_swapChain || !g_context)
        return;
    g_swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
    // FPS cap is done by the frame limiter
}

// ── Position & resize ───────────────────────────────────────────

static void update_position() {
    RECT r = process::get_game_rect();
    POINT tl{ r.left, r.top };
    ClientToScreen(g_targetWnd, &tl);

    int new_w = r.right;
    int new_h = r.bottom;
    if (new_w <= 0) new_w = 1;
    if (new_h <= 0) new_h = 1;

    if (new_w != g_width || new_h != g_height) {
        g_width  = new_w;
        g_height = new_h;

        SetWindowPos(g_overlayWnd, HWND_TOPMOST, tl.x, tl.y, g_width, g_height,
                     SWP_NOACTIVATE);

        // Recreate render target + staging at new size
        destroy_d3d11_resources();
        if (!create_d3d11_resources()) {
            debug_log("FAILED to recreate D3D11 resources after resize");
            g_ready = false;
        }
    } else {
        SetWindowPos(g_overlayWnd, HWND_TOPMOST, tl.x, tl.y, g_width, g_height,
                     SWP_NOACTIVATE);
    }
}

// ── Public API ──────────────────────────────────────────────────

bool initialize(HINSTANCE hInstance, HWND targetWnd) {
    g_targetWnd = targetWnd;

    RECT r;
    GetClientRect(targetWnd, &r);
    g_width  = r.right  > 0 ? r.right  : 1920;
    g_height = r.bottom > 0 ? r.bottom : 1080;

    g_wc.cbSize        = sizeof(WNDCLASSEXW);
    g_wc.style         = CS_HREDRAW | CS_VREDRAW;
    g_wc.lpfnWndProc   = wnd_proc;
    g_wc.hInstance     = hInstance;
    g_wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    g_wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    g_wc.lpszClassName = L"CS2_Overlay_Class";

    if (!RegisterClassExW(&g_wc)) return false;

    POINT tl{ r.left, r.top };
    ClientToScreen(targetWnd, &tl);

    // Try with WS_EX_TRANSPARENT first; fall back to a plain window
    DWORD ex_style = WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
    // Note: no WS_EX_LAYERED — we use DXGI_ALPHA_MODE_PREMULTIPLIED instead
    g_overlayWnd = CreateWindowExW(
        ex_style,
        L"CS2_Overlay_Class", L"CS2 Overlay",
        WS_POPUP,
        tl.x, tl.y, g_width, g_height,
        nullptr, nullptr, hInstance, nullptr);

    if (!g_overlayWnd) {
        debug_log("CreateWindowExW FAILED");
        return false;
    }

    debug_log("Overlay %dx%d at (%d,%d)", g_width, g_height, tl.x, tl.y);

    if (!create_d3d11_resources()) return false;
    if (!init_imgui()) return false;

    // Initial clear + show
    {
        float zero[4] = { 0, 0, 0, 0 };
        g_context->ClearRenderTargetView(g_rtv, zero);
        present_frame();
    }

    ShowWindow(g_overlayWnd, SW_SHOW);
    UpdateWindow(g_overlayWnd);
    SetWindowPos(g_overlayWnd, HWND_TOPMOST, tl.x, tl.y, g_width, g_height,
                 SWP_NOACTIVATE);

    g_visible = true;
    g_ready   = true;
    debug_log("Overlay ready");
    return true;
}

void shutdown() {
    g_ready = false;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    destroy_d3d11_resources();

    if (g_context) { g_context->Release(); g_context = nullptr; }
    if (g_device)  { g_device->Release();  g_device  = nullptr; }
    if (g_overlayWnd) { DestroyWindow(g_overlayWnd); g_overlayWnd = nullptr; }
    UnregisterClassW(L"CS2_Overlay_Class", GetModuleHandleW(nullptr));
}

bool pump_messages() {
    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (msg.message == WM_QUIT) return false;
    }
    memcpy(g_keyPrev, g_keyState, sizeof(g_keyState));
    for (int i = 0; i < 256; i++) {
        g_keyState[i] = (GetAsyncKeyState(i) & 0x8000) ? 1 : 0;
    }
    return true;
}

int   get_width()  { return g_width; }
int   get_height() { return g_height; }
bool  is_ready()   { return g_ready; }
void  set_visible(bool v) { g_visible = v; }
bool  is_visible() { return g_visible; }

bool is_key_down(int vk)     { return g_keyState[vk] != 0; }
bool was_key_pressed(int vk) { return g_keyState[vk] && !g_keyPrev[vk]; }

void begin_frame() {
    if (!g_ready) return;
    update_position();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    float zero[4] = { 0, 0, 0, 0 };
    g_context->ClearRenderTargetView(g_rtv, zero);
}

void end_frame() {
    if (!g_ready) return;

    ImGui::Render();
    // Bind our custom RTV before ImGui renders
    g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    present_frame();
}

} // namespace cs2::overlay

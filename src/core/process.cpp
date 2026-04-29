#include "process.h"
#include "memory.h"
#include <cstdio>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

namespace cs2::process {

static DWORD g_pid = 0;
static HWND g_gameWnd = nullptr;
static HANDLE g_hProcess = nullptr;
static uintptr_t g_clientBase = 0;
static size_t g_clientSize = 0;

static DWORD find_process_id(const wchar_t* name) {
    DWORD pid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, name) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

static HWND find_game_window(DWORD pid, const wchar_t* wclass) {
    struct ctx { DWORD pid; HWND result; const wchar_t* wclass; };
    ctx c = { pid, nullptr, wclass };
    EnumWindows([](HWND hwnd, LPARAM lp) -> BOOL {
        auto* ctx = (struct ctx*)lp;
        DWORD wpid = 0;
        GetWindowThreadProcessId(hwnd, &wpid);
        if (wpid == ctx->pid && IsWindowVisible(hwnd)) {
            wchar_t cls[256];
            GetClassNameW(hwnd, cls, 256);
            if (wcsstr(cls, ctx->wclass) != nullptr) {
                ctx->result = hwnd;
                return FALSE;
            }
        }
        return TRUE;
    }, (LPARAM)&c);
    return c.result;
}

bool initialize(const wchar_t* process_name, const wchar_t* window_class) {
    g_pid = find_process_id(process_name);
    if (!g_pid) {
        MessageBoxA(nullptr, "cs2.exe not found!", "Error", MB_ICONERROR);
        return false;
    }

    g_hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, g_pid);
    if (!g_hProcess) {
        MessageBoxA(nullptr, "OpenProcess failed. Run as admin.", "Error", MB_ICONERROR);
        return false;
    }

    memory::g_hProcess = g_hProcess;

    // Get client.dll base and size from remote process
    HMODULE hMods[1024];
    DWORD cbNeeded;
    if (EnumProcessModulesEx(g_hProcess, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_64BIT)) {
        int count = cbNeeded / sizeof(HMODULE);
        for (int i = 0; i < count; ++i) {
            wchar_t modName[MAX_PATH];
            if (GetModuleFileNameExW(g_hProcess, hMods[i], modName, MAX_PATH)) {
                wchar_t* baseName = wcsrchr(modName, L'\\');
                if (!baseName) baseName = modName; else ++baseName;
                if (_wcsicmp(baseName, L"client.dll") == 0) {
                    g_clientBase = (uintptr_t)hMods[i];
                    MODULEINFO mi;
                    if (GetModuleInformation(g_hProcess, hMods[i], &mi, sizeof(mi))) {
                        g_clientSize = mi.SizeOfImage;
                    }
                    memory::g_moduleBase = g_clientBase;
                    memory::g_moduleSize = g_clientSize;
                    printf("[+] client.dll base: 0x%llX, size: 0x%zX\n", g_clientBase, g_clientSize);
                    break;
                }
            }
        }
    }
    if (!g_clientBase) {
        printf("[!] Warning: client.dll not found in process modules\n");
    }

    g_gameWnd = find_game_window(g_pid, window_class);
    if (!g_gameWnd) {
        MessageBoxA(nullptr, "Game window not found!", "Error", MB_ICONERROR);
        return false;
    }

    return true;
}

void shutdown() {
    if (g_hProcess) {
        CloseHandle(g_hProcess);
        g_hProcess = nullptr;
        memory::g_hProcess = nullptr;
    }
}

HWND get_game_window() { return g_gameWnd; }

RECT get_game_rect() {
    RECT r{};
    if (g_gameWnd) GetClientRect(g_gameWnd, &r);
    return r;
}

DWORD get_process_id() { return g_pid; }

bool is_process_running() {
    DWORD code = 0;
    return g_hProcess && GetExitCodeProcess(g_hProcess, &code) && code == STILL_ACTIVE;
}

} // namespace cs2::process

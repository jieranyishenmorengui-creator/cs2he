#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include "CleanerTool.h"

static std::atomic<bool> g_running{ true };
static CleanerTool g_cleaner;

static void print_banner() {
    std::wcout << L"\n";
    std::wcout << L"  ╔════════════════════════════════════════╗\n";
    std::wcout << L"  ║       Perfect World Cleaner Tool      ║\n";
    std::wcout << L"  ║         完美世界竞技平台 清理工具       ║\n";
    std::wcout << L"  ╚════════════════════════════════════════╝\n";
    std::wcout << L"\n";
}

static void print_menu() {
    std::wcout << L"  ┌────────────────────────────────────────┐\n";
    std::wcout << L"  │  [1] 一键清理 (完整流程)               │\n";
    std::wcout << L"  │  [2] 停止服务 MessageTransfer          │\n";
    std::wcout << L"  │  [3] 删除服务 MessageTransfer          │\n";
    std::wcout << L"  │  [4] 等待目标进程                      │\n";
    std::wcout << L"  │  [5] 获取模块基址                      │\n";
    std::wcout << L"  │  [6] 暂停目标线程                      │\n";
    std::wcout << L"  │  [S] 查看状态                          │\n";
    std::wcout << L"  │  [Q] 退出                              │\n";
    std::wcout << L"  └────────────────────────────────────────┘\n";
    std::wcout << L"  请选择: ";
}

static void print_state() {
    const auto& s = g_cleaner.GetCurrentState();

    std::wcout << L"\n  ── 当前状态 ──────────────────────────────\n";
    std::wcout << L"  服务已停止:  " << (s.serviceStopped ? L"是" : L"否") << L"\n";
    std::wcout << L"  服务已删除:  " << (s.serviceDeleted ? L"是" : L"否") << L"\n";
    std::wcout << L"  进程已找到:  " << (s.processFound ? L"是" : L"否");
    if (s.processFound) std::wcout << L"  (PID: " << s.targetPid << L")";
    std::wcout << L"\n";
    std::wcout << L"  模块已加载:  " << (s.moduleLoaded ? L"是" : L"否");
    if (s.moduleLoaded) std::wcout << L"  (基址: 0x" << std::hex << std::uppercase << s.moduleBase << std::dec << L")";
    std::wcout << L"\n";
    std::wcout << L"  线程① 已暂停: " << (s.threadPaused ? L"是" : L"否") << L"\n";
    std::wcout << L"  线程② 已暂停: " << (s.secondThreadPaused ? L"是" : L"否") << L"\n";
    std::wcout << L"  线程③ 已暂停: " << (s.thirdThreadPaused ? L"是" : L"否") << L"\n";
    std::wcout << L"  执行中:      " << (s.waiting ? L"是" : L"否") << L"\n";
    std::wcout << L"  已完成:      " << (s.completed ? L"是" : L"否") << L"\n";
    std::wcout << L"  ────────────────────────────────────────────\n\n";
}

static void run_full_clean() {
    if (g_cleaner.IsWaiting()) {
        std::wcout << L"[!] 已有任务正在执行中，请等待完成\n";
        return;
    }

    std::wcout << L"\n[*] 开始一键清理流程...\n";
    std::wcout << L"[*] 步骤: 停止服务 → 删除服务 → 等待进程 → 等待模块 → 暂停线程\n\n";

    g_cleaner.StartCleanProcess();

    // Wait for completion
    int dots = 0;
    while (g_cleaner.IsWaiting()) {
        std::wcout << L"\r[*] 处理中";
        for (int i = 0; i < dots; i++) std::wcout << L".";
        for (int i = dots; i < 5; i++) std::wcout << L" ";
        std::wcout << std::flush;
        dots = (dots + 1) % 6;
        Sleep(200);
    }
    std::wcout << L"\r" << std::wstring(20, L' ') << L"\r";

    if (g_cleaner.IsCompleted()) {
        std::wcout << L"[✓] 一键清理完成\n\n";
    } else {
        std::wcout << L"[!] 流程异常结束\n\n";
    }
}

static void wait_for_process() {
    if (g_cleaner.IsWaiting()) {
        std::wcout << L"[!] 已有任务正在执行中\n";
        return;
    }

    std::wcout << L"[*] 等待目标进程: 完美世界竞技平台.exe\n";
    std::thread t([&]() {
        g_cleaner.WaitForTargetProcess();
    });
    t.detach();

    int dots = 0;
    while (g_cleaner.IsWaiting()) {
        std::wcout << L"\r[*] 等待中";
        for (int i = 0; i < dots; i++) std::wcout << L".";
        for (int i = dots; i < 5; i++) std::wcout << L" ";
        std::wcout << std::flush;
        dots = (dots + 1) % 6;
        Sleep(200);
    }
    std::wcout << L"\r" << std::wstring(20, L' ') << L"\r";

    if (g_cleaner.GetCurrentState().processFound) {
        std::wcout << L"[✓] 已找到目标进程\n\n";
    } else {
        std::wcout << L"[!] 未找到目标进程\n\n";
    }
}

static void get_module_base() {
    if (g_cleaner.IsWaiting()) {
        std::wcout << L"[!] 已有任务正在执行中\n";
        return;
    }

    const auto& s = g_cleaner.GetCurrentState();
    if (!s.processFound) {
        std::wcout << L"[!] 请先等待进程出现 (选项 4)\n\n";
        return;
    }

    std::wcout << L"[*] 正在获取模块基址: gameaddon.node\n";
    g_cleaner.WaitForTargetModule();

    const auto& s2 = g_cleaner.GetCurrentState();
    if (s2.moduleLoaded) {
        std::wcout << L"[✓] 模块基址: 0x" << std::hex << std::uppercase
            << s2.moduleBase << std::dec << L"\n\n";
    } else {
        std::wcout << L"[!] 获取模块基址失败\n\n";
    }
}

static void pause_target_thread() {
    if (g_cleaner.IsWaiting()) {
        std::wcout << L"[!] 已有任务正在执行中\n";
        return;
    }

    const auto& s = g_cleaner.GetCurrentState();
    if (!s.processFound || !s.moduleLoaded) {
        std::wcout << L"[!] 请先等待进程和模块 (选项 4 + 5)\n\n";
        return;
    }

    std::wcout << L"[*] 正在暂停目标线程...\n";

    std::thread t([&]() {
        g_cleaner.PauseTargetThread();
    });
    t.detach();

    int dots = 0;
    while (g_cleaner.IsWaiting()) {
        std::wcout << L"\r[*] 处理中";
        for (int i = 0; i < dots; i++) std::wcout << L".";
        for (int i = dots; i < 5; i++) std::wcout << L" ";
        std::wcout << std::flush;
        dots = (dots + 1) % 6;
        Sleep(200);
    }
    std::wcout << L"\r" << std::wstring(20, L' ') << L"\r";

    const auto& s2 = g_cleaner.GetCurrentState();
    if (s2.threadPaused && s2.secondThreadPaused && s2.thirdThreadPaused) {
        std::wcout << L"[✓] 全部线程已暂停\n\n";
    } else {
        std::wcout << L"[!] 部分线程暂停可能失败\n\n";
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Enable ANSI in console
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    // Set console title
    SetConsoleTitleW(L"Perfect World Cleaner Tool");

    // Initial state update
    g_cleaner.UpdateState();

    print_banner();
    std::wcout << L"[*] 按提示输入选项数字执行对应功能\n";
    std::wcout << L"[*] 默认目标: 完美世界竞技平台.exe\n";
    std::wcout << L"[*] 目标服务: MessageTransfer\n";
    std::wcout << L"[*] 目标模块: gameaddon.node\n\n";

    while (g_running) {
        print_menu();

        std::wstring input;
        std::getline(std::wcin, input);
        std::wcout << L"\n";

        if (input.empty()) continue;

        wchar_t cmd = input[0];
        switch (cmd) {
        case L'1':
            run_full_clean();
            break;

        case L'2': {
            std::wcout << L"[*] 正在停止服务...\n";
            if (g_cleaner.StopService())
                std::wcout << L"[✓] 服务已停止\n\n";
            else
                std::wcout << L"[!] 停止服务失败(可能已停止)\n\n";
            break;
        }

        case L'3': {
            std::wcout << L"[*] 正在删除服务...\n";
            if (g_cleaner.DeleteService())
                std::wcout << L"[✓] 服务已删除\n\n";
            else
                std::wcout << L"[!] 删除服务失败(可能不存在)\n\n";
            break;
        }

        case L'4':
            wait_for_process();
            break;

        case L'5':
            get_module_base();
            break;

        case L'6':
            pause_target_thread();
            break;

        case L's':
        case L'S':
            print_state();
            break;

        case L'q':
        case L'Q':
            std::wcout << L"[*] 正在退出...\n";
            g_running = false;
            break;

        default:
            std::wcout << L"[!] 无效选项，请重新输入\n\n";
            break;
        }
    }

    return 0;
}

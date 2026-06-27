#include "CleanerTool.h"
#include <iostream>
#include <tlhelp32.h>
#include <string.h>
#include <fstream>
#include <vector>
#include <iomanip>
#include <psapi.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include "ProcessUtils.h"

static std::wstring FindTargetProcessPath(const std::wstring& exeName);

// ── Free functions (declared in ProcessUtils.h) ──────────────────

DWORD FindProcessIdByName(const wchar_t* exeName) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe = { sizeof(pe) };
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, exeName) == 0) {
                CloseHandle(snap);
                return pe.th32ProcessID;
            }
        } while (Process32NextW(snap, &pe));
    }

    CloseHandle(snap);
    return 0;
}

uintptr_t GetModuleBaseAddress(DWORD pid, const wchar_t* moduleName) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    MODULEENTRY32W me = { sizeof(me) };
    if (Module32FirstW(snap, &me)) {
        do {
            if (_wcsicmp(me.szModule, moduleName) == 0) {
                CloseHandle(snap);
                return reinterpret_cast<uintptr_t>(me.modBaseAddr);
            }
        } while (Module32NextW(snap, &me));
    }

    CloseHandle(snap);
    return 0;
}

// ═══════════════════════════════════════════════════════════════════
//  CleanerState
// ═══════════════════════════════════════════════════════════════════
CleanerTool::CleanerState::CleanerState() {
    serviceStopped = false;
    serviceDeleted = false;
    threadPaused = false;
    thirdThreadPaused = false;
    secondThreadPaused = false;
    processFound = false;
    moduleLoaded = false;
    waiting = false;
    completed = false;
    targetPid = 0;
    moduleBase = 0;
    antiScreenshotStatusMessage = L"";
    antiScreenshotInjected = false;
    antiScreenshotInjecting = false;
}

// ═══════════════════════════════════════════════════════════════════
//  CleanerConfig
// ═══════════════════════════════════════════════════════════════════
CleanerTool::CleanerConfig::CleanerConfig() {
    serviceName = L"MessageTransfer";
    targetProcessName = L"完美世界竞技平台.exe";
    targetModuleName = L"gameaddon.node";
    targetOffset = 0x3A790;
    targetOffset2 = 0x3C530;
    targetOffset3 = 0x322194;
    checkInterval = 500;
    verboseLogging = false;
}

// ═══════════════════════════════════════════════════════════════════
//  CleanerTool
// ═══════════════════════════════════════════════════════════════════
CleanerTool::CleanerTool() : m_hJob(NULL) {
    Reset();
}

CleanerTool::CleanerTool(const CleanerConfig& cfg) : config(cfg), m_hJob(NULL) {
    Reset();
}

CleanerTool::~CleanerTool() {
    if (m_hJob) {
        CloseHandle(m_hJob);
        m_hJob = NULL;
    }
}

void CleanerTool::Reset() {
    state = CleanerState();
}

// ── StartCleanProcess ────────────────────────────────────────────

bool CleanerTool::StartCleanProcess() {
    if (state.waiting) return false;

    Reset();
    state.waiting = true;
    state.completed = false;

    DWORD threadId;
    HANDLE hThread = CreateThread(NULL, 0, CleanThreadProc, this, 0, &threadId);
    if (hThread) {
        CloseHandle(hThread);
        return true;
    }

    return false;
}

DWORD WINAPI CleanerTool::CleanThreadProc(LPVOID param) {
    CleanerTool* cleaner = (CleanerTool*)param;
    cleaner->ExecuteFullClean();
    return 0;
}

// ── ExecuteFullClean ─────────────────────────────────────────────

bool CleanerTool::ExecuteFullClean() {
    state.waiting = true;
    state.completed = false;

    if (!InternalStopService()) {
        std::wcout << L"[警告] 停止服务失败" << std::endl;
    }

    if (!InternalDeleteService()) {
        std::wcout << L"[警告] 删除服务失败" << std::endl;
    }

    WaitForTargetProcess();
    WaitForTargetModule();

    if (state.processFound && state.moduleLoaded) {
        PauseTargetThread();
        PauseSecondTargetThread();
        PauseThirdTargetThread();
    }

    state.completed = true;
    state.waiting = false;
    UpdateState();
    return true;
}

// ── Public wrappers ──────────────────────────────────────────────

bool CleanerTool::StopService() {
    return InternalStopService();
}

bool CleanerTool::DeleteService() {
    return InternalDeleteService();
}

bool CleanerTool::WaitForTargetProcess() {
    state.waiting = true;

    if (state.targetPid == 0) {
        std::wstring processPath = FindTargetProcessPath(config.targetProcessName);
        if (!processPath.empty()) {
            LaunchAndAttachToJob(processPath);
        }
    }

    int attempt = 0;
    while (!state.processFound) {
        state.targetPid = FindProcessIdByName(config.targetProcessName.c_str());
        state.processFound = (state.targetPid != 0);
        if (state.processFound) break;

        if (attempt % 10 == 0) {
            std::wcout << L"[等待] 等待目标进程: " << config.targetProcessName << std::endl;
        }
        attempt++;
        Sleep(config.checkInterval);
    }

    state.waiting = false;
    return state.processFound;
}

bool CleanerTool::WaitForTargetModule() {
    if (!state.processFound) return false;

    state.waiting = true;
    int attempt = 0;

    while (!state.moduleLoaded) {
        state.moduleBase = GetModuleBaseAddress(state.targetPid, config.targetModuleName.c_str());
        state.moduleLoaded = (state.moduleBase != 0);

        if (state.moduleLoaded) {
            std::wcout << L"[找到] 目标模块基址: 0x"
                << std::hex << std::uppercase << state.moduleBase << std::dec << std::endl;
            break;
        }

        if (attempt % 10 == 0) {
            std::wcout << L"[等待] 等待目标模块: " << config.targetModuleName << std::endl;
        }
        attempt++;
        Sleep(config.checkInterval);

        state.targetPid = FindProcessIdByName(config.targetProcessName.c_str());
        if (state.targetPid == 0) {
            state.processFound = false;
            break;
        }
    }

    state.waiting = false;
    return state.moduleLoaded;
}

bool CleanerTool::PauseTargetThread() {
    if (!state.processFound || !state.moduleLoaded) return false;

    state.waiting = true;
    int attempt = 0;

    while (!state.threadPaused) {
        if (InternalPauseTargetThread()) {
            state.threadPaused = true;
            std::wcout << L"[成功] 目标线程已暂停" << std::endl;
            break;
        }

        if (attempt % 10 == 0) {
            std::wcout << L"[等待] 尝试暂停目标线程..." << std::endl;
        }
        attempt++;

        Sleep(config.checkInterval);

        state.targetPid = FindProcessIdByName(config.targetProcessName.c_str());
        if (state.targetPid == 0) {
            state.processFound = false;
            break;
        }

        state.moduleBase = GetModuleBaseAddress(state.targetPid, config.targetModuleName.c_str());
        if (state.moduleBase == 0) {
            state.moduleLoaded = false;
            break;
        }
    }

    state.waiting = false;
    return state.threadPaused;
}

bool CleanerTool::PauseSecondTargetThread() {
    if (!state.processFound || !state.moduleLoaded || config.targetOffset2 == 0) return false;

    state.waiting = true;
    int attempt = 0;
    uintptr_t targetAddr2 = state.moduleBase + config.targetOffset2;

    while (!state.secondThreadPaused) {
        if (InternalPauseThreadAtAddress(targetAddr2, state.secondThreadPaused)) {
            std::wcout << L"[成功] ① 已处理" << std::endl;
            break;
        }

        if (attempt % 10 == 0) {
            std::wcout << L"[等待] 尝试第二个目标线程..." << std::endl;
        }
        attempt++;

        Sleep(config.checkInterval);

        state.targetPid = FindProcessIdByName(config.targetProcessName.c_str());
        if (state.targetPid == 0) {
            state.processFound = false;
            break;
        }

        state.moduleBase = GetModuleBaseAddress(state.targetPid, config.targetModuleName.c_str());
        if (state.moduleBase == 0) {
            state.moduleLoaded = false;
            break;
        }

        targetAddr2 = state.moduleBase + config.targetOffset2;
    }

    state.waiting = false;
    return state.secondThreadPaused;
}

bool CleanerTool::PauseThirdTargetThread() {
    return InternalPauseThirdTargetThread();
}

void CleanerTool::UpdateState() {
    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (scm) {
        SC_HANDLE svc = OpenServiceW(scm, config.serviceName.c_str(), SERVICE_QUERY_STATUS);
        if (svc) {
            SERVICE_STATUS_PROCESS ssp = { 0 };
            DWORD bytesNeeded = 0;
            if (QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO,
                (LPBYTE)&ssp, sizeof(ssp), &bytesNeeded)) {
                state.serviceStopped = (ssp.dwCurrentState == SERVICE_STOPPED);
            }
            CloseServiceHandle(svc);
        }
        else {
            DWORD err = GetLastError();
            state.serviceDeleted = (err == ERROR_SERVICE_DOES_NOT_EXIST);
        }
        CloseServiceHandle(scm);
    }

    state.targetPid = FindProcessIdByName(config.targetProcessName.c_str());
    state.processFound = (state.targetPid != 0);

    if (state.processFound) {
        state.moduleBase = GetModuleBaseAddress(state.targetPid, config.targetModuleName.c_str());
        state.moduleLoaded = (state.moduleBase != 0);
    }
}

// ── Getters / Setters ────────────────────────────────────────────

bool CleanerTool::IsCompleted() const {
    return state.completed;
}

bool CleanerTool::IsWaiting() const {
    return state.waiting;
}

void CleanerTool::SetConfig(const CleanerConfig& cfg) {
    config = cfg;
    Reset();
}

const CleanerTool::CleanerConfig& CleanerTool::GetConfig() const {
    return config;
}

const CleanerTool::CleanerState& CleanerTool::GetCurrentState() const {
    return state;
}

// ═══════════════════════════════════════════════════════════════════
//  Internal Service Operations
// ═══════════════════════════════════════════════════════════════════

bool CleanerTool::InternalStopService() {
    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm) return false;

    SC_HANDLE svc = OpenServiceW(scm, config.serviceName.c_str(),
        SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!svc) {
        DWORD err = GetLastError();
        if (err == ERROR_SERVICE_DOES_NOT_EXIST) {
            state.serviceStopped = true;
            CloseServiceHandle(scm);
            return true;
        }
        CloseServiceHandle(scm);
        return false;
    }

    SERVICE_STATUS_PROCESS ssp = { 0 };
    DWORD bytesNeeded = 0;
    if (QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO,
        (LPBYTE)&ssp, sizeof(ssp), &bytesNeeded)) {
        if (ssp.dwCurrentState == SERVICE_STOPPED) {
            state.serviceStopped = true;
            CloseServiceHandle(svc);
            CloseServiceHandle(scm);
            return true;
        }
    }

    SERVICE_STATUS tmp = { 0 };
    if (!ControlService(svc, SERVICE_CONTROL_STOP, &tmp)) {
        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        return false;
    }

    for (int i = 0; i < 50; i++) {
        Sleep(100);
        if (QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO,
            (LPBYTE)&ssp, sizeof(ssp), &bytesNeeded)) {
            if (ssp.dwCurrentState == SERVICE_STOPPED) {
                state.serviceStopped = true;
                CloseServiceHandle(svc);
                CloseServiceHandle(scm);
                return true;
            }
        }
    }

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return false;
}

bool CleanerTool::InternalDeleteService() {
    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm) return false;

    SC_HANDLE svc = OpenServiceW(scm, config.serviceName.c_str(),
        DELETE | SERVICE_QUERY_STATUS | SERVICE_STOP);
    if (!svc) {
        DWORD err = GetLastError();
        if (err == ERROR_SERVICE_DOES_NOT_EXIST) {
            state.serviceDeleted = true;
            CloseServiceHandle(scm);
            return true;
        }
        CloseServiceHandle(scm);
        return false;
    }

    if (!::DeleteService(svc)) {
        DWORD err = GetLastError();
        if (err == ERROR_SERVICE_DOES_NOT_EXIST) {
            state.serviceDeleted = true;
            CloseServiceHandle(svc);
            CloseServiceHandle(scm);
            return true;
        }
        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        return false;
    }

    state.serviceDeleted = true;
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return true;
}

// ═══════════════════════════════════════════════════════════════════
//  Internal Thread Operations
// ═══════════════════════════════════════════════════════════════════

bool CleanerTool::InternalPauseTargetThread() {
    if (!state.processFound || !state.moduleLoaded) return false;

    uintptr_t targetAddr = state.moduleBase + config.targetOffset;
    return InternalPauseThreadAtAddress(targetAddr, state.threadPaused);
}

bool CleanerTool::InternalPauseSecondTargetThread() {
    if (!state.processFound || !state.moduleLoaded) return false;

    uintptr_t targetAddr2 = state.moduleBase + config.targetOffset2;
    return InternalPauseThreadAtAddress(targetAddr2, state.secondThreadPaused);
}

bool CleanerTool::InternalPauseThirdTargetThread() {
    if (!state.processFound || !state.moduleLoaded || config.targetOffset3 == 0) return false;

    uintptr_t targetAddr3 = state.moduleBase + config.targetOffset3;
    return InternalPauseThreadAtAddress(targetAddr3, state.thirdThreadPaused);
}

bool CleanerTool::InternalPauseThreadAtAddress(uintptr_t targetAddress, bool& stateFlag) {
    if (!state.processFound) return false;

    HANDLE tsnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (tsnap == INVALID_HANDLE_VALUE) return false;

    THREADENTRY32 te = { sizeof(te) };
    bool anyPaused = false;

    if (Thread32First(tsnap, &te)) {
        do {
            if (te.th32OwnerProcessID != state.targetPid) continue;

            HANDLE th = OpenThread(THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION,
                FALSE, te.th32ThreadID);
            if (!th) continue;

            HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
            if (ntdll) {
                typedef LONG(WINAPI* NtQueryInformationThreadFn)(HANDLE, ULONG, PVOID, ULONG, PULONG);
                NtQueryInformationThreadFn NtQueryInformationThread =
                    (NtQueryInformationThreadFn)GetProcAddress(ntdll, "NtQueryInformationThread");

                if (NtQueryInformationThread) {
                    uintptr_t startAddr = 0;
                    ULONG retLen = 0;
                    LONG status = NtQueryInformationThread(th, 9, &startAddr, (ULONG)sizeof(startAddr), &retLen);
                    if (status == 0 && startAddr == targetAddress) {
                        SuspendThread(th);
                        anyPaused = true;
                    }
                }
            }

            CloseHandle(th);
        } while (Thread32Next(tsnap, &te));
    }

    CloseHandle(tsnap);
    stateFlag = anyPaused;
    return anyPaused;
}

// ═══════════════════════════════════════════════════════════════════
//  Process Launch
// ═══════════════════════════════════════════════════════════════════

DWORD CleanerTool::LaunchAndAttachToJob(const std::wstring& processPath) {
    if (m_hJob) {
        CloseHandle(m_hJob);
        m_hJob = NULL;
    }

    m_hJob = CreateJobObject(NULL, NULL);
    if (!m_hJob) {
        std::wcerr << L"[错误] 创建作业对象失败" << std::endl;
        return 0;
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo = { 0 };
    jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(m_hJob, JobObjectExtendedLimitInformation, &jobInfo, sizeof(jobInfo))) {
        std::wcerr << L"[错误] 设置作业属性失败" << std::endl;
        CloseHandle(m_hJob);
        m_hJob = NULL;
        return 0;
    }

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (!CreateProcessW(processPath.c_str(), NULL, NULL, NULL, FALSE,
        CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
        DWORD err = GetLastError();
        std::wcerr << L"[错误] 创建进程失败，错误码: " << err << std::endl;
        CloseHandle(m_hJob);
        m_hJob = NULL;
        return 0;
    }

    if (!AssignProcessToJobObject(m_hJob, pi.hProcess)) {
        DWORD err = GetLastError();
        std::wcerr << L"[错误] 将进程加入作业失败，错误码: " << err << std::endl;
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(m_hJob);
        m_hJob = NULL;
        return 0;
    }

    ResumeThread(pi.hThread);

    DWORD pid = pi.dwProcessId;
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    state.targetPid = pid;
    state.processFound = true;
    state.moduleLoaded = false;

    std::wcout << L"[信息] 已启动目标进程，PID: " << pid << std::endl;
    return pid;
}

// ═══════════════════════════════════════════════════════════════════
//  FindTargetProcessPath — look up installation path
// ═══════════════════════════════════════════════════════════════════

static std::wstring FindTargetProcessPath(const std::wstring& exeName) {
    // Check if process is already running
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe = { sizeof(pe) };
        if (Process32FirstW(snap, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, exeName.c_str()) == 0) {
                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
                    if (hProcess) {
                        wchar_t path[MAX_PATH] = { 0 };
                        DWORD size = MAX_PATH;
                        if (QueryFullProcessImageNameW(hProcess, 0, path, &size)) {
                            CloseHandle(hProcess);
                            CloseHandle(snap);
                            return path;
                        }
                        CloseHandle(hProcess);
                    }
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
    }

    // Check registry
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\完美世界竞技平台", 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS) {
        wchar_t installPath[MAX_PATH];
        DWORD type = 0, size = sizeof(installPath);
        if (RegQueryValueExW(hKey, L"InstallPath", NULL, &type, (LPBYTE)installPath, &size) == ERROR_SUCCESS && type == REG_SZ) {
            RegCloseKey(hKey);
            std::wstring fullPath = installPath;
            if (fullPath.back() != L'\\') fullPath += L'\\';
            fullPath += exeName;
            if (PathFileExistsW(fullPath.c_str())) {
                return fullPath;
            }
        }
        RegCloseKey(hKey);
    }

    // Common installation paths
    std::vector<std::wstring> commonPaths = {
        L"C:\\Program Files\\完美世界竞技平台",
        L"C:\\Program Files (x86)\\完美世界竞技平台",
        L"C:\\Program Files\\完美世界",
        L"C:\\Program Files (x86)\\完美世界"
    };
    wchar_t localAppData[MAX_PATH];
    if (GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH) > 0) {
        commonPaths.push_back(std::wstring(localAppData) + L"\\完美世界竞技平台");
    }

    for (const auto& dir : commonPaths) {
        std::wstring fullPath = dir + L"\\" + exeName;
        if (PathFileExistsW(fullPath.c_str())) {
            return fullPath;
        }
    }

    return L"";
}

#pragma once
#include <windows.h>
#include <string>

class CleanerTool {
public:
    struct CleanerState {
        bool serviceStopped;
        bool serviceDeleted;
        bool threadPaused;
        bool thirdThreadPaused;
        bool secondThreadPaused;
        bool processFound;
        bool moduleLoaded;
        bool waiting;
        bool completed;
        DWORD targetPid;
        uintptr_t moduleBase;
        std::wstring antiScreenshotStatusMessage;
        bool antiScreenshotInjected;
        bool antiScreenshotInjecting;

        CleanerState();
    };

    struct CleanerConfig {
        std::wstring serviceName;
        std::wstring targetProcessName;
        std::wstring targetModuleName;
        uintptr_t targetOffset;
        uintptr_t targetOffset2;
        uintptr_t targetOffset3;
        int checkInterval;
        bool verboseLogging;

        CleanerConfig();
    };

    CleanerTool();
    explicit CleanerTool(const CleanerConfig& cfg);
    ~CleanerTool();

    void Reset();
    bool StartCleanProcess();

    // Single-step operations
    bool StopService();
    bool DeleteService();
    bool WaitForTargetProcess();
    bool WaitForTargetModule();
    bool PauseTargetThread();
    bool PauseSecondTargetThread();
    bool PauseThirdTargetThread();

    // Full pipeline (called from thread)
    bool ExecuteFullClean();

    void UpdateState();

    bool IsCompleted() const;
    bool IsWaiting() const;

    void SetConfig(const CleanerConfig& cfg);
    const CleanerConfig& GetConfig() const;
    const CleanerState& GetCurrentState() const;

private:
    CleanerConfig config;
    CleanerState state;
    HANDLE m_hJob;

    bool InternalStopService();
    bool InternalDeleteService();
    bool InternalPauseTargetThread();
    bool InternalPauseSecondTargetThread();
    bool InternalPauseThirdTargetThread();
    bool InternalPauseThreadAtAddress(uintptr_t targetAddress, bool& stateFlag);
    DWORD LaunchAndAttachToJob(const std::wstring& processPath);

    static DWORD WINAPI CleanThreadProc(LPVOID param);
};

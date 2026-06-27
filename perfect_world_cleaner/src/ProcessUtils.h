#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <string>

// ── Process utilities ────────────────────────────────────────────

DWORD FindProcessIdByName(const wchar_t* exeName);
uintptr_t GetModuleBaseAddress(DWORD pid, const wchar_t* moduleName);

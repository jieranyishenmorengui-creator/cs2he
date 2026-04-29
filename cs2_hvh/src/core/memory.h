#pragma once
#include <windows.h>
#include <cstdint>
#include <vector>
#include <string>

namespace cs2::memory {

extern HANDLE g_hProcess;
extern uintptr_t g_moduleBase;
extern uintptr_t g_moduleSize;

// ── Memory validation ─────────────────────────────────────────────
// Proper validation beyond just null-check: uses VirtualQuery to verify
// the memory is committed, accessible, and not guarded/noaccess.

// Validate a pointer within the cheat's own process space.
inline bool IsPointerValid(void* ptr) {
    if (!ptr) return false;

    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0)
        return false;

    if (mbi.State != MEM_COMMIT)
        return false;

    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))
        return false;

    if (!(mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)))
        return false;

    return true;
}

// Validate an address in the remote (CS2) process space.
inline bool IsRemotePtrValid(uintptr_t addr) {
    if (!addr) return false;
    if (!g_hProcess) return false;

    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(g_hProcess, (LPCVOID)addr, &mbi, sizeof(mbi)) == 0)
        return false;

    if (mbi.State != MEM_COMMIT)
        return false;

    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))
        return false;

    if (!(mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)))
        return false;

    return true;
}

inline bool read(uintptr_t addr, void* buf, size_t size) {
    return ReadProcessMemory(g_hProcess, (LPCVOID)addr, buf, size, nullptr) != FALSE;
}

template<typename T>
T read(uintptr_t addr) {
    T val{};
    ReadProcessMemory(g_hProcess, (LPCVOID)addr, &val, sizeof(T), nullptr);
    return val;
}

// Validated read — checks IsRemotePtrValid before reading. Use for
// pointer-chain resolution where you want extra safety. For bulk/looping
// reads use the regular read<T> to avoid VirtualQueryEx overhead.
template<typename T>
T safe_read(uintptr_t addr) {
    T val{};
    if (!IsRemotePtrValid(addr)) return val;
    ReadProcessMemory(g_hProcess, (LPCVOID)addr, &val, sizeof(T), nullptr);
    return val;
}

template<typename T>
bool write(uintptr_t addr, const T& val) {
    return WriteProcessMemory(g_hProcess, (LPVOID)addr, &val, sizeof(T), nullptr) != FALSE;
}

std::vector<uint8_t> read_bytes(uintptr_t addr, size_t size);
std::string read_string(uintptr_t addr, size_t max_len = 128);
uintptr_t read_ptr(uintptr_t addr);
uintptr_t resolve_chain(uintptr_t base, const std::vector<uint32_t>& offsets);

} // namespace cs2::memory

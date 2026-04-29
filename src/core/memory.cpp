#include "memory.h"

namespace cs2::memory {

HANDLE g_hProcess = nullptr;
uintptr_t g_moduleBase = 0;
uintptr_t g_moduleSize = 0;

std::vector<uint8_t> read_bytes(uintptr_t addr, size_t size) {
    std::vector<uint8_t> buf(size);
    if (!ReadProcessMemory(g_hProcess, (LPCVOID)addr, buf.data(), size, nullptr))
        buf.clear();
    return buf;
}

std::string read_string(uintptr_t addr, size_t max_len) {
    std::string result;
    char ch;
    for (size_t i = 0; i < max_len; ++i) {
        if (!ReadProcessMemory(g_hProcess, (LPCVOID)(addr + i), &ch, 1, nullptr) || ch == '\0')
            break;
        result += ch;
    }
    return result;
}

uintptr_t read_ptr(uintptr_t addr) {
    uintptr_t val = 0;
    ReadProcessMemory(g_hProcess, (LPCVOID)addr, &val, sizeof(val), nullptr);
    return val;
}

uintptr_t resolve_chain(uintptr_t base, const std::vector<uint32_t>& offsets) {
    uintptr_t addr = base;
    for (size_t i = 0; i < offsets.size(); ++i) {
        addr = read_ptr(addr);
        if (!addr) return 0;
        addr += offsets[i];
    }
    return addr;
}

} // namespace cs2::memory

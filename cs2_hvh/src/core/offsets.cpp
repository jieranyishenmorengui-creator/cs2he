#include "offsets.h"
#include "memory.h"
#include <vector>
#include <cstring>
#include <cstdio>

namespace cs2::offsets {

Offsets g_offsets;

// Signature database — aligned with cs2-dumper pattern scanning approach
// Reference: https://github.com/a2x/cs2-dumper commit 2b80ca8
// These use IDA-style patterns: each byte is 2 hex chars, "?" = wildcard, spaces separate bytes.
struct SigEntry {
    const char* name;
    const char* pattern;
    int rip_offset;     // offset from pattern match to the RIP-relative displacement
    int instr_size;     // instruction size for RIP resolution (default 7)
    uintptr_t Offsets::*field;
};

static const SigEntry SIG_TABLE[] = {
    // dwGlobalVars — "48 89 15 ?? ?? ?? ?? 48 89 42"
    { "dwGlobalVars",             "48 89 15 ?? ?? ?? ?? 48 89 42", 3, 7, &Offsets::dwGlobalVars },
    // dwEntityList — "48 89 35 ?? ?? ?? ?? 48 85 f6"
    { "dwEntityList",             "48 89 35 ?? ?? ?? ?? 48 85 f6", 3, 7, &Offsets::dwEntityList },
    // dwLocalPlayerController — "48 8b 05 ?? ?? ?? ?? 41 89 be"
    { "dwLocalPlayerController",  "48 8b 05 ?? ?? ?? ?? 41 89 be", 3, 7, &Offsets::dwLocalPlayerController },
    // dwLocalPlayerPawn — "48 8d 05 ?? ?? ?? ?? 48 8b d9 48 85 c0"
    { "dwLocalPlayerPawn",        "48 8d 05 ?? ?? ?? ?? 48 8b d9 48 85 c0", 3, 7, &Offsets::dwLocalPlayerPawn },
    // dwViewMatrix — "48 8d 0d ?? ?? ?? ?? 48 c1 e0 06"
    { "dwViewMatrix",             "48 8d 0d ?? ?? ?? ?? 48 c1 e0 06", 3, 7, &Offsets::dwViewMatrix },
    // dwViewAngles — "48 8d 0d ?? ?? ?? ?? 48 8b 03 48 89 45"
    { "dwViewAngles",             "48 8d 0d ?? ?? ?? ?? 48 8b 03 48 89 45", 3, 7, &Offsets::dwViewAngles },
    // dwGameEntitySystem — "48 8d 0d ?? ?? ?? ?? 48 c1 e0 06 48 03 c0"
    { "dwGameEntitySystem",       "48 8d 0d ?? ?? ?? ?? 48 c1 e0 06 48 03 c0", 3, 7, &Offsets::dwGameEntitySystem },
    // dwPlantedC4 — "48 8b 15 ?? ?? ?? ?? 41 ff c0"
    { "dwPlantedC4",              "48 8b 15 ?? ?? ?? ?? 41 ff c0", 3, 7, &Offsets::dwPlantedC4 },
    // dwCSGOInput — "48 8d 0d ?? ?? ?? ?? 48 8b f8 e8"
    { "dwCSGOInput",              "48 8d 0d ?? ?? ?? ?? 48 8b f8 e8", 3, 7, &Offsets::dwCSGOInput },
    // dwBuildNumber — "48 8b 0d ?? ?? ?? ?? 48 8b 01 ff 50"
    { "dwBuildNumber",            "48 8b 0d ?? ?? ?? ?? 48 8b 01 ff 50", 3, 7, &Offsets::dwBuildNumber },
    // dwGameRules — "48 8b 05 ?? ?? ?? ?? 48 85 c0 74 ?? 8b 88"
    { "dwGameRules",              "48 8b 05 ?? ?? ?? ?? 48 85 c0 74 ?? 8b 88", 3, 7, &Offsets::dwGameRules },
};

static constexpr int SIG_COUNT = sizeof(SIG_TABLE) / sizeof(SIG_TABLE[0]);

static bool match_pattern(const uint8_t* data, const char* sig) {
    size_t len = strlen(sig);
    for (size_t i = 0; i < len; i += 3) {
        if (sig[i] == ' ') { --i; continue; }

        uint8_t nibble = 0;
        char c = sig[i];
        if (c >= '0' && c <= '9') nibble = (c - '0') << 4;
        else if (c >= 'a' && c <= 'f') nibble = (c - 'a' + 10) << 4;
        else if (c >= 'A' && c <= 'F') nibble = (c - 'A' + 10) << 4;
        else if (c == '?') {
            continue;
        } else continue;

        c = sig[i + 1];
        if (c >= '0' && c <= '9') nibble |= (c - '0');
        else if (c >= 'a' && c <= 'f') nibble |= (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') nibble |= (c - 'A' + 10);
        else if (c != '?') return false;

        size_t idx = i / 3;
        if (data[idx] != nibble) return false;
    }
    return true;
}

uintptr_t pattern_scan(const uint8_t* data, size_t size, const char* signature) {
    size_t sig_len = strlen(signature) / 3 + 1;
    for (size_t i = 0; i < size - sig_len; ++i) {
        if (match_pattern(data + i, signature))
            return (uintptr_t)(data + i);
    }
    return 0;
}

uintptr_t pattern_scan_module(uintptr_t base, size_t size, const char* signature) {
    auto buf = memory::read_bytes(base, size);
    if (buf.empty()) return 0;
    uintptr_t rva = pattern_scan(buf.data(), buf.size(), signature);
    if (!rva) return 0;
    return base + (rva - (uintptr_t)buf.data());
}

uintptr_t resolve_rip(uintptr_t addr, uint32_t offset, uint32_t instr_size) {
    int32_t rel = memory::read<int32_t>(addr + offset);
    return addr + instr_size + rel;
}

bool scan_offsets() {
    using namespace memory;

    g_offsets.clientBase = g_moduleBase;
    g_offsets.clientSize = g_moduleSize;

    auto buf = read_bytes(g_moduleBase, g_moduleSize);
    if (buf.empty()) {
        printf("[!] Failed to read client.dll memory\n");
        return false;
    }

    printf("[*] Scanning %zu signatures in client.dll (base: 0x%llX, size: 0x%zX)...\n",
           (size_t)SIG_COUNT, g_moduleBase, g_moduleSize);

    int found = 0;
    int missed = 0;

    for (int i = 0; i < SIG_COUNT; ++i) {
        const auto& sig = SIG_TABLE[i];
        uintptr_t match = pattern_scan(buf.data(), buf.size(), sig.pattern);

        if (match) {
            uintptr_t rva = match - (uintptr_t)buf.data();
            uintptr_t abs_addr = g_moduleBase + rva;
            uintptr_t resolved = resolve_rip(abs_addr, sig.rip_offset, sig.instr_size);
            g_offsets.*(sig.field) = resolved;

            printf("[+] %-28s : 0x%llX (pattern at +0x%llX)\n", sig.name, resolved, rva);
            ++found;
        } else {
            printf("[-] %-28s : NOT FOUND (pattern may be outdated)\n", sig.name);
            ++missed;
        }
    }

    printf("[*] Found: %d / %d offsets\n", found, SIG_COUNT);

    // Critical offsets that MUST be found
    bool critical = g_offsets.dwEntityList != 0
                 && g_offsets.dwLocalPlayerController != 0
                 && g_offsets.dwViewMatrix != 0;

    if (!critical) {
        printf("[!] Critical offsets missing! The game may have updated.\n");
        printf("[!] Check https://github.com/a2x/cs2-dumper for latest offsets.\n");
    }

    return critical;
}

} // namespace cs2::offsets

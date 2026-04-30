#pragma once
#include <cstdio>
#include <cstdarg>
#include <ctime>

// File-based debug logger — disabled
static void debug_log(const char* fmt, ...) {
    (void)fmt;
}

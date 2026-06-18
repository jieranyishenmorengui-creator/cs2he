#pragma once
#include <cstdio>
#include <cstdarg>
#include <ctime>

// Debug logger — writes to stderr (visible in the allocated console)
static void debug_log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[DEBUG] ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

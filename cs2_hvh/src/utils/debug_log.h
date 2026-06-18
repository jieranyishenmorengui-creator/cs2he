#pragma once
#include <cstdio>
#include <cstdarg>
#include <ctime>

// Debug logger — appends to cs2_hvh_debug.txt in the exe directory
static void debug_log(const char* fmt, ...) {
    FILE* f = fopen("cs2_hvh_debug.txt", "a");
    if (!f) return;
    va_list args;
    va_start(args, fmt);
    fprintf(f, "[DEBUG] ");
    vfprintf(f, fmt, args);
    fprintf(f, "\n");
    va_end(args);
    fclose(f);
}

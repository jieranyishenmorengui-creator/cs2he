#pragma once
#include <cstdio>
#include <cstdarg>
#include <ctime>

// File-based debug logger — writes timestamped entries to cs2_hvh_debug.txt
// This survives crashes/hangs since it flushes after every write.
static void debug_log(const char* fmt, ...) {
    FILE* f = fopen("cs2_hvh_debug.txt", "a");
    if (!f) return;

    // Timestamp
    time_t t = time(nullptr);
    struct tm tm;
    localtime_s(&tm, &t);
    fprintf(f, "[%02d:%02d:%02d] ", tm.tm_hour, tm.tm_min, tm.tm_sec);

    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);
    fprintf(f, "\n");
    fflush(f);
    fclose(f);
}

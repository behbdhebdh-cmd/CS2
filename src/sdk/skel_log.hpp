#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <share.h>

// Rate-limited skeleton pipeline logger. Writes a file and a console so bone
// reads / W2S / draw can be traced without flooding the overlay frame loop.
namespace skel_log {

inline FILE* file()
{
    static FILE* fp = nullptr;
    static bool tried = false;
    if (tried)
        return fp;
    tried = true;

    if (GetEnvironmentVariableA("CS2_SKEL_CONSOLE", nullptr, 0) > 0) {
        AllocConsole();
        SetConsoleTitleA("CS2 skeleton log");
        FILE* con = nullptr;
        freopen_s(&con, "CONOUT$", "w", stdout);
    }

    char exe[MAX_PATH]{};
    const DWORD n = GetModuleFileNameA(nullptr, exe, MAX_PATH);
    char path[MAX_PATH]{};
    if (n) {
        strcpy_s(path, exe);
        char* slash = std::strrchr(path, '\\');
        if (slash)
            strcpy_s(slash + 1, static_cast<size_t>(MAX_PATH - (slash + 1 - path)), "skeleton.log");
        fp = _fsopen(path, "w", _SH_DENYNO);
    }
    if (!fp)
        fp = _fsopen("D:\\CS2\\release\\skeleton.log", "w", _SH_DENYNO);
    return fp;
}

inline bool allow()
{
    static DWORD window_ms = 0;
    static int count = 0;
    static int total = 0;
    const DWORD now = GetTickCount();
    if (total < 24)
        return ++total, true;
    if (now - window_ms >= 1000) {
        window_ms = now;
        count = 0;
    }
    return count++ < 6;
}

inline void write(const char* fmt, ...)
{
    if (!allow())
        return;

    char line[512]{};
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);

    std::printf("[skel] %s\n", line);
    if (FILE* fp = file()) {
        std::fprintf(fp, "%s\n", line);
        std::fflush(fp);
    }
}

} // namespace skel_log

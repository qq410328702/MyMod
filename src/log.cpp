#include "log.h"

#include <windows.h>
#include <share.h>     // _SH_DENYNO
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <mutex>

namespace mymod::log_ {

namespace {
    FILE*       g_file  = nullptr;
    int         g_level = Info;
    std::mutex  g_mu;
}

void Init(const char* filename, int level)
{
    std::lock_guard<std::mutex> lock(g_mu);
    g_level = level;

    if (g_file) { fclose(g_file); g_file = nullptr; }
    if (level <= Off) return;

    // 用 _fsopen + _SH_DENYNO 让其他进程能读
    g_file = _fsopen(filename, "w", _SH_DENYNO);
    if (g_file) {
        time_t now = time(nullptr);
        char buf[64];
        ctime_s(buf, sizeof(buf), &now);
        fprintf(g_file, "===== MyMod log started %s", buf);
        fflush(g_file);
    }
}

void Shutdown()
{
    std::lock_guard<std::mutex> lock(g_mu);
    if (g_file) {
        fprintf(g_file, "===== Log closed =====\n");
        fclose(g_file);
        g_file = nullptr;
    }
}

void Write(Level lv, const char* fmt, ...)
{
    std::lock_guard<std::mutex> lock(g_mu);
    if (!g_file || lv > g_level) return;

    static const char* tag[] = { "OFF", "ERR", "INF", "DBG" };

    SYSTEMTIME st;
    GetLocalTime(&st);
    fprintf(g_file, "[%02d:%02d:%02d.%03d %s] ",
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, tag[lv]);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_file, fmt, ap);
    va_end(ap);

    fputc('\n', g_file);
    fflush(g_file);
}

}  // namespace mymod::log_

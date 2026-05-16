// ═══════════════════════════════════════════════════════════════
//  MyMod 主入口
// ───────────────────────────────────────────────────────────────
//  生命周期：
//    1. 游戏进程加载本 DLL（HD Mod 通过 Packs 机制）
//    2. DllMain 收到 DLL_PROCESS_ATTACH
//    3. 找到游戏根目录（取本 DLL 所在目录的上两级）
//    4. 加载 MyMod.ini，初始化日志器
//    5. 安装所有 hook
//    6. 控制权交还给游戏，hook 在游戏运行时被触发
//    7. 退出时收到 DLL_PROCESS_DETACH，卸载所有 hook
// ═══════════════════════════════════════════════════════════════

#include "config.h"
#include "log.h"
#include "hooks/hooks.h"

#include <windows.h>
#include <cstring>

namespace {

HMODULE g_self = nullptr;
bool    g_initialized = false;

// 取本 DLL 所在目录
void GetDllDir(char* out, size_t cap)
{
    GetModuleFileNameA(g_self, out, static_cast<DWORD>(cap));
    // 去掉文件名
    char* slash = strrchr(out, '\\');
    if (slash) *slash = '\0';
}

// 取游戏根目录
//   本 DLL 路径形如：
//     <Game>\_HD3_Data\Packs\MyMod\MyMod.dll
//   往上两级是 _HD3_Data\Packs，再往上一级就是游戏根目录。
//   不过最稳的做法是：取主进程 EXE 的目录。
void GetGameRoot(char* out, size_t cap)
{
    GetModuleFileNameA(nullptr, out, static_cast<DWORD>(cap));  // 主 EXE 路径
    char* slash = strrchr(out, '\\');
    if (slash) *slash = '\0';
}

void Initialize(HMODULE self)
{
    if (g_initialized) return;
    g_initialized = true;
    g_self = self;

    // ─── 1. 找到 MyMod.ini ────────────────────────────────
    // 优先：DLL 同目录
    char iniPath[MAX_PATH] = {};
    GetDllDir(iniPath, sizeof(iniPath));
    strcat_s(iniPath, sizeof(iniPath), "\\MyMod.ini");

    if (GetFileAttributesA(iniPath) == INVALID_FILE_ATTRIBUTES) {
        // 退而求其次：游戏根目录
        GetGameRoot(iniPath, sizeof(iniPath));
        strcat_s(iniPath, sizeof(iniPath), "\\MyMod.ini");
    }

    mymod::config::Load(iniPath);
    const auto& cfg = mymod::config::Get();

    // ─── 2. 初始化日志（写到游戏根目录） ──────────────────
    char logPath[MAX_PATH] = {};
    GetGameRoot(logPath, sizeof(logPath));
    strcat_s(logPath, sizeof(logPath), "\\");
    strcat_s(logPath, sizeof(logPath), cfg.LogFile);

    mymod::log_::Init(logPath, cfg.LogLevel);

    LOG_INFO("MyMod starting up");
    LOG_INFO("INI: %s", iniPath);
    LOG_INFO("Log: %s (level=%d)", logPath, cfg.LogLevel);
    LOG_INFO("Config: ShowStartupMessage=%d  EnableMessageBoxHook=%d  "
             "EnableMovementMod=%d",
             cfg.ShowStartupMessage, cfg.EnableMessageBoxHook,
             cfg.EnableMovementMod);

    // ─── 3. 安装 hook ─────────────────────────────────────
    using namespace mymod::hooks;
    Install_StartupMessage();
    Install_MessageBoxIATHook();
    Install_MovementHook();
    Install_CreatureColor();

    LOG_INFO("All hooks processed");
}

void Shutdown()
{
    if (!g_initialized) return;
    LOG_INFO("MyMod shutting down");
    mymod::hooks::UninstallAll();
    mymod::log_::Shutdown();
}

}  // namespace

// ═══════════════════════════════════════════════════════════
//  DllMain
// ═══════════════════════════════════════════════════════════
BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID /*reserved*/)
{
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            // 关掉 thread attach/detach 通知，提速
            DisableThreadLibraryCalls(hMod);
            try {
                Initialize(hMod);
            } catch (...) {
                // DllMain 里抛异常会让游戏崩溃 —— 一律吞掉
                MessageBoxA(nullptr, "MyMod init threw exception",
                            "MyMod ERROR", MB_OK | MB_ICONERROR);
            }
            break;

        case DLL_PROCESS_DETACH:
            try { Shutdown(); } catch (...) {}
            break;

        // 已禁用，不会触发：
        // case DLL_THREAD_ATTACH:
        // case DLL_THREAD_DETACH:
        default:
            break;
    }
    return TRUE;
}

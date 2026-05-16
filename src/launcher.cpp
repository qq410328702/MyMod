// ═══════════════════════════════════════════════════════════════
//  MyMod_launcher.exe
// ───────────────────────────────────────────────────────────────
//  启动 H3/HotA 并把 MyMod.dll 注入进去。
//  这个 launcher 是独立的小 EXE，不修改任何游戏文件。
//
//  原理：
//    1. CreateProcess(SUSPENDED) 启动游戏，但暂停在第一条指令前
//    2. 在游戏进程里 VirtualAllocEx 分配一段内存写 DLL 路径
//    3. CreateRemoteThread 起一个远线程调用 LoadLibraryA(DLL路径)
//    4. 等远线程结束 → DLL 已经注入并运行了 DllMain
//    5. ResumeThread 让游戏继续执行
//
//  这是最干净的注入方式，HD Mod / HotA / 杀毒软件都不会反对，
//  也不需要改任何系统 DLL。
// ═══════════════════════════════════════════════════════════════

#include <windows.h>
#include <cstdio>
#include <cstring>

static void Die(const char* msg, DWORD code = 0)
{
    char buf[512];
    if (code) {
        snprintf(buf, sizeof(buf), "%s\n\nWindows error: %lu", msg, code);
    } else {
        snprintf(buf, sizeof(buf), "%s", msg);
    }
    MessageBoxA(nullptr, buf, "MyMod Launcher", MB_OK | MB_ICONERROR);
    ExitProcess(1);
}

// 把 dll 注入到目标进程
static bool InjectDll(HANDLE hProc, const char* dllPath)
{
    size_t len = strlen(dllPath) + 1;

    // 在目标进程分配存放 DLL 路径的内存
    void* remoteBuf = VirtualAllocEx(hProc, nullptr, len,
                                     MEM_COMMIT | MEM_RESERVE,
                                     PAGE_READWRITE);
    if (!remoteBuf) return false;

    if (!WriteProcessMemory(hProc, remoteBuf, dllPath, len, nullptr)) {
        VirtualFreeEx(hProc, remoteBuf, 0, MEM_RELEASE);
        return false;
    }

    // LoadLibraryA 在 kernel32 中的地址在 ASLR 下与目标进程相同（系统 DLL 共享）
    HMODULE k32 = GetModuleHandleA("kernel32.dll");
    auto    pLL = GetProcAddress(k32, "LoadLibraryA");
    if (!pLL) {
        VirtualFreeEx(hProc, remoteBuf, 0, MEM_RELEASE);
        return false;
    }

    HANDLE hThread = CreateRemoteThread(
        hProc, nullptr, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(pLL),
        remoteBuf, 0, nullptr);

    if (!hThread) {
        VirtualFreeEx(hProc, remoteBuf, 0, MEM_RELEASE);
        return false;
    }

    // 等远线程结束（即 LoadLibrary 完成）
    WaitForSingleObject(hThread, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);

    CloseHandle(hThread);
    VirtualFreeEx(hProc, remoteBuf, 0, MEM_RELEASE);

    return exitCode != 0;  // LoadLibraryA 返回 HMODULE，0 = 失败
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR cmdLine, int)
{
    // 取自身所在目录
    char selfDir[MAX_PATH];
    GetModuleFileNameA(nullptr, selfDir, sizeof(selfDir));
    char* slash = strrchr(selfDir, '\\');
    if (slash) *slash = '\0';

    // 默认游戏 EXE
    char gameExe[MAX_PATH];
    snprintf(gameExe, sizeof(gameExe),
             "%s\\h3hota HD（深渊号角HD版）.exe", selfDir);

    if (GetFileAttributesA(gameExe) == INVALID_FILE_ATTRIBUTES) {
        // 退一步：HotA 主程序
        snprintf(gameExe, sizeof(gameExe), "%s\\h3hota.exe", selfDir);
    }
    if (GetFileAttributesA(gameExe) == INVALID_FILE_ATTRIBUTES) {
        Die("Cannot find h3hota.exe in game directory.\n"
            "Make sure MyMod_launcher.exe is in the H3/HotA folder.");
    }

    // 找 MyMod.dll
    char dllPath[MAX_PATH];
    snprintf(dllPath, sizeof(dllPath), "%s\\MyMod.dll", selfDir);
    if (GetFileAttributesA(dllPath) == INVALID_FILE_ATTRIBUTES) {
        char msg[512];
        snprintf(msg, sizeof(msg),
                 "MyMod.dll not found at:\n%s\n\n"
                 "Run build.bat first, then copy MyMod.dll to the game folder.",
                 dllPath);
        Die(msg);
    }

    // 启动游戏（暂停状态）
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};

    char cmdBuf[MAX_PATH * 2];
    snprintf(cmdBuf, sizeof(cmdBuf), "\"%s\" %s", gameExe, cmdLine);

    if (!CreateProcessA(
            gameExe,                  // application
            cmdBuf,                   // command line
            nullptr, nullptr,
            FALSE,
            CREATE_SUSPENDED,         // 关键：暂停启动
            nullptr,
            selfDir,                  // working directory
            &si, &pi)) {
        Die("CreateProcess failed", GetLastError());
    }

    // 注入 DLL
    if (!InjectDll(pi.hProcess, dllPath)) {
        DWORD err = GetLastError();
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        Die("DLL injection failed", err);
    }

    // 让游戏继续运行
    ResumeThread(pi.hThread);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 0;
}

// ═══════════════════════════════════════════════════════════════
//  示例 1：启动消息框
// ───────────────────────────────────────────────────────────────
//  这是最简单的"证明插件被加载"的办法——直接 MessageBoxA。
//  不涉及 hook，不依赖 patcher_x86，只是确认 DLL 真的进了进程。
//
//  注意：用户用熟后应该把 ShowStartupMessage = 0 关掉。
// ═══════════════════════════════════════════════════════════════

#include "hooks.h"
#include "../config.h"
#include "../log.h"

#include <windows.h>

namespace mymod::hooks {

void Install_StartupMessage()
{
    if (!config::Get().ShowStartupMessage) {
        LOG_DEBUG("Install_StartupMessage: skipped (disabled in INI)");
        return;
    }

    // 不能用 MB_OK 阻塞太久 —— 游戏可能正在初始化
    // 用一个独立线程，避免卡 DllMain
    HANDLE h = CreateThread(nullptr, 0,
        [](LPVOID) -> DWORD {
            // 等游戏窗口完全起来再弹（不然会被全屏覆盖）
            Sleep(2000);
            MessageBoxA(nullptr,
                        "[MyMod] DLL loaded!\n\n"
                        "If you see this, the plugin was injected successfully.\n"
                        "Set ShowStartupMessage=0 in MyMod.ini to suppress.",
                        "MyMod",
                        MB_OK | MB_ICONINFORMATION
                          | MB_TOPMOST          // 永远在最前
                          | MB_SETFOREGROUND    // 抢焦点
                          | MB_SYSTEMMODAL);    // 系统模态，全屏游戏也压不住
            return 0;
        },
        nullptr, 0, nullptr);

    if (h) CloseHandle(h);
    LOG_INFO("Install_StartupMessage: done");
}

}  // namespace mymod::hooks

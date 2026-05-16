// ═══════════════════════════════════════════════════════════════
//  示例 2：IAT Hook
// ───────────────────────────────────────────────────────────────
//  Import Address Table（IAT）hook 是最稳的"替换 Win32 API"方法。
//  原理：
//    PE 文件加载时，Windows 把每个被导入的 API（如 user32!MessageBoxA）
//    的实际地址填到目标进程的 IAT 表里。
//    我们扫描这张表，找到 MessageBoxA 那一项，把指针改成自己的函数。
//    游戏调用 MessageBoxA 时实际跳到我们这里，我们再选择是否调原函数。
//
//  这个 hook 完全不依赖 patcher_x86，纯 Win32 技术。
//  适合：拦截 user32/kernel32/winmm 等系统 DLL 的 API。
//  不适合：拦截游戏内部 C++ 函数（用 patcher_x86 的 HiHook）。
// ═══════════════════════════════════════════════════════════════

#include "hooks.h"
#include "../config.h"
#include "../log.h"

#include <windows.h>
#include <cstring>

namespace mymod::hooks {

namespace {

// 保存原 MessageBoxA 指针
using MessageBoxA_t = int (WINAPI*)(HWND, LPCSTR, LPCSTR, UINT);
MessageBoxA_t g_origMessageBoxA = nullptr;

// 我们的替换函数
int WINAPI MyMessageBoxA(HWND hWnd, LPCSTR text, LPCSTR caption, UINT type)
{
    LOG_DEBUG("MessageBoxA intercepted: caption=%s text=%s",
              caption ? caption : "(null)",
              text    ? text    : "(null)");

    // 给标题加前缀
    char newCaption[512];
    snprintf(newCaption, sizeof(newCaption), "[MyMod hook] %s",
             caption ? caption : "");

    // 调原函数
    return g_origMessageBoxA(hWnd, text, newCaption, type);
}

// 在指定模块的 IAT 中，把 dllName::funcName 替换为 newFunc
// 返回原函数指针（成功）或 nullptr（失败）
void* HookIAT(HMODULE module, const char* dllName,
              const char* funcName, void* newFunc)
{
    auto base = reinterpret_cast<BYTE*>(module);
    auto dos  = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto nt   = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);

    DWORD impDirRVA = nt->OptionalHeader
        .DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if (!impDirRVA) return nullptr;

    auto desc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + impDirRVA);

    for (; desc->Name; ++desc) {
        const char* name = reinterpret_cast<const char*>(base + desc->Name);
        if (_stricmp(name, dllName) != 0) continue;

        // OriginalFirstThunk 提供函数名，FirstThunk 是实际地址
        auto names = reinterpret_cast<IMAGE_THUNK_DATA*>(
            base + desc->OriginalFirstThunk);
        auto addrs = reinterpret_cast<IMAGE_THUNK_DATA*>(
            base + desc->FirstThunk);

        for (; names->u1.AddressOfData; ++names, ++addrs) {
            if (names->u1.Ordinal & IMAGE_ORDINAL_FLAG) continue;  // 按序号导入跳过

            auto byName = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(
                base + names->u1.AddressOfData);
            if (strcmp(reinterpret_cast<const char*>(byName->Name),
                       funcName) != 0) continue;

            // 改写 IAT 槽位
            DWORD oldProt;
            if (!VirtualProtect(&addrs->u1.Function, sizeof(void*),
                                PAGE_READWRITE, &oldProt)) {
                LOG_ERROR("VirtualProtect failed: %u", GetLastError());
                return nullptr;
            }
            void* orig = reinterpret_cast<void*>(addrs->u1.Function);
            addrs->u1.Function = reinterpret_cast<DWORD_PTR>(newFunc);
            VirtualProtect(&addrs->u1.Function, sizeof(void*), oldProt, &oldProt);

            LOG_INFO("IAT hook installed: %s::%s old=%p new=%p",
                     dllName, funcName, orig, newFunc);
            return orig;
        }
    }
    return nullptr;
}

}  // namespace

void Install_MessageBoxIATHook()
{
    if (!config::Get().EnableMessageBoxHook) {
        LOG_DEBUG("Install_MessageBoxIATHook: skipped (disabled in INI)");
        return;
    }

    // hook 主进程模块（h3hota.exe）的 IAT
    HMODULE main = GetModuleHandleA(nullptr);
    void* orig = HookIAT(main, "user32.dll", "MessageBoxA", &MyMessageBoxA);
    if (orig) {
        g_origMessageBoxA = reinterpret_cast<MessageBoxA_t>(orig);
        LOG_INFO("Install_MessageBoxIATHook: success");
    } else {
        LOG_ERROR("Install_MessageBoxIATHook: failed to find MessageBoxA in IAT");
    }
}

}  // namespace mymod::hooks

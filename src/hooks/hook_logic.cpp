// ═══════════════════════════════════════════════════════════════
//  示例 3：用 H3API 写真正的游戏逻辑 hook
// ───────────────────────────────────────────────────────────────
//  本文件演示一个完整的 hook 流程：
//    1. 从 H3API 获得函数地址（H3Hero::CalcMaxMovement = 0x4E5000）
//    2. 用 patcher_x86 的 HiHook + SPLICE_ + EXTENDED_ 替换它
//    3. 在替换函数里：先调原函数拿原值，再写日志
//    4. 通过 LooksLikeFunction() 自我保护，地址不对时跳过 hook
//
//  H3API 提供的地址基于 H3 SoD 3.2，HotA 1.8 大概率不动 H3 核心函数地址。
//  如果发现 hook 不触发，请用 IDA 对照 0x4E5000 处的字节验证。
// ═══════════════════════════════════════════════════════════════

#include "hooks.h"
#include "../config.h"
#include "../log.h"
#include "../pattern.h"
#include "../mymod_h3api.h"

#include <windows.h>

namespace mymod::hooks {

namespace {

// patcher_x86 实例
::Patcher*         g_patcher = nullptr;
::PatcherInstance* g_pi      = nullptr;

// 已注册的 hook
::HiHook*          g_hkHeroCalcMove = nullptr;


// ═══════════════════════════════════════════════════════════
//  H3Hero::CalcMaxMovement 的真实地址
// ───────────────────────────────────────────────────────────
//  来源：H3API single_header/H3API.hpp 第 30130 行
//        return THISCALL_1(INT32, 0x4E5000, this);
//
//  这是 H3 SoD 3.2 的地址。HotA 1.8 通常不动核心数学函数地址。
//  如果验证后发现 HotA 改了，去 hota_addresses.h 加上备选地址。
// ═══════════════════════════════════════════════════════════
constexpr DWORD ADDR_Hero_CalcMaxMovement = 0x004E5000;


// 原函数签名：int __thiscall H3Hero::CalcMaxMovement(this);
using OrigCalcMove_t = int (__thiscall*)(h3::H3Hero* self);


// ═══════════════════════════════════════════════════════════
//  替换函数
// ───────────────────────────────────────────────────────────
//  EXTENDED_ 模式：第 1 个参数是 HiHook*，之后是原函数的参数
//  原函数是 __thiscall（this 在 ecx）
//
//  我们这里不修改返回值，只调原函数 + 打日志。
//  这样最安全：万一 H3API 的地址错了，唯一的副作用是 log 不出现。
// ═══════════════════════════════════════════════════════════
int __stdcall MyCalcMaxMovement(::HiHook* h, h3::H3Hero* hero)
{
    auto orig    = reinterpret_cast<OrigCalcMove_t>(h->GetDefaultFunc());
    int  result  = orig(hero);

    // ─── 第一次触发时延迟 dump 生物数组 ────────────────
    // DllMain 太早了，那时 0x6747B0 指向的可能是空表
    // 进游戏后这个 hook 触发说明引擎完全初始化好了
    static bool firstTime = true;
    if (firstTime) {
        firstTime = false;
        LOG_INFO("");
        LOG_INFO("════════════════════════════════════════════════════════");
        LOG_INFO("  GAME RUNNING: re-dump creature array (delayed)");
        LOG_INFO("════════════════════════════════════════════════════════");
        if (config::Get().EnableCreatureColor) {
            mymod::hooks::DumpCreatureArrayDelayed();
        }
    }

    // 限流：避免每次 UI 重绘都刷一行（这个函数可能被频繁调用）
    static int counter = 0;
    if (++counter <= 30) {
        // 英雄名是 GBK (936) 编码 —— HotA 中文版是这样
        // 不能用 CP_ACP，因为 Win10/11 启用 UTF-8 时 ACP=65001
        // 必须显式指定 936
        char nameUtf8[64] = {};
        wchar_t wbuf[16] = {};
        int wlen = MultiByteToWideChar(936, 0, hero->name, -1, wbuf, 16);
        if (wlen > 0) {
            WideCharToMultiByte(CP_UTF8, 0, wbuf, -1,
                                nameUtf8, sizeof(nameUtf8), nullptr, nullptr);
        }

        // hero 字段名/偏移来自 H3API 中 struct H3Hero 的定义
        LOG_INFO("MaxMove[%d]: hero='%s' lvl=%d owner=%d "
                 "spellPts=%d move=%d/%d -> calc=%d",
                 counter,
                 nameUtf8[0] ? nameUtf8 : hero->name,
                 (int)hero->level,   // INT16
                 (int)hero->owner,   // INT8
                 (int)hero->spellPoints,
                 hero->movement,
                 hero->maxMovement,
                 result);

        if (counter == 30) {
            LOG_INFO("(further CalcMaxMovement calls suppressed)");
        }
    }

    return result;
}


// ═══════════════════════════════════════════════════════════
//  地址有效性检查（自我保护）
// ═══════════════════════════════════════════════════════════
bool LooksLikeFunction(DWORD addr)
{
    if (addr < 0x00400000 || addr > 0x10000000) return false;
    auto p = reinterpret_cast<const BYTE*>(addr);
    __try {
        BYTE b0 = p[0];
        // 函数序言常见字节：push ebp / push ebx / push esi / push edi /
        //                    sub esp,? / sub esp,? (long) / mov xxx
        return b0 == 0x55 || b0 == 0x53 || b0 == 0x56 || b0 == 0x57 ||
               b0 == 0x83 || b0 == 0x81 || b0 == 0x8B || b0 == 0x6A;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}


// ═══════════════════════════════════════════════════════════
//  H3API 兼容性自检
// ═══════════════════════════════════════════════════════════
void VerifyH3APICompat()
{
    // 注意：在 DllMain 时机调用 P_Game 通常返回 null —— 这是正常的，
    //   全局对象 H3Main 实例只在游戏启动到地图阶段才被构造和填充。
    //   如果你想在游戏内验证，hook 进游戏后的某个函数再调用本函数。
    h3::H3Main* game = P_Game;
    if (!game) {
        LOG_INFO("H3API compat: P_Game() = null  (expected at DllMain stage, "
                 "the global is filled only after entering a map)");
    } else {
        LOG_INFO("H3API compat: P_Game() = %p", game);
        LOG_INFO("H3API compat: date = day %d / week %d / month %d",
                 (int)game->date.day, (int)game->date.week, (int)game->date.month);
    }

    UINT32 cur = P_CurrentPlayerID;
    LOG_INFO("H3API compat: current player ID = %u", cur);

    // 校验函数地址处是否真有指令
    BYTE* p = reinterpret_cast<BYTE*>(ADDR_Hero_CalcMaxMovement);
    LOG_INFO("H3API compat: bytes at 0x%X = %02X %02X %02X %02X %02X %02X",
             ADDR_Hero_CalcMaxMovement,
             p[0], p[1], p[2], p[3], p[4], p[5]);
}

}  // namespace


// ═══════════════════════════════════════════════════════════
//  对外的安装入口
// ═══════════════════════════════════════════════════════════
void Install_MovementHook()
{
    if (!config::Get().EnableMovementMod) {
        LOG_DEBUG("Install_MovementHook: skipped (disabled in INI)");
        return;
    }

    // 1. 拿 patcher_x86 单例
    g_patcher = ::GetPatcher();
    if (!g_patcher) {
        LOG_ERROR("patcher_x86.dll not found in process. "
                  "Make sure HotA / HD Mod is properly installed.");
        return;
    }

    g_pi = g_patcher->CreateInstance("MyMod.Movement");
    if (!g_pi) {
        LOG_ERROR("CreateInstance failed (name already in use?)");
        return;
    }
    LOG_INFO("patcher_x86 instance: MyMod.Movement");

    // 2. 自检
    VerifyH3APICompat();

    // 3. 安装 hook
    if (!LooksLikeFunction(ADDR_Hero_CalcMaxMovement)) {
        LOG_ERROR("ADDR_Hero_CalcMaxMovement (0x%08X) doesn't look like a "
                  "function in this EXE. Skipping hook to avoid crash.",
                  ADDR_Hero_CalcMaxMovement);
        return;
    }

    g_hkHeroCalcMove = g_pi->WriteHiHook(
        ADDR_Hero_CalcMaxMovement,
        SPLICE_,        // 替换函数本体
        EXTENDED_,      // 第一参数是 HiHook*
        THISCALL_,      // 原函数是 __thiscall
        reinterpret_cast<void*>(MyCalcMaxMovement)
    );

    if (g_hkHeroCalcMove) {
        LOG_INFO("✅ HiHook(CalcMaxMovement) installed at 0x%08X",
                 ADDR_Hero_CalcMaxMovement);
    } else {
        LOG_ERROR("❌ HiHook(CalcMaxMovement) install FAILED at 0x%08X",
                  ADDR_Hero_CalcMaxMovement);
    }
}


void UninstallAll()
{
    if (g_pi) {
        g_pi->DestroyAll();
        LOG_INFO("All patches destroyed");
        g_pi = nullptr;
    }
    g_hkHeroCalcMove = nullptr;
}

}  // namespace mymod::hooks

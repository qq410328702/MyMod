// ═══════════════════════════════════════════════════════════════
//  示例 3：用 H3API 写真正的游戏逻辑 hook
// ───────────────────────────────────────────────────────────────
//  本文件演示如何使用 H3API 提供的 SDK：
//    - 用 H3Hero* / H3Town* 等强类型替代 void*
//    - 用 H3API 自带的 Patcher / PatcherInstance / HiHook
//    - 用 H3API 的"已知地址"替代手抄常量
//
//  重要：
//    H3API 是基于 H3 SoD 3.2 逆向的。HotA 1.8 在此基础上加了大量 hook，
//    但 *基础类的字段布局* 大部分仍然有效。
//    具体函数地址可能与 HotA EXE 不同，需要：
//      ① 用 IDA 验证地址
//      ② 或用模式扫描动态查找
//      ③ 或维护一份 hota_addresses.h 集中管理
// ═══════════════════════════════════════════════════════════════

#include "hooks.h"
#include "../config.h"
#include "../log.h"
#include "../pattern.h"
#include "../mymod_h3api.h"

#include <windows.h>

namespace mymod::hooks {

namespace {

// patcher_x86 实例（注意：现在用的是 H3API 自带的、在全局命名空间的版本）
::Patcher*         g_patcher = nullptr;
::PatcherInstance* g_pi      = nullptr;

// 已注册的 hook
::HiHook*          g_hkHeroDailyMove = nullptr;


// ═══════════════════════════════════════════════════════════
//  示例 A: hook 英雄"每日移动力计算"（伪示例，演示语法）
// ───────────────────────────────────────────────────────────
//  H3 中英雄每日获得的移动力由一个 __thiscall 函数计算。
//
//  H3API 在 H3Hero 类里提供了 GetMaxMovement() 方法（基于已知地址）。
//  但具体的"日初始化"函数地址可能各版本不同，这里演示思路。
//
//  ⚠️ 地址 0x4E2B70 是占位符 —— 需要你用 IDA 自己确认！
// ═══════════════════════════════════════════════════════════

// 原函数签名（假设）：int __thiscall calcDailyMovement(H3Hero* self);
using OrigCalcMove_t = int (__thiscall*)(h3::H3Hero* self);

int __stdcall MyDailyMovement(::HiHook* h, h3::H3Hero* hero)
{
    // 1. 调原函数（含其他 mod 链上的修改）
    auto orig      = reinterpret_cast<OrigCalcMove_t>(h->GetDefaultFunc());
    int  origValue = orig(hero);

    // 2. 现在 hero 是强类型 H3Hero*，能直接看字段
    //    （这些字段名/偏移来自 H3API 的逆向结果，详见 H3API.hpp 中
    //     `struct H3Hero` 的定义；本例字段都来自 H3 SoD 3.2，
    //     HotA 1.8 中绝大部分字段位置不变）
    LOG_DEBUG("Daily movement: hero='%s' lvl=%d owner=%d "
              "movement=%d/%d origReturn=%d",
              hero->name,         // H3Hero::name (CHAR[13])
              hero->level,        // H3Hero::level (INT16)
              hero->owner,        // H3Hero::owner (INT8, player color)
              hero->movement,     // 当前移动力
              hero->maxMovement,  // 最大移动力
              origValue);

    // 3. 改返回值：示例不真改，留作参考
    // return origValue * 2;

    return origValue;
}


// ═══════════════════════════════════════════════════════════
//  示例 B: 校验 H3API 类型布局是否对得上当前 EXE
// ───────────────────────────────────────────────────────────
//  H3API 是基于 H3 SoD 3.2 的，HotA 1.8 可能改了某些类的字段。
//  在 DllMain 时检查关键全局对象指针是否合理，能尽早发现问题。
// ═══════════════════════════════════════════════════════════
void VerifyH3APICompat()
{
    // H3API 提供 P_Game()、P_ActivePlayer()、P_CurrentPlayerID() 等
    // 访问全局对象的宏。这些宏内部固化了 H3 SoD 3.2 的全局对象地址。
    //
    // P_Game 是 H3DataPointer<H3Main> 智能指针对象（不可拷贝），
    // 用 -> 访问字段，或显式转换为 H3Main*。

    // 把智能指针转成裸指针，方便传参/比较
    // 注意：P_Game 是宏，已经包含 h3:: 前缀，不要再加
    h3::H3Main* game = P_Game;   // 利用 operator pointer() 隐式转换

    if (!game) {
        LOG_ERROR("H3API compat: P_Game() = null. EXE address probably differs from H3 SoD 3.2.");
        return;
    }
    LOG_INFO("H3API compat: P_Game() = %p", game);

    // H3Main::date 是嵌套 struct，包含 day / week / month
    // 不在游戏内时这些值可能是 0
    LOG_INFO("H3API compat: date = day %d / week %d / month %d",
             (int)game->date.day, (int)game->date.week, (int)game->date.month);

    // 当前玩家 ID 是单独的全局 UINT32
    // P_CurrentPlayerID 也是宏，已含 h3:: 前缀
    UINT32 cur = P_CurrentPlayerID;
    LOG_INFO("H3API compat: current player ID = %u", cur);
}


// ═══════════════════════════════════════════════════════════
//  地址（占位符）
// ───────────────────────────────────────────────────────────
//  HotA 1.8 真实地址需要你用 IDA 找。这里给的值仅用于演示流程：
//  代码会检查 LooksLikeFunction()，对不上就跳过 hook，不会崩。
// ═══════════════════════════════════════════════════════════
constexpr DWORD HOTA_HERO_DAILY_MOVE = 0x004E2B70;  // 占位符


bool LooksLikeFunction(DWORD addr)
{
    if (addr < 0x00400000 || addr > 0x10000000) return false;
    auto p = reinterpret_cast<const BYTE*>(addr);
    __try {
        BYTE b0 = p[0];
        // 常见函数序言
        return b0 == 0x55 || b0 == 0x53 || b0 == 0x56 || b0 == 0x57 ||
               b0 == 0x83 || b0 == 0x81 || b0 == 0x8B || b0 == 0x6A;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
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

    // 1. 加载 patcher_x86.dll 并拿到单例
    //    注意：这里是 H3API 提供的 ::GetPatcher()，不是手写版本
    g_patcher = ::GetPatcher();
    if (!g_patcher) {
        LOG_ERROR("patcher_x86.dll not found in process - "
                  "is the launcher injecting MyMod into HotA?");
        return;
    }

    g_pi = g_patcher->CreateInstance("MyMod.Movement");
    if (!g_pi) {
        LOG_ERROR("CreateInstance failed (name already in use?)");
        return;
    }
    LOG_INFO("patcher_x86 instance created: MyMod.Movement");

    // 2. 校验 H3API 与当前 EXE 的兼容性
    VerifyH3APICompat();

    // 3. 安装 hook（地址占位符，对不上时跳过不崩）
    if (LooksLikeFunction(HOTA_HERO_DAILY_MOVE)) {
        g_hkHeroDailyMove = g_pi->WriteHiHook(
            HOTA_HERO_DAILY_MOVE,
            SPLICE_,        // 替换函数本体
            EXTENDED_,      // 第一参数是 HiHook*
            THISCALL_,      // 原函数是 __thiscall
            reinterpret_cast<void*>(MyDailyMovement)
        );
        if (g_hkHeroDailyMove) {
            LOG_INFO("HiHook(DailyMovement) installed at 0x%08X",
                     HOTA_HERO_DAILY_MOVE);
        } else {
            LOG_ERROR("HiHook(DailyMovement) install FAILED at 0x%08X",
                      HOTA_HERO_DAILY_MOVE);
        }
    } else {
        LOG_INFO("Skipping HiHook(DailyMovement): 0x%08X doesn't look like "
                 "a function in this EXE — placeholder address, expected. "
                 "Find the real address with IDA and update HOTA_HERO_DAILY_MOVE.",
                 HOTA_HERO_DAILY_MOVE);
    }

    // 4. 模式扫描示范（仅记录，不创建 hook）
    HMODULE main = GetModuleHandleA(nullptr);
    if (main) {
        // 标准函数序言: push ebp; mov ebp,esp; sub esp,??
        void* p = mymod::pattern::FindIDA(main, "55 8B EC 83 EC ?? 56 57");
        LOG_DEBUG("Pattern-scan demo: first match at %p", p);
    }
}


// ═══════════════════════════════════════════════════════════
//  统一卸载
// ═══════════════════════════════════════════════════════════
void UninstallAll()
{
    if (g_pi) {
        g_pi->DestroyAll();   // 回滚所有补丁，patcher_x86 处理链式 hook 的兼容
        LOG_INFO("All patches destroyed");
        g_pi = nullptr;
    }
    g_hkHeroDailyMove = nullptr;
}

}  // namespace mymod::hooks

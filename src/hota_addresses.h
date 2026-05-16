#pragma once
//
// ═══════════════════════════════════════════════════════════════
//  HotA 1.8 EXE 地址表
// ───────────────────────────────────────────────────────────────
//  H3API 给的是 H3 SoD 3.2 的地址，HotA 1.8 的某些函数地址不一样。
//  本文件集中维护"已经验证过对得上 HotA 1.8 的地址"。
//
//  ▌ 每个地址都需要：
//    1. 在 IDA 中找到并验证
//    2. 注释里写清楚怎么找的（哪个字符串/什么特征）
//    3. 验证过的标 ✅，未验证的标 ⚠️
//
//  ▌ 怎么找新地址（详见 HotA二次开发实战指南.md §4）：
//    1. 用 IDA Free 打开 j:\HOMM3 HotA1.80 汉化正式版1.2\h3hota.exe
//    2. Shift+F12 → 搜字符串
//    3. 找到引用，按 X 看 caller
//    4. 把函数首地址填到下面，标注怎么找的
//
//  ▌ 跨版本兼容：
//    将来 HotA 出 1.9 时，本文件可能整体失效。
//    更鲁棒的方案是 mymod::pattern::FindIDA(...) 做模式扫描。
// ═══════════════════════════════════════════════════════════════

#include <windows.h>

namespace mymod::hota18 {

// ─── 全局对象（H3API 地址在 HotA 1.8 是否还对得上） ──────
// H3API 中 P_Game = 0x699538（来自 H3 SoD 3.2）
// HotA 在很多版本中保留了这个地址。
// VerifyH3APICompat() 在运行时会校验。

// ─── 函数地址（占位符，待你用 IDA 验证后填充） ────────────
// 命名规则：
//   - 前缀 ADDR_ 表示函数地址
//   - 后跟 类名_方法名 的 PascalCase
//   - 注释写参数签名 + IDA 找到的方式

// 占位：英雄每日移动力计算
// 签名（推测）：int __thiscall H3Hero::CalcMaxMovement(this)
// IDA 查找：搜字符串 "Movement"，或 H3API 中 H3Hero::CalcMaxMovement 的地址（H3API 已知）
constexpr DWORD ADDR_Hero_CalcMaxMovement = 0x004E2B70;  // ⚠️ 待验证

// 占位：玩家给资源
// 签名（推测）：void __thiscall H3Player::GiveResources(this, type, amount)
// IDA 查找：搜字符串 "wood"/"ore"/"gold"，找到资源相关函数
constexpr DWORD ADDR_Player_GiveResource = 0x00000000;  // ⚠️ 待填

// 占位：游戏开始时初始化玩家资源
// IDA 查找：在新游戏初始化流程里搜立即数 7500（默认起始金币）
constexpr DWORD ADDR_NewGame_InitResources = 0x00000000;  // ⚠️ 待填

// ─── 模式签名（pattern scan 用） ──────────────────────────
// 这些用于 EXE 版本不同时动态查找函数。
// 每条签名要够独特——至少 10 字节，含函数序言 + 几个特征指令。
// 用 mymod::pattern::FindIDA(GetModuleHandleA(nullptr), SIG_xxx) 调用。

// 占位签名：标准函数序言 + 一些特征
inline constexpr const char* SIG_Hero_CalcMaxMovement =
    "55 8B EC 83 EC ?? 53 56 57 8B F1";  // ⚠️ 待替换为真实签名


// ─── EXE 版本检测 ──────────────────────────────────────────
// HotA 在 EXE 头部某处保存了版本字符串，可用于判断当前是否 1.8.0
// 这里给个粗略判定：检查 .exe 文件大小 / 时间戳 / 某关键地址特征

inline bool LooksLikeHotA180(HMODULE module)
{
    if (!module) return false;
    // 简单检查：模块基址 + 已知函数地址处是否能读到合理字节
    auto base = reinterpret_cast<BYTE*>(module);
    // 这里可加更多检查——目前只是占位
    return base != nullptr;
}

}  // namespace mymod::hota18

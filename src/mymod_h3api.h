#pragma once
//
// ═══════════════════════════════════════════════════════════════
//  MyMod 的 H3API 集成头文件
// ───────────────────────────────────────────────────────────────
//  把 H3API 单头文件包进来，并配置好预处理宏。
//  所有需要操作 H3 内部对象的 .cpp 都应该包含本头文件，
//  而不是直接 include H3API.hpp。
//
//  好处：
//    1. 集中管理 H3API 配置宏
//    2. 减少其他文件的 include 噪音
//    3. 未来要加预编译头时只改一处
// ═══════════════════════════════════════════════════════════════

// VS 2017+ 在 /permissive- 模式下编 H3API 会报错，关掉
// （CMake 默认不开 /permissive-，这里只是保险）
#ifndef _ALLOW_RTCc_IN_STL
#define _ALLOW_RTCc_IN_STL
#endif

// 启用 H3API 各项扩展功能
#define _H3API_STD_CONVERSIONS_       // std::string ↔ H3String 互转
#define _H3API_UNPREFIXED_NAMES_      // 让 h3::Hero 也能写成 h3::H3Hero
#define _H3API_PATCHER_X86_           // 启用 H3API 内置的 patcher_x86 类定义

// 防止 H3API 重定义（CMake 已经定义了）
#ifdef _CRT_SECURE_NO_WARNINGS
#define MYMOD_HAD_CRT_SECURE
#undef _CRT_SECURE_NO_WARNINGS
#endif

#include "H3API.hpp"

#ifdef MYMOD_HAD_CRT_SECURE
#define _CRT_SECURE_NO_WARNINGS
#undef MYMOD_HAD_CRT_SECURE
#endif

// 把 h3 命名空间引进来，写代码方便（仅在 .cpp 里用，不要在 .h 里 using）
// 注意：本宏只在 .cpp 中用，避免污染头文件的命名空间
#define USE_H3 using namespace h3;

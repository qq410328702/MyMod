#pragma once
//
// 所有 hook 的安装入口集中声明
//

namespace mymod::hooks {

// 示例 1：在 DllMain 阶段弹消息框，证明注入成功
//   不需要 patcher_x86，只用 Win32 API
void Install_StartupMessage();

// 示例 2：IAT hook MessageBoxA
//   不需要 patcher_x86，演示标准的 IAT hook 技术
//   能给所有 H3 弹的消息框加前缀
void Install_MessageBoxIATHook();

// 示例 3：用 patcher_x86 hook 游戏内部函数
//   占位地址 + 模式扫描 fallback；只写日志，不改逻辑
void Install_MovementHook();

// 示例 4：生物名彩色化 - 阶段 1（dump 到 log）
//   遍历 H3CreatureInformation[]，按阶级染色生成 "{~Color}Lv.X 名字{~}"
//   不真挂 hook，先验证字段访问与色名映射
void Install_CreatureColor();

// 在游戏运行时再 dump 一次（被 hook_logic.cpp 在第一次 hook 触发时调用）
void DumpCreatureArrayDelayed();

// 卸载所有 hook（DLL_PROCESS_DETACH 时调用）
void UninstallAll();

}  // namespace mymod::hooks

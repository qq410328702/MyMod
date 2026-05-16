#pragma once
//
// 模式扫描（pattern scan / signature scan）
//
// 跨 EXE 版本兼容用。给定一段字节签名（含 ?? 通配符），
// 在指定模块的 .text 段里搜索匹配位置。
//
// 用法：
//   // 例：搜索 push ebp; mov ebp,esp; sub esp,??
//   const BYTE pat[] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x00 };
//   const char mask[] = "xxxxx?";
//   void* p = mymod::pattern::Find(GetModuleHandleA("h3hota.exe"), pat, mask);
//

#include <windows.h>
#include <cstddef>

namespace mymod::pattern {

// 在 module 的 .text 段中查找；mask 中 'x' 表示精确匹配，'?' 表示任意字节
// 返回匹配的首字节地址；找不到返回 nullptr
void* Find(HMODULE module, const BYTE* pat, const char* mask);

// IDA 风格签名："55 8B EC 83 EC ?? 56"
// 自动解析为字节数组 + mask
void* FindIDA(HMODULE module, const char* signature);

}  // namespace mymod::pattern

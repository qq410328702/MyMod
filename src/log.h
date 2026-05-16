#pragma once
//
// 简易日志器：写入游戏目录下的 MyMod.log
//
// 用法：
//   LOG_INFO("hero %d moved", id);
//   LOG_ERROR("hook failed at 0x%X", addr);
//

#include <cstdio>

namespace mymod::log_ {

enum Level : int {
    Off   = 0,
    Error = 1,
    Info  = 2,
    Debug = 3
};

void Init(const char* filename, int level);
void Shutdown();

void Write(Level lv, const char* fmt, ...);

}  // namespace mymod::log_

// 便捷宏
#define LOG_ERROR(...) ::mymod::log_::Write(::mymod::log_::Error, __VA_ARGS__)
#define LOG_INFO(...)  ::mymod::log_::Write(::mymod::log_::Info,  __VA_ARGS__)
#define LOG_DEBUG(...) ::mymod::log_::Write(::mymod::log_::Debug, __VA_ARGS__)

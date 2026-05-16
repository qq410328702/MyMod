#pragma once
//
// 从 MyMod.ini 读取配置
//

namespace mymod::config {

struct Config {
    // [Demo]
    bool ShowStartupMessage  = true;
    bool EnableMessageBoxHook = true;

    // [Movement]
    bool EnableMovementMod   = false;

    // [CreatureColor]
    bool EnableCreatureColor = true;     // 阶段 1：dump 到 log

    // [Logging]
    int  LogLevel = 2;     // 0..3
    char LogFile[260] = "MyMod.log";
};

// 加载配置；ini_path 是完整路径
void Load(const char* ini_path);

// 全局只读访问
const Config& Get();

}  // namespace mymod::config

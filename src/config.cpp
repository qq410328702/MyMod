#include "config.h"

#include <windows.h>
#include <cstring>

namespace mymod::config {

static Config g_cfg;

void Load(const char* ini_path)
{
    g_cfg.ShowStartupMessage =
        GetPrivateProfileIntA("Demo", "ShowStartupMessage", 1, ini_path) != 0;

    g_cfg.EnableMessageBoxHook =
        GetPrivateProfileIntA("Demo", "EnableMessageBoxHook", 1, ini_path) != 0;

    g_cfg.EnableMovementMod =
        GetPrivateProfileIntA("Movement", "EnableMovementMod", 0, ini_path) != 0;

    g_cfg.EnableCreatureColor =
        GetPrivateProfileIntA("CreatureColor", "EnableCreatureColor", 1, ini_path) != 0;

    g_cfg.LogLevel =
        GetPrivateProfileIntA("Logging", "LogLevel", 2, ini_path);

    GetPrivateProfileStringA("Logging", "LogFile", "MyMod.log",
                             g_cfg.LogFile, sizeof(g_cfg.LogFile), ini_path);
}

const Config& Get() { return g_cfg; }

}  // namespace mymod::config

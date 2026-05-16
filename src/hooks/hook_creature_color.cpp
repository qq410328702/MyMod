// ═══════════════════════════════════════════════════════════════
//  生物名彩色化（阶段 1：在 log 里 dump）
// ───────────────────────────────────────────────────────────────
//  目标：
//    遍历 H3 生物表 H3CreatureInformation[]（地址 0x6747B0），
//    按阶级（level 字段）选颜色，把"Lv.X 名字"格式的彩色字符串
//    写到 MyMod.log，验证字段访问、颜色映射方案都正确。
//
//  阶段 2 会把这个彩色字符串注入到游戏的 UI 里。
//
//  颜色方案（按阶级，借鉴 RPG 物品稀有度）：
//    Lv.0  灰      —— 战争机器 (catapult/ballista 等)
//    Lv.1  浅灰    —— 农民/小鬼等
//    Lv.2  白
//    Lv.3  绿      —— Common
//    Lv.4  蓝      —— Uncommon
//    Lv.5  紫      —— Rare
//    Lv.6  橙      —— Epic
//    Lv.7  金      —— Legendary
//    其他  红      —— 异常情况
// ═══════════════════════════════════════════════════════════════

#include "hooks.h"
#include "../config.h"
#include "../log.h"
#include "../mymod_h3api.h"

#include <windows.h>
#include <cstring>
#include <cstdio>

namespace mymod::hooks {

namespace {

// ═══════════════════════════════════════════════════════════
//  HotA 1.8 实际的 H3CreatureInformation 字段偏移
// ───────────────────────────────────────────────────────────
//  通过 IDA 反汇编 + 运行时 dump 验证（参见 docs/hota18-creature.md）：
//    +0x00  town          INT32     city/faction id
//    +0x04  level          INT32     1..7（战争机器=0）
//    +0x08  soundName      char*     例如 "pike" "halb"
//    +0x0C  defName        char*     例如 "cpkman.def"
//    +0x10  flags          UINT32    flyer/shooter/...
//    +0x14  ???            (4)       HotA 加的字段
//    +0x18  nameSingular   char*     '枪兵' '戟兵' (GBK)
//    +0x1C  abilityText    char*     '无视冲锋。' (GBK) — 注意是特技描述
//
//  注意：H3API 自带的字段名 (`nameSingular`/`namePlural`) 偏移对不上 HotA 1.8。
//        这里直接用裸偏移读，不通过 H3API 的成员访问。
// ═══════════════════════════════════════════════════════════
constexpr int OFF_LEVEL          = 0x04;
constexpr int OFF_SOUND_NAME     = 0x08;
constexpr int OFF_DEF_NAME       = 0x0C;
constexpr int OFF_NAME_SINGULAR  = 0x18;
constexpr int OFF_ABILITY_TEXT   = 0x1C;
constexpr int CREATURE_RECORD_SIZE = 0x74;  // 116 字节


// ─── 颜色名 → H3CN.toml 里 [TextColor] 已定义的色名 ──────
// 这些字符串会被 SoD_SP / HotA 的彩色文本系统解析成 RGB
const char* ColorByLevel(int level)
{
    switch (level) {
        case 0:  return "Gray";
        case 1:  return "Silver";
        case 2:  return "White";
        case 3:  return "Green";
        case 4:  return "Cyan";        // 用 Cyan，更显眼
        case 5:  return "Purple";
        case 6:  return "Orange";
        case 7:  return "Gold";
        default: return "Red";         // 异常或新阵营
    }
}

// ─── GBK 名字转 UTF-8（log 是 UTF-8 编码） ────────────────
static void GbkToUtf8(const char* gbk, char* utf8, size_t cap)
{
    if (!gbk || !utf8 || cap == 0) {
        if (utf8 && cap > 0) utf8[0] = '\0';
        return;
    }
    wchar_t wbuf[64] = {};
    int wlen = MultiByteToWideChar(936, 0, gbk, -1, wbuf,
                                   static_cast<int>(sizeof(wbuf)/sizeof(wchar_t)));
    if (wlen <= 0) {
        // 解码失败时原样拷贝（at least 显示乱码而不是空）
        strncpy(utf8, gbk, cap - 1);
        utf8[cap - 1] = '\0';
        return;
    }
    WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, utf8,
                        static_cast<int>(cap), nullptr, nullptr);
}


// ─── 城镇 ID → 阵营名（来源：H3API NH3Towns） ────────────
const char* TownName(int town)
{
    // 常见城镇编号；HotA 加了 9（港口/Cove）、10（工厂/Factory）、11（棱堡/Bulwark）
    switch (town) {
        case 0:  return "Castle";
        case 1:  return "Rampart";
        case 2:  return "Tower";
        case 3:  return "Inferno";
        case 4:  return "Necropolis";
        case 5:  return "Dungeon";
        case 6:  return "Stronghold";
        case 7:  return "Fortress";
        case 8:  return "Conflux";
        case 9:  return "Cove";       // HotA 港口
        case 10: return "Factory";    // HotA 工厂
        case 11: return "Bulwark";    // HotA 1.8 棱堡
        case -1: return "Neutral";    // 中立单位 (战争机器等)
        default: return "?";
    }
}

// ─── 在主模块内存中搜索字节序列 ──────────────────────────
// 用于在 EXE / DLL 内存中找已知字符串（比如 "长矛兵" 的 GBK 字节）
// 返回所有匹配地址，最多 maxResults 个
static int FindBytesInMemory(const BYTE* needle, size_t needleLen,
                              DWORD* results, int maxResults)
{
    HMODULE main = GetModuleHandleA(nullptr);
    if (!main || needleLen == 0) return 0;

    auto base = reinterpret_cast<BYTE*>(main);
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return 0;

    // 遍历所有 section
    int found = 0;
    auto sec = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++sec) {
        BYTE* secStart = base + sec->VirtualAddress;
        DWORD secSize = sec->Misc.VirtualSize;
        if (secSize < needleLen) continue;

        for (DWORD off = 0; off <= secSize - needleLen; ++off) {
            __try {
                if (memcmp(secStart + off, needle, needleLen) == 0) {
                    if (found < maxResults) {
                        results[found++] = (DWORD)(uintptr_t)(secStart + off);
                    }
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                break;
            }
        }
    }
    return found;
}

// 在主模块的 .rdata/.data 段里搜"指向 target 的 4 字节指针"
// 找谁引用了 needle 字符串
static int FindPointersTo(DWORD target, DWORD* results, int maxResults)
{
    HMODULE main = GetModuleHandleA(nullptr);
    if (!main) return 0;
    auto base = reinterpret_cast<BYTE*>(main);
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);

    int found = 0;
    auto sec = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++sec) {
        BYTE* secStart = base + sec->VirtualAddress;
        DWORD secSize = sec->Misc.VirtualSize;
        if (secSize < 4) continue;

        for (DWORD off = 0; off <= secSize - 4; off += 4) {
            __try {
                DWORD val = *(DWORD*)(secStart + off);
                if (val == target) {
                    if (found < maxResults) {
                        results[found++] = (DWORD)(uintptr_t)(secStart + off);
                    }
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                break;
            }
        }
    }
    return found;
}

}  // namespace


// ═══════════════════════════════════════════════════════════
//  对外的安装入口
//  其实"安装"只是遍历一次生物表打印日志 —— 不真挂 hook。
//  阶段 2 才会有真正的 hook。
// ═══════════════════════════════════════════════════════════
void Install_CreatureColor()
{
    if (!config::Get().EnableCreatureColor) {
        LOG_DEBUG("Install_CreatureColor: skipped (disabled in INI)");
        return;
    }

    LOG_INFO("─── Creature color dump (stage 1) ───");

    // P_Creatures 是 H3DataArrayPointer<H3CreatureInformation>，地址 0x6747B0
    // 用 H3CreatureInformation::Get() 拿到数组首指针更直接
    h3::H3CreatureInformation* info = h3::H3CreatureInformation::Get();
    if (!info) {
        LOG_ERROR("H3CreatureInformation::Get() == null. "
                  "Game data not loaded yet?");
        return;
    }
    LOG_INFO("Creature info array @ %p, sizeof(H3CreatureInformation)=%d (0x%X)",
             info, (int)sizeof(h3::H3CreatureInformation),
             (int)sizeof(h3::H3CreatureInformation));

    // ─── 关键诊断：搜索 "长矛兵" 字符串在主模块的位置 ───
    // GBK 编码的 "长矛兵": B3 A4 C3 AC B1 F8
    // 也试 "Pikeman" 找英文兜底
    LOG_INFO("");
    LOG_INFO("─── Searching '长矛兵' (GBK) in main module ───");
    {
        const BYTE pikemanGbk[] = { 0xB3, 0xA4, 0xC3, 0xAC, 0xB1, 0xF8, 0x00 };
        DWORD addrs[16];
        int n = FindBytesInMemory(pikemanGbk, 7, addrs, 16);
        LOG_INFO("Found %d match(es)", n);
        for (int i = 0; i < n; ++i) {
            const char* s = reinterpret_cast<const char*>(addrs[i]);
            char u8[64];
            GbkToUtf8(s, u8, sizeof(u8));
            LOG_INFO("  string '%s' at 0x%08X", u8, addrs[i]);

            // 找谁引用这个字符串
            DWORD refs[8];
            int rn = FindPointersTo(addrs[i], refs, 8);
            for (int j = 0; j < rn; ++j) {
                LOG_INFO("    referenced from 0x%08X", refs[j]);
            }
        }
    }

    LOG_INFO("");
    LOG_INFO("─── Searching 'Pikeman' (ASCII) in main module ───");
    {
        const BYTE pikemanAscii[] = { 'P','i','k','e','m','a','n', 0 };
        DWORD addrs[16];
        int n = FindBytesInMemory(pikemanAscii, 8, addrs, 16);
        LOG_INFO("Found %d match(es)", n);
        for (int i = 0; i < n; ++i) {
            LOG_INFO("  string 'Pikeman' at 0x%08X", addrs[i]);
            DWORD refs[8];
            int rn = FindPointersTo(addrs[i], refs, 8);
            for (int j = 0; j < rn; ++j) {
                LOG_INFO("    referenced from 0x%08X", refs[j]);
            }
        }
    }

    // 同时尝试解读前几个字段（不做合法性检查）
    h3::H3CreatureInformation& c0 = info[0];
    LOG_INFO("creature[0] interpreted: town=%d level=%d nameSing=%p namePlur=%p",
             c0.town, c0.level, (void*)c0.nameSingular, (void*)c0.namePlural);
    if (c0.nameSingular && !IsBadStringPtrA(c0.nameSingular, 32)) {
        char nameU8[96];
        GbkToUtf8(c0.nameSingular, nameU8, sizeof(nameU8));
        LOG_INFO("creature[0] nameSing bytes: %02X %02X %02X %02X %02X %02X (utf8: %s)",
                 (BYTE)c0.nameSingular[0], (BYTE)c0.nameSingular[1],
                 (BYTE)c0.nameSingular[2], (BYTE)c0.nameSingular[3],
                 (BYTE)c0.nameSingular[4], (BYTE)c0.nameSingular[5],
                 nameU8);
    }

    // 原版 H3 SoD 共 145 个生物（ID 0..144），HotA 把这个数扩了
    // 但我们不知道 HotA 1.8 的精确总数，先 dump 一段范围（0..200）
    // 用合法性检查避免读到无效数据
    constexpr int MAX_CREATURE = 200;
    int validCount = 0;
    int consecutiveBlanks = 0;

    for (int i = 0; i < MAX_CREATURE; ++i) {
        h3::H3CreatureInformation& c = info[i];

        // 合法性：name 指针得能读、level 在合理范围
        // 把整段读放到 SEH 里，最坏情况下安全失败
        const char* nameSing = nullptr;
        const char* namePlur = nullptr;
        int level = -999;
        int town = -999;
        int hp = 0, atk = 0, def = 0, spd = 0;

        __try {
            nameSing = c.nameSingular;
            namePlur = c.namePlural;
            level    = c.level;
            town     = c.town;
            hp       = c.hitPoints;
            atk      = c.attack;
            def      = c.defence;
            spd      = c.speed;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            LOG_DEBUG("creature[%d]: unreadable - stopping at this point", i);
            break;
        }

        // 名字必须能读到；creature[0] 等占位槽位 nameSing=null，跳过
        if (!nameSing || IsBadStringPtrA(nameSing, 64)) {
            ++consecutiveBlanks;
            // 前 5 次 blank 也记录下来，看分布
            if (consecutiveBlanks <= 5) {
                LOG_INFO("creature[%3d]: blank slot (town=%d lvl=%d nameSing=%p)",
                         i, town, level, (void*)nameSing);
            }
            if (consecutiveBlanks > 10) {
                LOG_INFO("creature[%d]: 10 consecutive blanks, stopping", i);
                break;
            }
            continue;
        }
        consecutiveBlanks = 0;

        // 级别 0~7（含战争机器的 0），高于 7 通常是 HotA 新阵营或异常
        if (level < 0 || level > 10) {
            LOG_DEBUG("creature[%d] '%s': level=%d out of range - skipping",
                      i, nameSing, level);
            continue;
        }

        char nameU8[96];
        GbkToUtf8(nameSing, nameU8, sizeof(nameU8));

        char colored[256];
        snprintf(colored, sizeof(colored),
                 "{~%s}Lv.%d %s{~}",
                 ColorByLevel(level), level, nameU8);

        LOG_INFO("creature[%3d] %-10s lv%d hp%-3d a%-2d d%-2d sp%-2d  %s",
                 i, TownName(town), level, hp, atk, def, spd, colored);

        ++validCount;
    }

    LOG_INFO("─── Total %d valid creatures dumped ───", validCount);
}


// ═══════════════════════════════════════════════════════════
//  延迟 dump（被 hook_logic.cpp 在游戏内首次 hook 触发时调用）
//  这时全局对象已就绪，0x6747B0 指针指向真正的数据
// ═══════════════════════════════════════════════════════════
void DumpCreatureArrayDelayed()
{
    LOG_INFO("─── Delayed creature dump (game running) ───");

    // 重新读取 0x6747B0 指针（HotA 在游戏运行时把它指向真实数据）
    DWORD* pArrayPtr = reinterpret_cast<DWORD*>(0x006747B0);
    DWORD arrayAddr = *pArrayPtr;
    LOG_INFO("  *0x006747B0 = 0x%08X (current array address)", arrayAddr);

    if (!arrayAddr) {
        LOG_ERROR("  array pointer is null at runtime too");
        return;
    }

    // 用裸偏移读字段（绕开 H3API 的字段名，因为它们对不上 HotA 1.8）
    BYTE* arrayBase = reinterpret_cast<BYTE*>(arrayAddr);

    int validCount = 0;
    int consecutiveBlanks = 0;

    for (int i = 0; i < 200; ++i) {
        BYTE* slot = arrayBase + i * CREATURE_RECORD_SIZE;

        int level = 0;
        const char* sound = nullptr;
        const char* nameSing = nullptr;
        const char* abilityText = nullptr;

        __try {
            level       = *reinterpret_cast<int*>(slot + OFF_LEVEL);
            sound       = *reinterpret_cast<const char**>(slot + OFF_SOUND_NAME);
            nameSing    = *reinterpret_cast<const char**>(slot + OFF_NAME_SINGULAR);
            abilityText = *reinterpret_cast<const char**>(slot + OFF_ABILITY_TEXT);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            LOG_DEBUG("creature[%d]: unreadable, stopping", i);
            break;
        }

        if (!nameSing || IsBadStringPtrA(nameSing, 64)) {
            ++consecutiveBlanks;
            if (consecutiveBlanks > 10) {
                LOG_INFO("creature[%d]: 10 consecutive blanks, stop", i);
                break;
            }
            continue;
        }
        consecutiveBlanks = 0;

        char nameU8[96] = {};
        char abilU8[256] = {};
        GbkToUtf8(nameSing, nameU8, sizeof(nameU8));
        if (abilityText && !IsBadStringPtrA(abilityText, 256)) {
            GbkToUtf8(abilityText, abilU8, sizeof(abilU8));
        }

        // 拼装彩色字符串："{~Color}Lv.X 名字{~}"
        char colored[128];
        snprintf(colored, sizeof(colored),
                 "{~%s}Lv.%d %s{~}",
                 ColorByLevel(level), level, nameU8);

        LOG_INFO("creature[%3d] sound='%-5s' lvl=%d  %s   ability=\"%s\"",
                 i, sound ? sound : "?",
                 level, colored, abilU8);

        ++validCount;
    }

    LOG_INFO("─── 共 %d 个有效生物 ───", validCount);
}

}  // namespace mymod::hooks

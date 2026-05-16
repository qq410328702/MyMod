#include "pattern.h"

#include <cstring>
#include <cstdlib>

namespace mymod::pattern {

namespace {

// 找到 module 的 .text 段范围
bool GetTextSection(HMODULE module, BYTE*& begin, size_t& size)
{
    auto base = reinterpret_cast<BYTE*>(module);
    auto dos  = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return false;

    auto sec = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++sec) {
        if (memcmp(sec->Name, ".text", 5) == 0) {
            begin = base + sec->VirtualAddress;
            size  = sec->Misc.VirtualSize;
            return true;
        }
    }
    return false;
}

}  // namespace

void* Find(HMODULE module, const BYTE* pat, const char* mask)
{
    BYTE*  begin = nullptr;
    size_t size  = 0;
    if (!GetTextSection(module, begin, size)) return nullptr;

    size_t patLen = strlen(mask);
    if (size < patLen) return nullptr;

    for (size_t i = 0; i <= size - patLen; ++i) {
        bool ok = true;
        for (size_t j = 0; j < patLen; ++j) {
            if (mask[j] == 'x' && begin[i + j] != pat[j]) { ok = false; break; }
        }
        if (ok) return begin + i;
    }
    return nullptr;
}

void* FindIDA(HMODULE module, const char* signature)
{
    // 解析 IDA 风格签名
    BYTE pat[256] = {};
    char mask[257] = {};
    size_t n = 0;

    const char* p = signature;
    while (*p && n < sizeof(pat)) {
        while (*p == ' ') ++p;
        if (!*p) break;

        if (p[0] == '?') {
            pat[n] = 0;
            mask[n] = '?';
            ++p;
            if (*p == '?') ++p;        // 兼容 "??"
        } else {
            pat[n] = static_cast<BYTE>(strtoul(p, const_cast<char**>(&p), 16));
            mask[n] = 'x';
        }
        ++n;
    }
    mask[n] = 0;
    return Find(module, pat, mask);
}

}  // namespace mymod::pattern

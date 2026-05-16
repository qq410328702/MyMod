# IDAPython 脚本：在 h3hota.exe 中找生物名相关数据
#
# 由 ida.exe -A -S<this> 命令行运行，无需 GUI 交互
# 输出写到 ida_output.txt
#
# IDA 9.x 兼容版

import idaapi
import ida_bytes
import ida_xref
import ida_segment
import ida_funcs
import ida_search
import idautils
import idc

OUT = r"J:\HOMM3 HotA1.80 汉化正式版1.2\MyMod\ida_output.txt"

# 等 IDA 自动分析跑完（IDB 已分析过会立刻返回）
idaapi.auto_wait()

f = open(OUT, "w", encoding="utf-8")

def log(msg):
    f.write(msg + "\n")

log("=== IDA 9.3 analysis of h3hota.exe ===")
log("")
log(f"image_base = 0x{idaapi.get_imagebase():08X}")
log(f"min_ea     = 0x{idaapi.inf_get_min_ea():08X}")
log(f"max_ea     = 0x{idaapi.inf_get_max_ea():08X}")
log("")

# 列出所有 segments 帮我们理解内存布局
log("=== Segments ===")
for s in range(ida_segment.get_segm_qty()):
    seg = ida_segment.getnseg(s)
    log(f"  {seg.start_ea:08X} - {seg.end_ea:08X}  {ida_segment.get_segm_name(seg)}")
log("")


def find_bytes_all(needle: bytes):
    """在整个程序里搜索字节序列，返回所有匹配地址（最多20个）"""
    results = []
    ea = idaapi.inf_get_min_ea()
    end = idaapi.inf_get_max_ea()
    while ea < end and len(results) < 20:
        # IDA 9: ida_bytes.find_bytes(pattern, start, ...) 返回 ea 或 BADADDR
        try:
            found = ida_bytes.find_bytes(needle, ea)
        except Exception:
            # 老 API
            found = idaapi.BADADDR
            try:
                found = ida_search.find_binary(
                    ea, end, " ".join(f"{b:02X}" for b in needle), 16, 0)
            except Exception:
                pass
        if found == idaapi.BADADDR or found >= end:
            break
        results.append(found)
        ea = found + 1
    return results


def show_string_at(addr):
    """显示某地址处的字符串内容（GBK + 字节）"""
    raw = ida_bytes.get_bytes(addr, 32) or b''
    # 取直到 \0
    null = raw.find(b'\x00')
    s = raw[:null] if null >= 0 else raw
    try:
        decoded = s.decode("gbk", errors="replace")
    except Exception:
        decoded = "(decode failed)"
    return decoded, s.hex(' ')


def report_xrefs(addr, max_count=20):
    """列出所有引用 addr 的位置"""
    xrefs = []
    for xr in idautils.XrefsTo(addr, 0):
        xrefs.append(xr)
        if len(xrefs) >= max_count:
            break
    log(f"    {len(xrefs)} 个 xref:")
    for xr in xrefs:
        seg = ida_segment.getseg(xr.frm)
        seg_name = ida_segment.get_segm_name(seg) if seg else "?"
        func = ida_funcs.get_func(xr.frm)
        func_name = ida_funcs.get_func_name(func.start_ea) if func else ""
        log(f"      from 0x{xr.frm:08X} [{seg_name}] {func_name}")


def find_and_report(needle: bytes, label: str):
    log(f"")
    log(f"─── 搜索 {label}  ({needle.hex(' ')}) ───")
    matches = find_bytes_all(needle)
    log(f"  找到 {len(matches)} 处")
    for addr in matches:
        seg = ida_segment.getseg(addr)
        seg_name = ida_segment.get_segm_name(seg) if seg else "?"
        decoded, hexbytes = show_string_at(addr)
        log(f"  @ 0x{addr:08X} [{seg_name}] '{decoded}'  bytes: {hexbytes}")
        report_xrefs(addr)


# 1. 中文兵种名（GBK 编码）
find_and_report("长矛兵".encode("gbk"), "长矛兵 (GBK)")
find_and_report("长矛兵们".encode("gbk"), "长矛兵们 (GBK)")
find_and_report("农民".encode("gbk"), "农民 (GBK)")
find_and_report("农民们".encode("gbk"), "农民们 (GBK)")
find_and_report("天使".encode("gbk"), "天使 (GBK)")
find_and_report("恶狼骑手".encode("gbk"), "恶狼骑手 (GBK)")

# 2. 英文兵种名（汉化版可能保留了 SoD 4.0 英文）
find_and_report(b"Pikeman\x00", "Pikeman (ASCII)")
find_and_report(b"Halberdier\x00", "Halberdier (ASCII)")
find_and_report(b"Peasant\x00", "Peasant (ASCII)")

# 3. sound name（已知存在）
find_and_report(b"pike\x00", "pike (sound)")
find_and_report(b"halb\x00", "halb (sound)")

# 4. 看 0x6703B8（HotA 1.8 的生物 sound/def 数组）周围的引用
log("")
log("─── 0x6703B8 (HotA creature sound/def array) 的引用者 ───")
report_xrefs(0x6703B8, 30)

# 5. 看 SoD 3.2 的 0x6747B0（H3API 标的位置）现在是什么
log("")
log("─── 0x6747B0 (SoD 3.2 H3CreatureInformation array) 当前内容 ───")
val = ida_bytes.get_dword(0x6747B0)
log(f"  *0x6747B0 = 0x{val:08X}")
report_xrefs(0x6747B0, 30)

# 6. 找所有指向 0x6703B8 的 dword 指针（"谁在静态记录这个数组地址"）
log("")
log("─── 全程序扫描: 谁的字节里 == 0x006703B8 ───")
target = 0x006703B8
target_bytes = target.to_bytes(4, 'little')
matches = find_bytes_all(target_bytes)
log(f"  找到 {len(matches)} 处")
for addr in matches[:30]:
    seg = ida_segment.getseg(addr)
    seg_name = ida_segment.get_segm_name(seg) if seg else "?"
    func = ida_funcs.get_func(addr)
    func_name = ida_funcs.get_func_name(func.start_ea) if func else ""
    log(f"  0x{addr:08X} [{seg_name}] {func_name}")

log("")
log("=== Done ===")
f.close()

# GUI 模式下不退出 IDA，方便查看 Output 窗口里的日志
# 命令行模式可以打开下一行
# idc.qexit(0)
import ida_kernwin
ida_kernwin.msg("\n\n*** ida_find_creature_data.py finished ***\n")
ida_kernwin.msg(f"*** Output written to: {OUT}\n")

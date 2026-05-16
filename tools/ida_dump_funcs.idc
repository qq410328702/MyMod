// 把几个关键函数的反汇编 dump 到文件，方便分析

#include <idc.idc>

static dump_func(out_file, addr, label)
{
    auto end_ea, ea, line, func_addr;
    fprintf(out_file, "\n══════════════════════════════════════════════════════\n");
    fprintf(out_file, "  %s @ 0x%08X\n", label, addr);
    fprintf(out_file, "══════════════════════════════════════════════════════\n");

    func_addr = get_func_attr(addr, FUNCATTR_START);
    if (func_addr == BADADDR) {
        fprintf(out_file, "  (not in any function)\n");
        return;
    }
    end_ea = get_func_attr(func_addr, FUNCATTR_END);

    fprintf(out_file, "  function name: %s\n", get_func_name(func_addr));
    fprintf(out_file, "  range: 0x%08X - 0x%08X (%d bytes)\n",
            func_addr, end_ea, end_ea - func_addr);
    fprintf(out_file, "\n");

    // dump 反汇编
    ea = func_addr;
    while (ea < end_ea && ea != BADADDR) {
        line = generate_disasm_line(ea, 0);
        if (line != "")
            fprintf(out_file, "  %08X  %s\n", ea, line);
        ea = next_head(ea, end_ea);
    }
}

static main()
{
    auto out_file;
    out_file = fopen("J:\\HOMM3 HotA1.80 汉化正式版1.2\\MyMod\\ida_funcs.txt", "w");
    if (out_file == 0) {
        msg("ERROR: cannot open output\n");
        return;
    }

    fprintf(out_file, "=== Disassembly of HotA functions accessing 0x6747B0 ===\n");

    // 几个引用 0x6747B0 的函数（从前面分析得到）
    // 重点关注前几个：sub_40AB40 和 sub_40B0B0
    dump_func(out_file, 0x40AB40, "sub_40AB40 (loads creature data?)");
    dump_func(out_file, 0x40B0B0, "sub_40B0B0");
    dump_func(out_file, 0x416700, "sub_416700");
    dump_func(out_file, 0x4170B0, "sub_4170B0");
    dump_func(out_file, 0x420C00, "sub_420C00");

    fclose(out_file);
    msg("\n*** ida_dump_funcs.idc finished ***\n");
    msg("*** Output: J:\\HOMM3 HotA1.80 汉化正式版1.2\\MyMod\\ida_funcs.txt\n");
}

// dump sub_40AB40 调用的子函数 + 看 0x6747B0 的初始值

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

    fprintf(out_file, "  range: 0x%08X - 0x%08X (%d bytes)\n",
            func_addr, end_ea, end_ea - func_addr);

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
    out_file = fopen("J:\\HOMM3 HotA1.80 汉化正式版1.2\\MyMod\\ida_subfuncs.txt", "w");
    if (out_file == 0) {
        msg("ERROR: cannot open output\n");
        return;
    }

    fprintf(out_file, "=== HotA creature data investigation ===\n\n");

    // 1. 看 0x6747B0 在静态数据里的值（程序启动时初始化）
    fprintf(out_file, "─── Static value at 0x6747B0 ───\n");
    auto v;
    v = get_dword(0x6747B0);
    fprintf(out_file, "  *0x6747B0 = 0x%08X\n", v);
    if (v != 0 && v != 0xFFFFFFFF) {
        // 顺着这个指针看前 116 字节
        fprintf(out_file, "  bytes at 0x%08X:\n", v);
        auto i, line, b;
        line = "  ";
        for (i = 0; i < 32; i++) {
            b = get_wide_byte(v + i);
            line = line + sprintf("%02X ", b);
        }
        fprintf(out_file, "%s\n", line);
    }
    fprintf(out_file, "\n");

    // 2. dump sub_40AB40 调用的几个子函数
    //    特别是 sub_44A750/sub_44A7D0/sub_44AAB0 看是否处理字符串
    dump_func(out_file, 0x44A750, "sub_44A750");
    dump_func(out_file, 0x44A7D0, "sub_44A7D0");
    dump_func(out_file, 0x44AAB0, "sub_44AAB0 (key: 0x40AC51 调用)");
    dump_func(out_file, 0x44A9B0, "sub_44A9B0");

    // 3. 看 sub_416700 是不是"加载兵种数据"
    dump_func(out_file, 0x416700, "sub_416700 (前面引用 0x6747B0 多次)");

    fclose(out_file);
    msg("\n*** ida_dump_subfuncs.idc finished ***\n");
}

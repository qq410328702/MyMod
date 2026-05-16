// IDC 脚本：在 h3hota.exe 中找生物名相关数据
//
// IDA Free 9.3 GUI 模式：
//   File → Script File... (Alt+F7) 选这个 .idc 文件
//   或 File → Script Command... (Shift+F2) 复制内容，下拉选 IDC，Run
//
// 输出写到 J:\HOMM3 HotA1.80 汉化正式版1.2\MyMod\ida_output.txt

#include <idc.idc>

static log_str(file, msg)
{
    fprintf(file, "%s\n", msg);
}

static find_bytes_all(needle_hex, label, out_file)
{
    // needle_hex 是 IDA 风格的字节模式，比如 "B3 A4 C3 AC B1 F8"
    auto results, ea, end_ea, found, i, count;
    auto raw, decoded, hexstr;
    auto seg_name, xref_count, xr;
    auto func_addr, func_name;

    fprintf(out_file, "\n─── 搜索 %s  (%s) ───\n", label, needle_hex);

    results = "";
    ea = get_inf_attr(INF_MIN_EA);
    end_ea = get_inf_attr(INF_MAX_EA);
    count = 0;

    while (ea < end_ea && count < 20)
    {
        found = find_binary(ea, SEARCH_DOWN | SEARCH_NEXT, needle_hex);
        if (found == BADADDR || found >= end_ea)
            break;

        // 取这个地址处的 32 字节，用 GBK/ASCII 显示
        seg_name = get_segm_name(found);
        fprintf(out_file, "  @ 0x%08X [%s]\n", found, seg_name);

        // 列出 xrefs
        xref_count = 0;
        xr = get_first_dref_to(found);
        while (xr != BADADDR && xref_count < 20)
        {
            seg_name = get_segm_name(xr);
            func_addr = get_func_attr(xr, FUNCATTR_START);
            if (func_addr != BADADDR)
                func_name = get_func_name(func_addr);
            else
                func_name = "";
            fprintf(out_file, "    DREF from 0x%08X [%s] %s\n", xr, seg_name, func_name);
            xr = get_next_dref_to(found, xr);
            xref_count = xref_count + 1;
        }
        // 同时列出 code xrefs
        xr = get_first_cref_to(found);
        while (xr != BADADDR && xref_count < 20)
        {
            seg_name = get_segm_name(xr);
            func_addr = get_func_attr(xr, FUNCATTR_START);
            if (func_addr != BADADDR)
                func_name = get_func_name(func_addr);
            else
                func_name = "";
            fprintf(out_file, "    CREF from 0x%08X [%s] %s\n", xr, seg_name, func_name);
            xr = get_next_cref_to(found, xr);
            xref_count = xref_count + 1;
        }
        if (xref_count == 0)
            fprintf(out_file, "    (no xref)\n");

        ea = found + 1;
        count = count + 1;
    }
    fprintf(out_file, "  共 %d 处\n", count);
}

static report_xrefs_to(addr, label, out_file)
{
    auto xr, seg_name, func_addr, func_name, count;
    fprintf(out_file, "\n─── 引用 %s (0x%08X) 的位置 ───\n", label, addr);
    count = 0;

    xr = get_first_dref_to(addr);
    while (xr != BADADDR && count < 30)
    {
        seg_name = get_segm_name(xr);
        func_addr = get_func_attr(xr, FUNCATTR_START);
        if (func_addr != BADADDR)
            func_name = get_func_name(func_addr);
        else
            func_name = "";
        fprintf(out_file, "  DREF from 0x%08X [%s] %s\n", xr, seg_name, func_name);
        xr = get_next_dref_to(addr, xr);
        count = count + 1;
    }
    xr = get_first_cref_to(addr);
    while (xr != BADADDR && count < 30)
    {
        seg_name = get_segm_name(xr);
        func_addr = get_func_attr(xr, FUNCATTR_START);
        if (func_addr != BADADDR)
            func_name = get_func_name(func_addr);
        else
            func_name = "";
        fprintf(out_file, "  CREF from 0x%08X [%s] %s\n", xr, seg_name, func_name);
        xr = get_next_cref_to(addr, xr);
        count = count + 1;
    }
    fprintf(out_file, "  共 %d 个引用\n", count);
}

static main()
{
    auto out_file;
    auto seg_idx, seg_start, seg_end, seg_name;

    out_file = fopen("J:\\HOMM3 HotA1.80 汉化正式版1.2\\MyMod\\ida_output.txt", "w");
    if (out_file == 0)
    {
        msg("ERROR: cannot open output file\n");
        return;
    }

    fprintf(out_file, "=== IDA 9.3 IDC analysis of h3hota.exe ===\n\n");
    fprintf(out_file, "image_base = 0x%08X\n", get_imagebase());
    fprintf(out_file, "min_ea     = 0x%08X\n", get_inf_attr(INF_MIN_EA));
    fprintf(out_file, "max_ea     = 0x%08X\n\n", get_inf_attr(INF_MAX_EA));

    // 列出所有段
    fprintf(out_file, "=== Segments ===\n");
    seg_start = get_first_seg();
    while (seg_start != BADADDR)
    {
        seg_end = get_segm_end(seg_start);
        seg_name = get_segm_name(seg_start);
        fprintf(out_file, "  %08X - %08X  %s\n", seg_start, seg_end, seg_name);
        seg_start = get_next_seg(seg_start);
    }

    // 1. 中文兵种名（GBK 编码）
    find_bytes_all("B3 A4 C3 AC B1 F8", "长矛兵 (GBK)", out_file);
    find_bytes_all("B3 A4 C3 AC B1 F8 C3 C7", "长矛兵们 (GBK)", out_file);
    find_bytes_all("C5 A9 C3 F1", "农民 (GBK)", out_file);
    find_bytes_all("CC EC CA B9", "天使 (GBK)", out_file);
    find_bytes_all("B6 F1 C0 C7 C6 EF CA D6", "恶狼骑手 (GBK)", out_file);

    // 2. 英文兵种名
    find_bytes_all("50 69 6B 65 6D 61 6E 00", "Pikeman (ASCII)", out_file);
    find_bytes_all("48 61 6C 62 65 72 64 69 65 72 00", "Halberdier (ASCII)", out_file);
    find_bytes_all("50 65 61 73 61 6E 74 00", "Peasant (ASCII)", out_file);

    // 3. sound name 已知存在
    find_bytes_all("70 69 6B 65 00", "pike (sound)", out_file);
    find_bytes_all("68 61 6C 62 00", "halb (sound)", out_file);

    // 4. 找谁引用了 0x6703B8（HotA 1.8 sound/def 数组首地址）
    report_xrefs_to(0x6703B8, "0x6703B8 HotA creature array", out_file);

    // 5. 看 SoD 3.2 H3API 标的位置 0x6747B0 现在是什么
    report_xrefs_to(0x6747B0, "0x6747B0 SoD 3.2 array (H3API expected)", out_file);

    // 6. 找谁的 4 字节常量 == 0x006703B8（"代码里把这个地址当数组指针使用"）
    fprintf(out_file, "\n─── 全程序扫描: 谁的字节里 == 0x006703B8 ───\n");
    find_bytes_all("B8 03 67 00", "ptr 0x006703B8 (little endian)", out_file);

    fprintf(out_file, "\n=== Done ===\n");
    fclose(out_file);

    msg("\n*** ida_find_creature_data.idc finished ***\n");
    msg("*** Output: J:\\HOMM3 HotA1.80 汉化正式版1.2\\MyMod\\ida_output.txt\n");
}

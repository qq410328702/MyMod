# MyMod —— 一个最小可用的 H3/HotA 插件示例

> 这是一个完整能编译、能跑起来的 H3 / HotA 插件起手模板。
> 包含 3 个由浅到深的 hook 示例，从"证明注入成功"到"修改游戏逻辑"。

## 目录

```
MyMod/
├── CMakeLists.txt            # CMake 构建脚本
├── build.bat                 # 一键构建（Windows）
├── README.md                 # 本文档
├── MyMod.ini                 # 用户配置（运行时读取）
├── src/
│   ├── main.cpp              # DllMain 与初始化
│   ├── log.h / log.cpp       # 简单日志器
│   ├── config.h / config.cpp # INI 配置解析
│   ├── pattern.h / pattern.cpp # 模式扫描（pattern scan）
│   └── hooks/
│       ├── hooks.h
│       ├── hook_msgbox.cpp   # 示例 1：DllMain 中弹消息框，证明注入成功
│       ├── hook_iat.cpp      # 示例 2：IAT hook MessageBoxA，演示函数替换
│       └── hook_logic.cpp    # 示例 3：用 patcher_x86 hook 游戏内部函数
└── third_party/
    └── patcher_x86.hpp       # baratorch 公开的补丁框架头文件
```

## 1. 准备工作

### 必装

- **Visual Studio 2019 或 2022 Community**，安装时勾选：
  - "Desktop development with C++"
  - "C++ CMake tools for Windows"
  - "MSVC v143 - VS 2022 C++ x86/x64 build tools"
- **CMake 3.20+**（VS 自带的也行）
- **Heroes 3 + HotA 已安装**（用本目录上层那一份）
- **HD Mod 已启用**（HotA 自带）—— 我们用它的 Packs 机制加载 DLL

### 推荐

- IDA Free / x64dbg / Cheat Engine（逆向时用，本示例不强求）

## 2. 构建

最简：

```cmd
build.bat
```

它会调用 CMake，输出 `build\Release\MyMod.dll`。

手动：

```cmd
cmake -A Win32 -B build .
cmake --build build --config Release
```

> ⚠️ **必须是 Win32（32 位）**，因为 H3 是 32 位程序。

## 3. 安装到游戏

把生成的 DLL 复制到游戏的 HD Mod Packs 目录：

```cmd
xcopy /Y build\Release\MyMod.dll  "..\_HD3_Data\Packs\MyMod\"
copy /Y MyMod.ini                  "..\_HD3_Data\Packs\MyMod\"
```

或者手动：把 `MyMod.dll` 和 `MyMod.ini` 一起放到：

```
J:\HOMM3 HotA1.80 汉化正式版1.2\_HD3_Data\Packs\MyMod\
```

启动游戏（`h3hota HD（深渊号角HD版）.exe` 或 `HD_Launcher.exe`）。

## 4. 运行验证

启动游戏后：

1. **示例 1**：会弹出一个消息框 "[MyMod] DLL loaded!"，证明 DLL 被注入了。
2. **示例 2**：所有 H3 弹的系统消息框（"Are you sure?"等）会在标题栏前缀加上 `[MyMod hook]`。
3. **示例 3**：如果在 `MyMod.ini` 里启用了 `EnableMovementMod=1`，每天英雄获得移动力时会被记录到 `MyMod.log`。

游戏目录会出现 `MyMod.log`，里面有完整调试输出。

## 5. 关掉示例（重要）

示例 1 的弹框每次启动都弹很烦。在 `MyMod.ini` 里设：

```ini
[Demo]
ShowStartupMessage = 0
```

## 6. 下一步：改成你自己的功能

1. 在 `src/hooks/` 下新建一个文件，照 `hook_logic.cpp` 的格式写。
2. 在 `hooks.h` 里声明 `Install_YourHook()` 函数。
3. 在 `main.cpp` 的 `InstallAllHooks()` 里调一下。
4. 重新 `build.bat`。

最常修改的几个地方：

- `src/hooks/hook_logic.cpp` 中的 `MOVEMENT_FUNC_ADDRESS` 常量是占位符，你需要：
  - 用 IDA 打开 `h3hota.exe`，找到处理英雄每日移动力的函数。
  - 把地址替换进去。
  - **或者**用 `pattern.h` 的扫描器动态查找。

更详细的逆向流程见上一级目录的 `HotA二次开发实战指南.md`。

## 7. 故障排查

| 现象 | 原因 | 解决 |
|---|---|---|
| 编译报错 "patcher_x86.hpp not found" | include 路径没设 | 用 CMake 重新生成 |
| DLL 编译出来是 64 位 | 没指定 Win32 平台 | `cmake -A Win32` |
| 启动游戏没反应 | 没放对目录 / HD Mod 没启用 | 确认在 `_HD3_Data\Packs\MyMod\` 下 |
| 弹框显示 "patcher not found" | 游戏没装 HD Mod 或 patcher_x86.dll | 确认游戏目录有 `patcher_x86.dll` |
| 游戏崩溃 | hook 写错了 | 删掉 DLL，看 `MyMod.log` 最后一行 |

## 8. 许可

- 本模板代码采用 MIT License，你可以自由使用。
- `third_party/patcher_x86.hpp` 来自 [baratorch 公开发布](https://gist.github.com/Ginden/0681928f412e709491f1b55ea396bbb1)，原作者保留权利。
- 不附带 H3 / HotA 的任何资源、EXE 或受版权保护的素材。

## 9. 参考

- 上层目录 `HotA二次开发实战指南.md`（详细逆向方法）
- 上层目录 `HotA逆向开发分析.md`（HotA 团队工作方式分析）
- [patcher_x86 API](https://gist.github.com/Ginden/0681928f412e709491f1b55ea396bbb1)
- [H3API 头文件库](https://github.com/RoseKavalier/H3API)
- [SoD_SP 完整插件源码](https://github.com/RoseKavalier/H3Plugins)

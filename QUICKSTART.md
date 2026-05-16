# MyMod 快速开始

> 5 分钟从下载到看到效果。详细文档见 `README.md`。

## 1. 装工具

下载并安装 [Visual Studio 2022 Community](https://visualstudio.microsoft.com/free-developer-offers/)。
安装时勾选：

- ✅ Desktop development with C++
- ✅ C++ CMake tools for Windows
- ✅ MSVC v143 - VS 2022 C++ x86/x64 build tools

## 2. 编译

打开 **"x64 Native Tools Command Prompt for VS 2022"**（或 x86 版本，CMake 会处理），
进入本目录：

```cmd
cd /d "J:\HOMM3 HotA1.80 汉化正式版1.2\MyMod"
build.bat
```

成功后会看到：

```
Output: build\Release\MyMod.dll
```

## 3. 安装

把编译产物和 INI 复制到 HotA 的 HD Mod 插件目录：

```cmd
mkdir "..\_HD3_Data\Packs\MyMod"
copy /Y build\Release\MyMod.dll "..\_HD3_Data\Packs\MyMod\"
copy /Y MyMod.ini               "..\_HD3_Data\Packs\MyMod\"
```

> 想让编译完自动复制？设置环境变量再编译：
> ```cmd
> set H3_GAME_DIR=J:\HOMM3 HotA1.80 汉化正式版1.2
> build.bat
> ```
> CMake 会自动复制到 `%H3_GAME_DIR%\_HD3_Data\Packs\MyMod\`。

## 4. 运行

启动 `h3hota HD（深渊号角HD版）.exe`。

应该看到：

1. 弹出蓝色信息框 "[MyMod] DLL loaded!"
2. 主菜单或对话框打开时，标题前缀变成 "[MyMod hook] ..."
3. 游戏目录里出现 `MyMod.log`，里面有完整加载过程

## 5. 关闭演示弹框

`MyMod.ini` 里改：

```ini
[Demo]
ShowStartupMessage = 0
```

重启游戏。

## 6. 改成你自己的功能

修改 `src/hooks/hook_logic.cpp`：

- 把 `PLACEHOLDER_FUNC_ADDR` / `PLACEHOLDER_MOVEMENT_FUNC` 换成你用 IDA 找到的真实地址
- 在 hook 函数里写你的逻辑
- `build.bat` 重新编译

详细方法看上一级目录 `HotA二次开发实战指南.md`。

## 排查

- 编译失败 "patcher_x86.hpp not found" → CMake 没正确生成，删 `build/` 重来
- 启动游戏没反应 → 看 DLL 是不是放对了 `_HD3_Data\Packs\MyMod\MyMod.dll`
- 弹框说 "patcher not found" → 游戏目录得有 `patcher_x86.dll`（HotA 自带）
- 游戏崩溃 → 删掉 DLL 看 `MyMod.log` 最后几行

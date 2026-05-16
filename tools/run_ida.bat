@echo off
REM ──────────────────────────────────────────────────────────────
REM 用 IDA Free 9.3 跑 h3hota.exe，自动执行 ida_find_creature_data.py
REM 第一次运行约需要 5-15 分钟（IDA 分析全部代码）
REM 之后会生成 .i64 工程文件，下次秒开
REM ──────────────────────────────────────────────────────────────

setlocal

set "IDA=C:\Program Files\IDA Free 9.3\ida.exe"
set "EXE=J:\HOMM3 HotA1.80 汉化正式版1.2\h3hota.exe.i64"
set "SCRIPT=%~dp0ida_find_creature_data.py"
set "OUT=J:\HOMM3 HotA1.80 汉化正式版1.2\MyMod\ida_output.txt"

if not exist "%IDA%" (
    echo [ERROR] ida.exe not found at: %IDA%
    exit /b 1
)
if not exist "%EXE%" (
    echo [ERROR] h3hota.exe not found at: %EXE%
    exit /b 1
)

echo === 删除旧输出 ===
del "%OUT%" 2>nul

echo === 启动 IDA 自动分析 + 脚本 ===
echo  IDA:    %IDA%
echo  Target: %EXE%
echo  Script: %SCRIPT%
echo.
echo 第一次运行约需要 5-15 分钟，请耐心等待...
echo.

REM -A    : 全自动模式（不弹任何对话框）
REM -S    : 加载完成后运行脚本
REM 注意：IDA 会在 EXE 同目录创建 .i64 工程文件
"%IDA%" -A -S"%SCRIPT%" "%EXE%"

if errorlevel 1 (
    echo [ERROR] IDA 退出码非 0
    exit /b 1
)

echo.
echo === 输出文件 ===
if exist "%OUT%" (
    type "%OUT%"
) else (
    echo [WARN] 输出文件未生成，看 IDA 日志
)

endlocal

@echo off
setlocal enableextensions enabledelayedexpansion

REM ============================================================
REM  MyMod 一键构建脚本（自动定位 VS 工具链）
REM
REM  用法：
REM    build.bat              ← Release 构建
REM    build.bat Debug        ← Debug 构建
REM    build.bat clean        ← 清理 build/
REM ============================================================

REM ── 处理参数 ──────────────────────────────────────────────
set "CFG=Release"
if /i "%~1"=="debug"   set "CFG=Debug"
if /i "%~1"=="release" set "CFG=Release"
if /i "%~1"=="clean" goto :clean

REM ── 1. 定位 vswhere ──────────────────────────────────────
set "PFX86=%ProgramFiles(x86)%"
set "VSWHERE=!PFX86!\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "!VSWHERE!" (
    echo [ERROR] vswhere.exe not found.
    echo Expected at: !VSWHERE!
    echo Install Visual Studio 2019/2022 first.
    exit /b 1
)

REM ── 2. 找 VS 安装目录 ────────────────────────────────────
set "VSPATH="
for /f "usebackq delims=" %%i in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"

if not defined VSPATH (
    echo [ERROR] No VS installation with C++ tools found.
    echo Open VS Installer and add "Desktop development with C++".
    exit /b 1
)
echo Found VS: !VSPATH!

REM ── 3. 加载环境（x86 工具链）────────────────────────────
set "VCVARS=!VSPATH!\VC\Auxiliary\Build\vcvarsall.bat"
if not exist "!VCVARS!" (
    echo [ERROR] vcvarsall.bat not found at: !VCVARS!
    exit /b 1
)
call "!VCVARS!" x86 >nul
if errorlevel 1 (
    echo [ERROR] vcvarsall.bat failed.
    exit /b 1
)

REM ── 4. 找 cmake ──────────────────────────────────────────
set "CMAKE=!VSPATH!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not exist "!CMAKE!" (
    where cmake >nul 2>nul
    if errorlevel 1 (
        echo [ERROR] cmake not found.
        echo Add "C++ CMake tools for Windows" in VS Installer.
        exit /b 1
    )
    set "CMAKE=cmake"
)
echo Using CMake: !CMAKE!

REM ── 5. 配置 ─────────────────────────────────────────────
if not exist "build\CMakeCache.txt" (
    echo === Configuring ===
    "!CMAKE!" -A Win32 -B build .
    if errorlevel 1 exit /b 1
)

REM ── 6. 构建 ─────────────────────────────────────────────
echo === Building [!CFG!] ===
"!CMAKE!" --build build --config !CFG! --parallel
if errorlevel 1 exit /b 1

echo.
echo === Done ===
echo Output: build\!CFG!\MyMod.dll
echo.
echo To install:
echo   install.bat
echo Or manually:
echo   copy /Y build\!CFG!\MyMod.dll "..\_HD3_Data\Packs\MyMod\"
echo   copy /Y MyMod.ini             "..\_HD3_Data\Packs\MyMod\"

exit /b 0

:clean
echo Cleaning build directory...
if exist build rmdir /s /q build
echo Done.
exit /b 0

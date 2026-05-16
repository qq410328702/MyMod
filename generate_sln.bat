@echo off
setlocal enableextensions enabledelayedexpansion

REM ============================================================
REM  用 CMake 生成 Visual Studio 2022 解决方案 (.sln)
REM
REM  生成位置: build\MyMod.sln
REM  双击 .sln 即可在 VS 中打开
REM ============================================================

set "PFX86=%ProgramFiles(x86)%"
set "VSWHERE=!PFX86!\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "!VSWHERE!" (
    echo [ERROR] vswhere.exe not found.
    exit /b 1
)

set "VSPATH="
for /f "usebackq delims=" %%i in (`"!VSWHERE!" -latest -products * -property installationPath`) do set "VSPATH=%%i"

set "CMAKE=!VSPATH!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not exist "!CMAKE!" (
    where cmake >nul 2>nul || (echo [ERROR] cmake not found & exit /b 1)
    set "CMAKE=cmake"
)

echo Using CMake: !CMAKE!
echo.
echo === Generating Visual Studio 2022 solution (Win32) ===
"!CMAKE!" -G "Visual Studio 17 2022" -A Win32 -B build .
if errorlevel 1 exit /b 1

echo.
echo ============================================================
echo  Done! Solution generated at:
echo    build\MyMod.sln
echo.
echo  To open in Visual Studio:
echo    1. Double-click build\MyMod.sln
echo    2. Or run:  start build\MyMod.sln
echo ============================================================
endlocal

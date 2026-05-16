@echo off
setlocal

REM ===========================================
REM  把 MyMod 安装到游戏根目录
REM
REM  使用 launcher 注入方式（不修改任何游戏文件）
REM ===========================================

set "DLL=build\Release\MyMod.dll"
set "EXE=build\Release\MyMod_launcher.exe"
set "INI=MyMod.ini"
set "DEST=.."

if not exist "%DLL%" (
    echo [ERROR] %DLL% not found. Run build.bat first.
    exit /b 1
)
if not exist "%EXE%" (
    echo [ERROR] %EXE% not found. Run build.bat first.
    exit /b 1
)
if not exist "%DEST%\h3hota.exe" (
    echo [ERROR] Game directory not found at: %DEST%
    echo This script expects to run from inside MyMod/ folder
    echo with the game in the parent directory.
    exit /b 1
)

copy /Y "%DLL%" "%DEST%\MyMod.dll"            || exit /b 1
copy /Y "%EXE%" "%DEST%\MyMod_launcher.exe"   || exit /b 1
copy /Y "%INI%" "%DEST%\MyMod.ini"            || exit /b 1

echo.
echo === Installed ===
echo   %DEST%\MyMod.dll
echo   %DEST%\MyMod_launcher.exe
echo   %DEST%\MyMod.ini
echo.
echo To start the game with MyMod:
echo   Double-click MyMod_launcher.exe in the game folder.
echo.
echo (Or from cmd: cd .. ^&^& MyMod_launcher.exe)

endlocal

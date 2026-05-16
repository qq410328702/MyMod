@echo off
setlocal

set "IDA=C:\Program Files\IDA Free 9.3\ida.exe"
set "IDB=J:\HOMM3 HotA1.80 汉化正式版1.2\h3hota.exe.i64"
set "SCRIPT=J:\HOMM3 HotA1.80 汉化正式版1.2\MyMod\tools\ida_find_creature_data.py"
set "IDALOG=J:\HOMM3 HotA1.80 汉化正式版1.2\MyMod\ida_log.txt"

echo === Cleanup ===
del "J:\HOMM3 HotA1.80 汉化正式版1.2\MyMod\ida_output.txt" 2>nul
del "%IDALOG%" 2>nul

echo === Running IDA with -L log ===
echo  IDA:    "%IDA%"
echo  IDB:    "%IDB%"
echo  Script: "%SCRIPT%"
echo  Log:    "%IDALOG%"
echo.

"%IDA%" -A -L"%IDALOG%" -S"%SCRIPT%" "%IDB%"
echo Exit code: %errorlevel%
echo.

echo === IDA log ===
if exist "%IDALOG%" type "%IDALOG%"
echo.

echo === ida_output.txt ===
if exist "J:\HOMM3 HotA1.80 汉化正式版1.2\MyMod\ida_output.txt" (
    type "J:\HOMM3 HotA1.80 汉化正式版1.2\MyMod\ida_output.txt"
) else (
    echo NOT GENERATED
)
endlocal

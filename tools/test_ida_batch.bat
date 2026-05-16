@echo off
REM 用 IDA 批处理模式（-B）跑 h3hota.exe，纯分析，不跑脚本
REM 看 IDA 是否能正常工作
echo === IDA batch test ===
"C:\Program Files\IDA Free 9.3\ida.exe" -B "J:\HOMM3 HotA1.80 汉化正式版1.2\h3hota.exe"
echo.
echo Exit code: %errorlevel%
echo.
echo === Generated files ===
dir "J:\HOMM3 HotA1.80 汉化正式版1.2\h3hota.*" 2>nul

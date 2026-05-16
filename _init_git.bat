@echo off
chcp 65001 >nul
cd /d "j:\HOMM3 HotA1.80 汉化正式版1.2\MyMod"

echo ============================================================
echo  Step 1: Remove embedded H3API .git directory
echo ============================================================
if exist "third_party\H3API\.git" (
    rmdir /s /q "third_party\H3API\.git"
    echo Removed third_party\H3API\.git
) else (
    echo (already removed)
)

echo.
echo ============================================================
echo  Step 2: Init MyMod git repo
echo ============================================================
git init -b main
if errorlevel 1 exit /b 1

echo.
echo ============================================================
echo  Step 3: Re-add H3API as a proper submodule
echo ============================================================
REM 先把整个 H3API 目录从工作区移走（避免 add submodule 时冲突）
if exist "third_party\H3API" (
    move /Y "third_party\H3API" "third_party\_H3API_temp" >nul
)
git submodule add --depth 1 https://github.com/RoseKavalier/H3API.git third_party/H3API
if errorlevel 1 (
    echo Submodule add failed, restoring local copy
    if exist "third_party\_H3API_temp" move /Y "third_party\_H3API_temp" "third_party\H3API"
    exit /b 1
)
REM 删掉 _H3API_temp（submodule add 已经重新拉取了一份干净的）
if exist "third_party\_H3API_temp" rmdir /s /q "third_party\_H3API_temp"

echo.
echo ============================================================
echo  Step 4: Stage all files
echo ============================================================
git add .
if errorlevel 1 exit /b 1

echo.
echo === Staged files preview (first 60) ===
git status --short | more +0 -60

echo.
echo ============================================================
echo  Step 5: Initial commit
echo ============================================================
git commit -m "Initial commit: MyMod skeleton with H3API integration" -m "- DllMain + log + config + pattern scan + 3 hooks (msgbox / IAT / patcher_x86)" -m "- MyMod_launcher.exe injects MyMod.dll into h3hota.exe" -m "- CMake build, VS 2022 .sln generation, post-build auto-deploy" -m "- H3API integrated as submodule under third_party/H3API"
if errorlevel 1 exit /b 1

echo.
echo ============================================================
echo  Done!
echo ============================================================
git log --oneline -5
echo.
echo Repo:    j:\HOMM3 HotA1.80 汉化正式版1.2\MyMod
echo Branch:  main
echo Submodule: third_party/H3API @ RoseKavalier/H3API

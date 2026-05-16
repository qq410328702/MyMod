cmake_minimum_required(VERSION 3.20)
project(MyMod CXX)

# ─── 强制 32 位 ───────────────────────────────────────────
# H3 是 32 位程序，DLL 必须也是 32 位才能注入进去
if(NOT CMAKE_GENERATOR_PLATFORM STREQUAL "Win32" AND NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR
        "MyMod must be built as 32-bit (Win32). "
        "Run: cmake -G \"Visual Studio 17 2022\" -A Win32 -B build .")
endif()

# ─── 编译选项 ─────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    # /MT 静态链接运行时，分发时不依赖 vc_redist
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/W3 /utf-8 /Zc:__cplusplus)
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()

# ─── 推断游戏目录 ─────────────────────────────────────────
# 优先级：环境变量 H3_GAME_DIR > 工程根目录的上一级（约定）
if(DEFINED ENV{H3_GAME_DIR})
    set(GAME_DIR "$ENV{H3_GAME_DIR}")
else()
    get_filename_component(GAME_DIR "${CMAKE_SOURCE_DIR}/.." ABSOLUTE)
endif()

# 验证游戏目录存在并包含 H3 EXE
if(EXISTS "${GAME_DIR}/h3hota.exe")
    message(STATUS "Game directory: ${GAME_DIR}")
    set(GAME_DIR_VALID TRUE)
else()
    message(WARNING "Game directory not auto-detected at: ${GAME_DIR}")
    message(WARNING "  Set environment variable H3_GAME_DIR to override.")
    set(GAME_DIR_VALID FALSE)
endif()

# ─── 源文件 ───────────────────────────────────────────────
set(MYMOD_SOURCES
    src/main.cpp
    src/log.cpp
    src/config.cpp
    src/pattern.cpp
    src/hooks/hook_msgbox.cpp
    src/hooks/hook_iat.cpp
    src/hooks/hook_logic.cpp
    src/hooks/hook_creature_color.cpp
)

# ─── DLL 目标 ─────────────────────────────────────────────
add_library(MyMod SHARED ${MYMOD_SOURCES})

target_include_directories(MyMod PRIVATE
    src
    third_party
    third_party/H3API/single_header  # H3API：H3 类型/函数定义的 SDK
)

# 不要给 DLL 加 lib 前缀
set_target_properties(MyMod PROPERTIES
    PREFIX ""
    OUTPUT_NAME "MyMod"
)

# ─── 注入器 launcher ──────────────────────────────────────
add_executable(MyMod_launcher WIN32 src/launcher.cpp)
set_target_properties(MyMod_launcher PROPERTIES
    OUTPUT_NAME "MyMod_launcher"
)
target_link_libraries(MyMod_launcher PRIVATE user32)

# ─── 自动部署到游戏目录（post-build） ─────────────────────
if(GAME_DIR_VALID)
    add_custom_command(TARGET MyMod POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:MyMod>" "${GAME_DIR}/MyMod.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/MyMod.ini" "${GAME_DIR}/MyMod.ini"
        COMMENT "Deploying MyMod.dll → ${GAME_DIR}"
    )

    add_custom_command(TARGET MyMod_launcher POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:MyMod_launcher>" "${GAME_DIR}/MyMod_launcher.exe"
        COMMENT "Deploying MyMod_launcher.exe → ${GAME_DIR}"
    )
endif()

# ─── F5 调试器配置（生成 .vcxproj.user） ──────────────────
# 让按 F5 时：
#   1. 启动游戏目录里部署好的 MyMod_launcher.exe（不是 build/Release/ 那个）
#   2. 工作目录设到游戏根，这样 launcher 能找到 MyMod.dll 和 h3hota.exe
#   3. 调试器附加在 MyMod_launcher 上（短暂）—— 它会启动游戏并退出
#      → 游戏进程独立运行，但 DLL 已被注入
if(GAME_DIR_VALID AND MSVC)
    set_target_properties(MyMod_launcher PROPERTIES
        VS_DEBUGGER_COMMAND           "${GAME_DIR}/MyMod_launcher.exe"
        VS_DEBUGGER_WORKING_DIRECTORY "${GAME_DIR}"
        VS_DEBUGGER_ENVIRONMENT       "PATH=%PATH%;${GAME_DIR}"
    )

    # MyMod 这个 DLL 项目不能直接 F5（DLL 不能独立启动）。
    # 但如果用户误把它设为启动项，VS 会用 dll 项目的调试器配置 —— 这里也指向 launcher，
    # 这样 F5 不会直接报错，仍能顺利启动游戏。
    set_target_properties(MyMod PROPERTIES
        VS_DEBUGGER_COMMAND           "${GAME_DIR}/MyMod_launcher.exe"
        VS_DEBUGGER_WORKING_DIRECTORY "${GAME_DIR}"
    )
endif()

# ─── 把 MyMod_launcher 设为 .sln 的默认启动项 ─────────────
# 解决"打开 sln 后默认启动项是 ALL_BUILD（不能运行）"的尴尬
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY VS_STARTUP_PROJECT MyMod_launcher)

cmake_minimum_required(VERSION 3.16)
project(QuickLaunch CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DWIN32_LEAN_AND_MEAN)
add_definitions(-DNOMINMAX)

# 关闭 MSVC 安全警告（sprintf_s）
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

add_executable(QuickLaunch 
    WIN32
    src/main.cpp
    src/Config.cpp
    src/WindowManager.cpp
    src/ButtonRenderer.cpp
)

# GDI+
target_link_libraries(QuickLaunch gdiplus)

# 复制资源文件到输出目录
add_custom_command(TARGET QuickLaunch POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/Config.json
        $<TARGET_FILE_DIR:QuickLaunch>/
)

add_custom_command(TARGET QuickLaunch POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/close.png
        $<TARGET_FILE_DIR:QuickLaunch>/
)

# 使 Visual Studio 在 exe 目录下查找资源
set_target_properties(QuickLaunch PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:QuickLaunch>"
)

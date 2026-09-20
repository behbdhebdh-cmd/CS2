cmake_minimum_required(VERSION 3.24)
project(CS2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(IMGUI_DEFINE_MATH_OPERATORS)

set(LIAN "${CMAKE_SOURCE_DIR}/vendor/imgui-lianflow")

add_executable(CS2 WIN32
    src/app/main.cpp
    src/app/config.cpp
    src/sdk/memory.cpp
    src/sdk/game.cpp
    src/sdk/vis.cpp
    src/sdk/offset_update.cpp
    src/features/esp.cpp
    src/features/combat.cpp
    src/features/hotkeys.cpp
    src/features/spec.cpp
    src/features/hitlog.cpp
    src/features/weapon_icons.cpp
    "${LIAN}/imgui.cpp"
    "${LIAN}/imgui_draw.cpp"
    "${LIAN}/imgui_tables.cpp"
    "${LIAN}/imgui_widgets.cpp"
    "${LIAN}/imgui_freetype.cpp"
    "${LIAN}/backends/imgui_impl_dx11.cpp"
    "${LIAN}/backends/imgui_impl_win32.cpp"
    "${LIAN}/examples/example_win32_directx11/custom_widgets.cpp"
)

target_include_directories(CS2 PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${LIAN}"
    "${LIAN}/backends"
    "${LIAN}/examples"
    "${LIAN}/examples/example_win32_directx11"
    "${LIAN}/freetype/include"
    "${LIAN}/SDK/Include"
)

target_link_directories(CS2 PRIVATE
    "${LIAN}/freetype/win64"
    "${LIAN}/SDK/Lib/x64"
)

target_link_libraries(CS2 PRIVATE
    d3d11
    d3dcompiler
    dxgi
    freetype
    d3dx11
    winmm
    dwmapi
    winhttp
)

target_compile_definitions(CS2 PRIVATE
    UNICODE
    _UNICODE
)

if (MSVC)
    target_compile_options(CS2 PRIVATE /utf-8 /W3 /MP)
    set_target_properties(CS2 PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()

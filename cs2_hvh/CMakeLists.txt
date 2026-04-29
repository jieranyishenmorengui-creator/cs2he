cmake_minimum_required(VERSION 3.20)
project(cs2_hvh VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS)

set(SOURCES
    src/main.cpp
    src/core/memory.cpp
    src/core/process.cpp
    src/core/overlay.cpp
    src/core/renderer.cpp
    src/core/offsets.cpp
    src/features/aimbot.cpp
    src/features/esp.cpp
    src/features/crosshair.cpp
    src/features/misc.cpp
    src/menu/menu.cpp
    src/config/config.cpp
    src/imgui/imgui.cpp
    src/imgui/imgui_draw.cpp
    src/imgui/imgui_widgets.cpp
    src/imgui/imgui_tables.cpp
    src/imgui/imgui_impl_dx11.cpp
    src/imgui/imgui_impl_win32.cpp
)

set(HEADERS
    src/utils/sdk.h
    src/utils/math.h
    src/core/memory.h
    src/core/process.h
    src/core/overlay.h
    src/core/renderer.h
    src/core/offsets.h
    src/features/aimbot.h
    src/features/esp.h
    src/features/crosshair.h
    src/features/misc.h
    src/menu/menu.h
    src/config/config.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d11
    dxgi
    d3dcompiler
    dwmapi
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /O2 /MT)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
endif()

cmake_minimum_required(VERSION 3.20)
project(chip8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(raylib REQUIRED)

# ---- Dear ImGui + rlImGui ----
include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.92.9b
    GIT_SHALLOW    TRUE
)
FetchContent_Declare(
    rlimgui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui rlimgui)

add_library(imgui_rl STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${rlimgui_SOURCE_DIR}/rlImGui.cpp
)
target_include_directories(imgui_rl SYSTEM PUBLIC
    ${imgui_SOURCE_DIR}
    ${rlimgui_SOURCE_DIR}
)
target_link_libraries(imgui_rl PUBLIC raylib)

# ---- Emulator core ----
add_library(chip8_core STATIC src/chip8.cpp)
target_include_directories(chip8_core PUBLIC src)
target_compile_options(chip8_core PRIVATE -Wall -Wextra -Wpedantic)
# TODO: remove once input (IsKeyDown/IsKeyReleased) and draw_debug
# are moved out of chip8.cpp -- the core should not depend on raylib.
target_link_libraries(chip8_core PUBLIC imgui_rl)

# ---- Emulator app ----
add_executable(chip8 src/main.cpp src/audio.cpp)
target_link_libraries(chip8 PRIVATE chip8_core raylib imgui_rl)
target_compile_options(chip8 PRIVATE -Wall -Wextra -Wpedantic)

# ---- Tests ----
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.8.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

add_executable(chip8_tests tests/test_opcodes.cpp)
target_link_libraries(chip8_tests PRIVATE chip8_core Catch2::Catch2WithMain)

enable_testing()
include(${catch2_SOURCE_DIR}/extras/Catch.cmake)
catch_discover_tests(chip8_tests)

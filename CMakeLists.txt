cmake_minimum_required(VERSION 3.20)
project(chip8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(raylib REQUIRED)

add_executable(chip8 src/main.cpp)
target_link_libraries(chip8 PRIVATE raylib)
target_compile_options(chip8 PRIVATE -Wall -Wextra -Wpedantic)

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(chip8)

set(CMAKE_C_STANDARD 99)

set(BUILD_SHARED_LIBS OFF)
add_subdirectory(vendors/SDL)

add_subdirectory(src)
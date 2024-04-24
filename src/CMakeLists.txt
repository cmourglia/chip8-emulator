add_executable(chip8 main.c cpu.c)

target_link_libraries(chip8 PRIVATE SDL3::SDL3)
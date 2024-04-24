#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef float f64;

void chip8_init();
void chip8_load_rom(const char* rom_file);
void chip8_cycle();

u8 chip8_get_screen_width();
u8 chip8_get_screen_height();

u8* chip8_get_keys();
u8* chip8_get_graphics_buffer();
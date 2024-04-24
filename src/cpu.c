#include "cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

u8 chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
    0x20, 0x60, 0x20, 0x20, 0x70,  // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
    0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
    0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
    0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
    0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
    0xF0, 0x80, 0xF0, 0x80, 0x80   // F
};

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

u8 memory[4096];
u8 graphics[SCREEN_WIDTH * SCREEN_HEIGHT];
u8 registers[16];

u16 pc;
u16 I;

u8 delay_timer;
u8 sound_timer;

u16 stack[16];
u16 sp;

u8 keys[16];

void chip8_init() {
    // Reset pointers
    pc = 0x200;
    I = 0;
    sp = 0;

    // Zero memory
    memset(memory, 0, sizeof(memory));
    memset(graphics, 0, sizeof(graphics));
    memset(registers, 0, sizeof(registers));
    memset(stack, 0, sizeof(stack));

    // Load font
    for (int i = 0; i < 80; i++) {
        memory[i] = chip8_fontset[i];
    }

    // Reset timers
    delay_timer = 0;
    sound_timer = 0;
}

void chip8_load_rom(const char* rom_file) {
    FILE* rom = fopen(rom_file, "rb");

    if (rom == NULL) {
        printf("Could not load rom `%s`\n", rom_file);
        return;
    }

    fseek(rom, 0, SEEK_END);
    long rom_size = ftell(rom);
    rewind(rom);

    u8* buf = malloc(rom_size);
    fread(buf, rom_size, 1, rom);
    fclose(rom);

    memcpy(memory + 0x200, buf, rom_size);
}

void chip8_cycle() {
    u16 opcode = (u16)memory[pc] << 8 | (u16)memory[pc + 1];

#define Vx registers[(opcode & 0x0F00) >> 8]
#define Vy registers[(opcode & 0x00F0) >> 4]
#define VF registers[0x0F]
#define kk (opcode & 0x00FF)
#define nnn (opcode & 0x0FFF)

#define INCREMENT_PC() pc += 2

    switch ((opcode >> 12) & 0x000F) {
        case 0x0: {
            u16 mode = opcode & 0x00FF;

            switch (mode) {
                // 00E0 - CLS
                case 0xE0:
                    memset(graphics, 0, sizeof(graphics));
                    INCREMENT_PC();
                    break;

                // 00EE - RET
                case 0xEE:
                    pc = stack[--sp];
                    INCREMENT_PC();
                    break;

                default: {
                    printf("Unknown opcode: 0x%X\n", opcode);
                } break;
            }
        } break;

        // 1nnn - JP addr
        case 0x1: {
            pc = nnn;
        } break;

        // 2nnn - CALL addr
        case 0x2: {
            stack[sp++] = pc;
            pc = nnn;
        } break;

        // 3xkk - SE Vx, byte
        case 0x3: {
            if (Vx == kk) {
                INCREMENT_PC();
            }
            INCREMENT_PC();
        } break;

        // 4xkk - SNE Vx, byte
        case 0x4: {
            if (Vx != kk) {
                INCREMENT_PC();
            }
            INCREMENT_PC();
        } break;

        // 5xy0 - SE Vx, Vy
        case 0x5: {
            if (Vx == Vy) {
                INCREMENT_PC();
            }
            INCREMENT_PC();
        } break;

        // 6xkk - LD Vx, byte
        case 0x6: {
            Vx = kk;
            INCREMENT_PC();
        } break;

        // 7xkk - ADD Vx, byte
        case 0x7: {
            Vx += kk;
            INCREMENT_PC();
        } break;

        case 0x8: {
            u8 mode = (opcode & 0x000F);

            switch (mode) {
                // 8xy0 - LD Vx, Vy
                case 0x0: {
                    Vx = Vy;
                } break;

                // 8xy1 - OR Vx, Vy
                case 0x1: {
                    Vx |= Vy;
                } break;

                // 8xy2 - AND Vx, Vy
                case 0x2: {
                    Vx &= Vy;
                } break;

                // 8xy3 - XOR Vx, Vy
                case 0x3: {
                    Vx ^= Vy;
                } break;

                // 8xy4 - ADD Vx, Vy
                case 0x4: {
                    u16 sum = (u16)Vx + (u16)Vy;

                    VF = sum > 255 ? 1 : 0;
                    Vx = (u8)(sum & 0x00FF);
                } break;

                // 8xy5 - SUB Vx, Vy
                case 0x5: {
                    VF = Vx > Vy ? 1 : 0;
                    Vx -= Vy;
                } break;

                // 8xy6 - SHR Vx{, Vy}
                case 0x6: {
                    VF = Vx & 0x01;
                    Vx = Vx >> 1;
                } break;

                // 8xy7 - SUBN Vx, Vy
                case 0x7: {
                    VF = Vy > Vx ? 1 : 0;
                    Vx = Vy - Vx;
                } break;

                // 8xyE - SHL Vx{, Vy}
                case 0xE: {
                    VF = Vx & 0x80;
                    Vx = Vx << 1;
                } break;

                default: {
                    printf("Unknown opcode: 0x%X\n", opcode);
                } break;
            }

            INCREMENT_PC();
        } break;

        // 9xy0 - SNE Vx, Vy
        case 0x9: {
            if (Vx != Vy) {
                INCREMENT_PC();
            }
            INCREMENT_PC();
        } break;

        // Annn - LD I, addr
        case 0xA: {
            I = nnn;
            INCREMENT_PC();
        } break;

        // Bnnn - JP V0, addr
        case 0xB: {
            pc = nnn + registers[0];
        } break;

        // Cxkk - RND Vx, byte
        case 0xC: {
            Vx = (rand() % 255) & kk;
            INCREMENT_PC();
        } break;

        // Dxyn - DRW Vx, Vy, nibble
        case 0xD: {
            // A sprite is composed of up to 15 bytes. Each byte is a line of
            // the sprite to be drawn. Each bit of the byte indicates wether a
            // pixel is lit or not e.g. 10010011 would mean X--X--XX (X being
            // lit, - not)

            u8 n = opcode & 0x000F;

            VF = 0;

            for (int j = 0; j < n; j++) {
                u8 sprite_row = memory[I + j];

                u8 mask = 0x80;
                for (int i = 0; i < 8; i++) {
                    if (sprite_row & (mask >> i)) {
                        u16 x = (Vx + i) % SCREEN_WIDTH;
                        u16 y = (Vy + j) % SCREEN_HEIGHT;

                        u16 pixel_index = x + y * SCREEN_WIDTH;

                        if (graphics[pixel_index] == 1) {
                            VF = 1;
                        }

                        graphics[pixel_index] ^= 1;
                    }
                }
            }

            INCREMENT_PC();
        } break;

        case 0xE: {
            u16 mode = opcode & 0x00FF;

            switch (mode) {
                // Ex9E - SKP Vx
                case 0x9E: {
                    if (keys[Vx] == 1) {
                        INCREMENT_PC();
                    }
                    INCREMENT_PC();
                } break;

                // ExA1 - SKNP Vx
                case 0xA1: {
                    if (keys[Vx] == 0) {
                        INCREMENT_PC();
                    }
                    INCREMENT_PC();
                } break;

                default: {
                    printf("Unknown opcode: 0x%X\n", opcode);
                } break;
            }
        } break;

        case 0xF: {
            u16 mode = opcode & 0x00FF;

            switch (mode) {
                // Fx07 - LD Vx, DT
                case 0x07: {
                    Vx = delay_timer;
                } break;

                // Fx0A - LD Vx, K
                case 0x0A: {
                    bool key_pressed = false;
                    for (int i = 0; i < 16 && !key_pressed; i++) {
                        if (keys[i] != 0) {
                            Vx = i;
                            key_pressed = true;
                        }
                    }

                    if (!key_pressed) {
                        // Suspend execution until a key is pressed
                        return;
                    }
                } break;

                // Fx15 - LD DT, Vx
                case 0x15: {
                    delay_timer = Vx;
                } break;

                // Fx18 - LD ST, Vx
                case 0x18: {
                    sound_timer = Vx;
                } break;

                // Fx1E - ADD I, Vx
                case 0x1E: {
                    VF = (I + Vx > 0xFFF) ? 1 : 0;
                    I = I + Vx;
                } break;

                // Fx29 - LD F, Vx
                case 0x29: {
                    I = Vx * 0x5;  // 5 bytes per hexadecimal sprite
                } break;

                // Fx33 - LD B, Vx
                case 0x33: {
                    memory[I] = Vx / 100;
                    memory[I + 1] = (Vx / 10) % 10;
                    memory[I + 2] = Vx % 10;
                } break;

                // Fx55 - LD [I], Vx
                case 0x55: {
                    u16 x = (opcode & 0x0F00) >> 8;

                    for (int i = 0; i <= x; i++) {
                        memory[I + i] = registers[i];
                    }
                } break;

                // Fx65 - LD Vx, [I]
                case 0x65: {
                    u16 x = (opcode & 0x0F00) >> 8;

                    for (int i = 0; i <= x; i++) {
                        registers[i] = memory[I + i];
                    }
                } break;

                default: {
                    printf("Unknown opcode: 0x%X\n", opcode);
                } break;
            }

            INCREMENT_PC();
        } break;

        default:
            printf("Unknown opcode: 0x%X\n", opcode);
    }

#undef Vx
#undef Vy
#undef VF
#undef kk
#undef INCREMENT_PC

    // Update timers
    if (delay_timer > 0) {
        delay_timer -= 1;
    }

    if (sound_timer > 0) {
        if (sound_timer == 1) {
            printf("BEEP\n");
        }
        sound_timer -= 1;
    }
}

u8 chip8_get_screen_width() { return SCREEN_WIDTH; }
u8 chip8_get_screen_height() { return SCREEN_HEIGHT; }

u8* chip8_get_keys() { return keys; }
u8* chip8_get_graphics_buffer() { return graphics; }
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "cpu.h"

// clang-format off
// #define USE_COLEMAK
#ifdef USE_COLEMAK
// NOTE: Colemak keymap
static const u32 keymap[] = {
    SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4,
    SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_F, SDL_SCANCODE_P,
    SDL_SCANCODE_A, SDL_SCANCODE_R, SDL_SCANCODE_S, SDL_SCANCODE_T,
    SDL_SCANCODE_Z, SDL_SCANCODE_X, SDL_SCANCODE_C, SDL_SCANCODE_D,
};
#else
// NOTE: QWERTY keymap
static const u32 keymap[] = {
    SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4,
    SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_R,
    SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_F,
    SDL_SCANCODE_Z, SDL_SCANCODE_X, SDL_SCANCODE_C, SDL_SCANCODE_V,
};
#endif
// clang-format on

#define ARRAY_COUNT(x) (sizeof(x) / sizeof(x[0]))

void fill_screen(SDL_Texture* texture) {
    u32* buffer = NULL;
    int pitch = 0;

    SDL_LockTexture(texture, NULL, &buffer, &pitch);
    for (int i = 0; i < chip8_get_screen_width() * chip8_get_screen_height();
         i++) {
        buffer[i] =
            chip8_get_graphics_buffer()[i] == 1 ? 0xFFFFFFFF : 0x000000FF;
    }
    SDL_UnlockTexture(texture);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: chip8 rom_file\n");
        exit(1);
    }

    i32 screen_width = 1280;
    i32 screen_height = 640;

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window =
        SDL_CreateWindow("CHIP-8 Emulator", screen_width, screen_height, 0);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL, 0);

    if (renderer == NULL) {
        printf("Renderer creation error: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Texture* texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
        chip8_get_screen_width(), chip8_get_screen_height());
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    chip8_init();
    chip8_load_rom(argv[1]);

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    running = false;
                } break;

                case SDL_EVENT_KEY_DOWN: {
                    if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                        running = false;
                    }

                    for (int i = 0; i < ARRAY_COUNT(keymap); i++) {
                        if (event.key.keysym.scancode == keymap[i]) {
                            chip8_get_keys()[i] = 1;
                        }
                    }
                } break;

                case SDL_EVENT_KEY_UP: {
                    for (int i = 0; i < ARRAY_COUNT(keymap); i++) {
                        if (event.key.keysym.scancode == keymap[i]) {
                            chip8_get_keys()[i] = 0;
                        }
                    }
                } break;
            }
        }

        chip8_cycle();

        fill_screen(texture);

        SDL_Rect dst = {.x = 0, .y = 0, .w = screen_width, .h = screen_height};

        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // SDL_Delay(12);
    }

    return 0;
}
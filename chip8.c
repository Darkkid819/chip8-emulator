#include "chip8.h"
#include <stdio.h>
#include <string.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 320

// #define DEBUG

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* texture = NULL;
uint32_t pixels[SCREEN_WIDTH * SCREEN_HEIGHT];

int initializeSDL(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    window = SDL_CreateWindow("CHIP-8 Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    if (texture == NULL) {
        printf("Texture could not be created! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    return 1;
}

void renderDisplay(void) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            uint8_t pixel = display[x][y];
            pixels[y * SCREEN_WIDTH + x] = pixel ? 0xFFFFFFFF : 0xFF000000;  // white for on, black for off
        }
    }

    SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

int handleEvents(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            return 0;
        }
    }
    return 1;
}

void cleanupSDL(void) {
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <ROM file>\n", argv[0]);
        return -1;
    }

    if (!initializeSDL()) {
        return -1;
    }

    initializeMemory();

    if (!loadROM(argv[1])) {
        return -1;
    }

    int running = 1;
    while (running) {
        executeCycle();
        updateTimers();
        renderDisplay();
        running = handleEvents();
        SDL_Delay(1000 / 60);  // Run at 60Hz
    }

    cleanupSDL();
    return 0;
}


void initializeMemory(void) {
    for(int i = 0; i < MEMORY_SIZE; i++) {
        memory[i] = 0;
    }

    for(int i = 0; i < 80; i++) {
        memory[0x050 + i] = fontset[i];
    }

    sp = 0;
    delayTimer = 0;
    soundTimer = 0;

    pc = START_ADDRESS;
    I = 0;
    
    memset(keypad, 0, sizeof(keypad));
}

int loadROM(const char* filename) {
    FILE* rom = fopen(filename, "rb");
    if (rom == NULL) {
        printf("Failed to open ROM: %s\n", filename);
        return 0;
    }

    // read the ROM contents into memory starting at 0x200
    fread(&memory[START_ADDRESS], sizeof(uint8_t), MEMORY_SIZE - START_ADDRESS, rom);

    fclose(rom);
    return 1;
}

void clearDisplay(void) {
    memset(display, 0, sizeof(display));
}

uint8_t drawSprite(uint8_t x, uint8_t y, uint8_t height, const uint8_t *sprite) {
    uint8_t pixelErased = 0;

    for (int row = 0; row < height; row++) {
        uint8_t spriteRow = sprite[row];

        for (int col = 0; col < 8; col++) {
            if ((spriteRow & (0x80 >> col)) != 0) {
                int xCoord = (x + col) % SCREEN_WIDTH;
                int yCoord = (y + row) % SCREEN_HEIGHT;

                if (display[xCoord][yCoord] == 1) {
                    pixelErased = 1;
                }
                display[xCoord][yCoord] ^= 1;
            }
        }
    }

    return pixelErased;
}

void pushStack(uint16_t value) {
    if (sp < STACK_SIZE) {
        stack[sp++] = value;
    }
    // handle overflow if necessary
}

uint16_t popStack(void) {
    if (sp > 0) {
        return stack[--sp];
    }
    // handle underflow if necessary
    return 0;
}

void updateTimers(void) {
    if (delayTimer > 0) {
        delayTimer--;
    }

    if (soundTimer > 0) {
        soundTimer--;

        if (soundTimer == 0) {
            // trigger beep
        }
    }
}

void executeCycle(void) {
    uint16_t opcode = memory[pc] << 8 | memory[pc + 1];

    #ifdef DEBUG
    printf("Executing opcode: 0x%04X at PC: 0x%04X\n", opcode, pc);
    #endif

    pc += 2;

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode & 0x00FF) {
                case 0x00E0:  
                    // 00E0: clear screen
                    clearDisplay();
                    break;

                case 0x00EE:  
                    // 00EE: return from a subroutine
                    pc = popStack();
                    break;
                
                default:
                    printf("Unknown opcode: 0x%x\n", opcode);
                    break;
            }
            break;
        
        case 0x1000:
            // 1NNN: jump to address NNN
            pc = opcode & 0x0FFF;
            break;

        case 0x2000:
            // 2NNN: call subroutine at NNN
            pushStack(pc);
            pc = opcode & 0x0FFF;
            break;

        case 0x6000:
            // 6XNN: set VX to NN
            V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            break;

        case 0x7000:
            // 7XNN: add NN to VX (no carry)
            V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            break;

        case 0xA000:
            // ANNN: set I to NNN
            I = opcode & 0x0FFF;
            break;

        case 0xD000: {
            // DXYN: display
            uint8_t x = V[(opcode & 0x0F00) >> 8];
            uint8_t y = V[(opcode & 0x00F0) >> 4];
            uint8_t height = opcode & 0x000F;
            uint8_t pixelErased = 0;

            V[0xF] = 0;  // reset VF flag

            for (int row = 0; row < height; row++) {
                uint8_t spriteRow = memory[I + row];

                for (int col = 0; col < 8; col++) {
                    if ((spriteRow & (0x80 >> col)) != 0) {
                        int xCoord = (x + col) % SCREEN_WIDTH;
                        int yCoord = (y + row) % SCREEN_HEIGHT;

                        if (display[xCoord][yCoord] == 1) {
                            V[0xF] = 1;  // collision
                        }

                        display[xCoord][yCoord] ^= 1;
                    }
                }
            }
            break;
        }

        default:
            printf("Unknown opcode: 0x%X\n", opcode);
            break;
    }
}

void setKeyDown(uint8_t key) {
    if (key < KEYPAD_SIZE) {
        keypad[key] = 1;
    }
}

void setKeyUp(uint8_t key) {
    if (key < KEYPAD_SIZE) {
        keypad[key] = 0;
    }
}

uint8_t isKeyPressed(uint8_t key) {
    if (key < KEYPAD_SIZE) {
        return keypad[key];
    }
    return 0;
}
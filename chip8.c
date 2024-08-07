#include "chip8.h"

#include <stdio.h>
#include <stdlib.h>
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

uint8_t sdlKeyToChip8Key(SDL_Keycode key) {
    switch (key) {
        case SDLK_1: return 0x1;
        case SDLK_2: return 0x2;
        case SDLK_3: return 0x3;
        case SDLK_4: return 0xC;
        case SDLK_q: return 0x4;
        case SDLK_w: return 0x5;
        case SDLK_e: return 0x6;
        case SDLK_r: return 0xD;
        case SDLK_a: return 0x7;
        case SDLK_s: return 0x8;
        case SDLK_d: return 0x9;
        case SDLK_f: return 0xE;
        case SDLK_z: return 0xA;
        case SDLK_x: return 0x0;
        case SDLK_c: return 0xB;
        case SDLK_v: return 0xF;
        default: return 0xFF;  // invalid key
    }
}

int handleEvents(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            return 0;
        } else if (e.type == SDL_KEYDOWN) {
            uint8_t chip8Key = sdlKeyToChip8Key(e.key.keysym.sym);
            if (chip8Key != 0xFF) {
                setKeyDown(chip8Key);
            }
        } else if (e.type == SDL_KEYUP) {
            uint8_t chip8Key = sdlKeyToChip8Key(e.key.keysym.sym);
            if (chip8Key != 0xFF) {
                setKeyUp(chip8Key);
            }
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

        case 0x3000:
            // 3XNN: skip the next instruction if VX == NN
            if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
                pc += 2;
            } 
            break;

        case 0x4000:
            // 4XNN: skip the next instruction if VX != NN
            if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
                pc += 2;
            } 
            break;

        case 0x5000:
            // 5XY0: skip the next instruction if VX == VY
            if (V[(opcode & 0x00F0) >> 4] == V[(opcode & 0x0F00) >> 8]) {
                pc += 2;
            }
            break;

        case 0x6000:
            // 6XNN: set VX to NN
            V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            break;

        case 0x7000:
            // 7XNN: add NN to VX (no carry)
            V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            break;

        case 0x8000:
            switch (opcode & 0x000F) {
                case 0x0000:
                    // 8XY0: set VX = VY
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
                    break;

                case 0x0001:
                    // 8xY1: set VX = VX OR VY
                    V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
                    break;

                case 0x0002:
                    // 8XY2: set VX = VX AND VY
                    V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
                    break;

                case 0x0003:
                    // 8XY3: set VX = VX XOR VY
                    V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
                    break;

                case 0x0004:
                    // 8XY4: add VY to VX, set VF to 1 if theres a carry, 0 otherwise
                    if (V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8])) {
                        V[0xF] = 1;
                    } else {
                        V[0xF] = 0;
                    }
                    V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
                    break;

                case 0x0005:
                    // 8XY5: subtract VY from VX. set VF to 0 if theres a borrow, 1 otherwise
                    if (V[(opcode & 0x0F00) >> 8] >= V[(opcode & 0x00F0) >> 4]) {
                        V[0xF] = 1;
                    } else {
                        V[0xF] = 0;
                    }
                    V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
                    break;

                case 0x0007:
                    // 8XY7: set VX = VY - VX. set VF to 0 if theres a borrow, 1 otherwise
                    if (V[(opcode & 0x00F0) >> 4] >= V[(opcode & 0x0F00) >> 8]) {
                        V[0xF] = 1;
                    } else {
                        V[0xF] = 0;
                    }
                    V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
                    break;

                case 0x0006:
                    // 8XY6: shift VX right by one. VF is set to the value of LSB of VX before shift
                    V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
                    V[(opcode & 0x0F00) >> 8] >>= 1;
                    break;

                case 0x000E:
                    // 8XYE: shift VX left by one. VF is set to the value of MSB of VX before shift
                    V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
                    V[(opcode & 0x0F00) >> 8] <<= 1;
                    break;

                default:
                    printf("Unknown opcode: 0x%x\n", opcode);
                    break;
            }
            break;

        case 0x9000:
            // 9XY0: skip the next instruction if VX != VY
            if (V[(opcode & 0x00F0) >> 4] != V[(opcode & 0x0F00) >> 8]) {
                pc += 2;
            }
            break;

        case 0xA000:
            // ANNN: set I to NNN
            I = opcode & 0x0FFF;
            break;

        case 0xB000:
            // BNNN: jump to address NNN + V0
            pc = (opcode * 0x0FFF) + V[0];
            break;

        case 0xC000:
            // CXNN: set VX to a random number ANDed with NN
            V[(opcode & 0x0F00) >> 8] = (rand() % 256) & (opcode & 0x00FF);
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

        case 0xE000:
            switch (opcode & 0x00FF) {
                case 0x009E:
                    // EX9E: skip the next instruction if the key with the value in VX is pressed
                    if (isKeyPressed(V[(opcode & 0x0F00) >> 8])) {
                        pc += 2;
                    }
                    break;

                case 0x00A1:
                    // EXA1: skip the next instruction if the key with the value in VX is not pressed
                    if (!isKeyPressed(V[(opcode & 0x0F00) >> 8])) {
                        pc += 2;
                    }
                    break;

                default:
                    printf("Unknown opcode: 0x%x\n", opcode);
                    break;
            }
            break;

        case 0xF000:
            switch (opcode & 0x00FF) {
                case 0x0007:
                    // FX07: set VX to the current value of the delay timer
                    V[(opcode & 0x0F00) >> 8] = delayTimer;
                    break;

                case 0x000A: {
                    // FX0A: wait for a key press and store it in VX
                    uint8_t keyPressed = 0;

                    for (uint8_t i = 0; i < KEYPAD_SIZE; i++) {
                        if (isKeyPressed(i)) {
                            V[(opcode & 0x0F00) >> 8] = i;
                            keyPressed = 1;
                            break;
                        }
                    }

                    // retry
                    if (!keyPressed) {
                        pc -= 2;
                    }
                    break;
                }

                case 0x0015:
                    // FX15: set the delay timer to the value in VX
                    delayTimer = V[(opcode & 0x0F00) >> 8];
                    break;

                case 0x0018:
                    // FX18: set the sound timer to the value in VX
                    soundTimer = V[(opcode & 0x0F00) >> 8];
                    break;

                case 0x001E:
                    // FX1E: add VX to I, set VF to 1 if theres overflow (I > 0xFFF)
                    if (I + V[(opcode & 0x0F00) >> 8] > 0xFFF) {
                        V[0xF] = 1;
                    } else {
                        V[0xF] = 0;
                    }
                    I += V[(opcode & 0x0F00) >> 8];
                    break;

                case 0x0029:
                    // FX29: set I to the memory address of the sprite data corresponding to the hex digit in VX
                    I = 0x050 + (V[(opcode & 0x0F00) >> 8] * 5); 
                    break;

                case 0x0033: {
                    // FX33: store BCD representation of VX in memory locations I, I+1, and I+2
                    uint8_t value = V[(opcode & 0x0F00) >> 8];
                    memory[I] = value / 100;  // hundreds digit
                    memory[I + 1] = (value / 10) % 10;  // tens digit
                    memory[I + 2] = value % 10;  // ones digit
                    break;
                }

                case 0x0055: {
                    // FX55: store V0 to VX in memory starting at address I
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    for (uint8_t i = 0; i <= x; i++) {
                        memory[I + i] = V[i];
                    }
                    // I is unchanged in modern CHIP-8 interpreters
                    break;
                }

                case 0x0065: {
                    // FX65: load V0 to VX from memory starting at address I
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    for (uint8_t i = 0; i <= x; i++) {
                        V[i] = memory[I + i];
                    }
                    // I is unchanged in modern CHIP-8 interpreters
                    break;
                }

                default:
                    printf("Unknown opcode: 0x%x\n", opcode);
                    break;
            }
            break;

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
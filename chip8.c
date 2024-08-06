#include "chip8.h"
#include <string.h>

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
    pc += 2;

    // implement instruction set
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
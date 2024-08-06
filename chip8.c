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
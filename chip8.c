#include "chip8.h"
#include <string.h>

void initialize_memory(void) {
    for(int i = 0; i < MEMORY_SIZE; i++) {
        memory[i] = 0;
    }

    for(int i = 0; i < 80; i++) {
        memory[0x050 + i] = fontset[i];
    }
}

void clear_display(void) {
    memset(display, 0, sizeof(display));
}

uint8_t draw_sprite(uint8_t x, uint8_t y, uint8_t height, const uint8_t *sprite) {
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

void push_stack(uint16_t value) {
    if (sp < STACK_SIZE) {
        stack[sp++] = value;
    }
    // handle overflow if necessary
}

uint16_t pop_stack(void) {
    if (sp > 0) {
        return stack[--sp];
    }
    // handle underflow if necessary
    return 0;
}
/**
 * MatryoshkaOS - Minimal Test Kernel
 * Just display a message to test if kernel boots
 */

#include <matryoshka/types.h>

// VGA memory
static uint16_t *vga_buffer = (uint16_t*)0xB8000;

void kernel_main(void) {
    const char *msg = "MatryoshkaOS Test - Kernel Booted!";
    
    // Clear screen
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = 0x0F20;  // White on black space
    }
    
    // Write message
    int i = 0;
    while (msg[i]) {
        vga_buffer[i] = 0x0F00 | msg[i];  // White on black
        i++;
    }
    
    // Halt
    while (1) {
        __asm__ volatile("hlt");
    }
}


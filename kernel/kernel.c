/**
 * MatryoshkaOS - Kernel Main
 * Entry point after assembly initialization
 */

#include <matryoshka/vga.h>

/**
 * Kernel main entry point
 * Called from entry.asm after CPU initialization
 */
void kernel_main(void) {
    // Initialize VGA driver
    vga_init();
    
    // Display welcome message
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("MatryoshkaOS v1.0\n");
    
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("====================================\n\n");
    
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("Kernel booted successfully!\n\n");
    
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("System Information:\n");
    vga_puts("  - Architecture: x86_64\n");
    vga_puts("  - Bootloader: GRUB2 Multiboot2\n");
    vga_puts("  - VGA Mode: 80x25 text mode\n\n");
    
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Status:\n");
    vga_puts("  [OK] VGA driver initialized\n");
    vga_puts("  [OK] Kernel running in long mode\n\n");
    
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("System halted. Next steps:\n");
    vga_puts("  1. Implement memory management\n");
    vga_puts("  2. Setup interrupt handling\n");
    vga_puts("  3. Add process management\n\n");
    
    vga_set_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    vga_puts("MatryoshkaOS - Built for System QA Portfolio\n");
    
    // Halt CPU
    while (1) {
        __asm__ volatile("hlt");
    }
}


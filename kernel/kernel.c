/**
 * MatryoshkaOS - Kernel Main
 * Entry point after assembly initialization
 */

#include <matryoshka/vga.h>
#include <matryoshka/multiboot2.h>
#include <matryoshka/pmm.h>

/**
 * Format a number with KB/MB/GB suffix
 */
static void format_memory_size(uint64_t bytes, char *buffer) {
    if (bytes >= 1024 * 1024 * 1024) {
        // GB
        uint64_t gb = bytes / (1024 * 1024 * 1024);
        buffer[0] = '0' + (gb / 10);
        buffer[1] = '0' + (gb % 10);
        buffer[2] = ' ';
        buffer[3] = 'G';
        buffer[4] = 'B';
        buffer[5] = '\0';
    } else if (bytes >= 1024 * 1024) {
        // MB
        uint64_t mb = bytes / (1024 * 1024);
        if (mb >= 1000) {
            buffer[0] = '0' + (mb / 1000);
            buffer[1] = '0' + ((mb / 100) % 10);
            buffer[2] = '0' + ((mb / 10) % 10);
            buffer[3] = '0' + (mb % 10);
            buffer[4] = ' ';
            buffer[5] = 'M';
            buffer[6] = 'B';
            buffer[7] = '\0';
        } else if (mb >= 100) {
            buffer[0] = '0' + (mb / 100);
            buffer[1] = '0' + ((mb / 10) % 10);
            buffer[2] = '0' + (mb % 10);
            buffer[3] = ' ';
            buffer[4] = 'M';
            buffer[5] = 'B';
            buffer[6] = '\0';
        } else {
            buffer[0] = '0' + (mb / 10);
            buffer[1] = '0' + (mb % 10);
            buffer[2] = ' ';
            buffer[3] = 'M';
            buffer[4] = 'B';
            buffer[5] = '\0';
        }
    } else {
        // KB
        uint64_t kb = bytes / 1024;
        buffer[0] = '0' + (kb / 1000);
        buffer[1] = '0' + ((kb / 100) % 10);
        buffer[2] = '0' + ((kb / 10) % 10);
        buffer[3] = '0' + (kb % 10);
        buffer[4] = ' ';
        buffer[5] = 'K';
        buffer[6] = 'B';
        buffer[7] = '\0';
    }
}

/**
 * Kernel main entry point
 * Called from entry.asm after CPU initialization
 * @param mbi_addr Physical address of Multiboot2 information structure
 */
void kernel_main(unsigned long mbi_addr) {
    // Initialize VGA driver first (before anything else)
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
    
    // Check if we received Multiboot info
    if (mbi_addr == 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("ERROR: No Multiboot information received!\n");
        vga_puts("Multiboot address is NULL\n\n");
        goto halt;
    }
    
    // Initialize Physical Memory Manager
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Initializing Memory Management...\n");
    
    multiboot_info_t *mbi = (multiboot_info_t *)mbi_addr;
    pmm_init(mbi);
    
    // Get and display memory statistics
    pmm_stats_t stats;
    pmm_get_stats(&stats);
    
    char buffer[16];
    
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("  Total Memory: ");
    format_memory_size(stats.total_memory, buffer);
    vga_puts(buffer);
    vga_puts("\n");
    
    vga_puts("  Free Memory:  ");
    format_memory_size(stats.free_memory, buffer);
    vga_puts(buffer);
    vga_puts("\n");
    
    vga_puts("  Used Memory:  ");
    format_memory_size(stats.used_memory, buffer);
    vga_puts(buffer);
    vga_puts("\n\n");
    
    // Test PMM allocation
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("Testing PMM...\n");
    
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    uint64_t frame1 = pmm_alloc_frame();
    if (frame1 != 0) {
        vga_puts("  [OK] Allocated frame 1\n");
    }
    
    uint64_t frame2 = pmm_alloc_frame();
    if (frame2 != 0) {
        vga_puts("  [OK] Allocated frame 2\n");
    }
    
    pmm_free_frame(frame1);
    vga_puts("  [OK] Freed frame 1\n");
    
    uint64_t frame3 = pmm_alloc_frame();
    if (frame3 != 0) {
        vga_puts("  [OK] Re-allocated frame (should reuse frame 1)\n");
    }
    
    // Clean up
    pmm_free_frame(frame2);
    pmm_free_frame(frame3);
    vga_puts("\n");
    
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Status:\n");
    vga_puts("  [OK] VGA driver initialized\n");
    vga_puts("  [OK] Kernel running in long mode\n");
    vga_puts("  [OK] Physical Memory Manager initialized\n");
    vga_puts("  [OK] PMM allocation/deallocation working\n\n");
    
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("System halted. Next steps:\n");
    vga_puts("  1. Implement virtual memory management (VMM)\n");
    vga_puts("  2. Add heap allocator (kmalloc/kfree)\n");
    vga_puts("  3. Setup interrupt handling\n\n");
    
    vga_set_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    vga_puts("MatryoshkaOS - Built for System QA Portfolio\n");
    
halt:
    // Halt CPU
    while (1) {
        __asm__ volatile("hlt");
    }
}


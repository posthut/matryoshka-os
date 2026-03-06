/**
 * MatryoshkaOS - Kernel Main
 * Entry point after assembly initialization
 */

#include <matryoshka/vga.h>
#include <matryoshka/serial.h>
#include <matryoshka/gdt.h>
#include <matryoshka/multiboot2.h>
#include <matryoshka/pmm.h>
#include <matryoshka/heap.h>
#include <matryoshka/idt.h>
#include <matryoshka/pic.h>
#include <matryoshka/timer.h>
#include <matryoshka/keyboard.h>
#include <matryoshka/vmm.h>
#include <matryoshka/vfs.h>
#include <matryoshka/ramfs.h>
#include <matryoshka/e1000.h>
#include <matryoshka/net.h>
#include <matryoshka/task.h>
#include <matryoshka/shell.h>

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

/* Network polling task — runs in background, handles ARP/ICMP */
static void net_task(void) {
    while (1) {
        net_poll();
        task_yield();
    }
}

/* Demo tasks — preempted by timer, no explicit yield needed */
static void demo_task_a(void) {
    for (int i = 0; i < 5; i++) {
        vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        vga_puts("[Task A] tick ");
        vga_putchar('0' + i);
        vga_putchar('\n');
        timer_sleep_ms(80);
    }
}

static void demo_task_b(void) {
    for (int i = 0; i < 5; i++) {
        vga_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
        vga_puts("[Task B] tock ");
        vga_putchar('0' + i);
        vga_putchar('\n');
        timer_sleep_ms(80);
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
    serial_init();
    klog("MatryoshkaOS booting...");
    
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
    
    // Setup own GDT before touching memory that might overlap GRUB's GDT
    gdt_init();
    
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
    
    // Initialize Heap Allocator
    if (heap_init() != 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("ERROR: Failed to initialize heap!\n");
        goto halt;
    }
    
    // Test Heap Allocator
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("Testing Heap Allocator...\n");
    
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("  Test 1: Basic allocation...\n");
    
    // Test 1: Basic allocation
    char *str1 = (char *)kmalloc(64);
    if (str1) {
        vga_puts("  [OK] Allocated 64 bytes\n");
        
        // Write to allocated memory
        const char *test_msg = "Hello from heap!";
        for (int i = 0; test_msg[i] != '\0'; i++) {
            str1[i] = test_msg[i];
        }
        str1[16] = '\0';
        
        vga_puts("  [OK] Wrote to allocated memory: ");
        vga_puts(str1);
        vga_puts("\n");
    }
    
    vga_puts("  Test 2: Multiple allocations...\n");
    
    // Test 2: Multiple allocations
    void *ptr1 = kmalloc(128);
    void *ptr2 = kmalloc(256);
    void *ptr3 = kmalloc(512);
    
    if (ptr1 && ptr2 && ptr3) {
        vga_puts("  [OK] Multiple allocations (128, 256, 512 bytes)\n");
    }
    
    vga_puts("  Test 3: kzalloc...\n");
    
    // Test 3: kzalloc (zeroed allocation)
    uint32_t *numbers = (uint32_t *)kzalloc(16);
    if (numbers) {
        bool all_zero = true;
        for (int i = 0; i < 4; i++) {
            if (numbers[i] != 0) {
                all_zero = false;
                break;
            }
        }
        if (all_zero) {
            vga_puts("  [OK] kzalloc properly zeroed memory\n");
        }
    }
    
    vga_puts("  Test 4: Free and reallocation...\n");
    
    // Test 4: Free and reallocation
    kfree(ptr2);
    vga_puts("  [OK] Freed middle allocation\n");
    
    void *ptr4 = kmalloc(128);
    if (ptr4) {
        vga_puts("  [OK] Re-allocated freed memory\n");
    }
    
    vga_puts("  Test 5: Heap statistics...\n");
    
    // Test 5: Heap statistics
    size_t total, used, free;
    heap_get_stats(&total, &used, &free);
    
    vga_puts("  Heap Stats: ");
    format_memory_size(used, buffer);
    vga_puts(buffer);
    vga_puts(" used / ");
    format_memory_size(total, buffer);
    vga_puts(buffer);
    vga_puts(" total\n\n");
    
    vga_puts("  Cleaning up test allocations...\n");
    
    // Clean up test allocations
    kfree(str1);
    kfree(ptr1);
    kfree(ptr3);
    kfree(ptr4);
    kfree(numbers);
    
    vga_puts("  [OK] All tests completed!\n\n");
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("About to initialize IDT...\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    // Initialize Interrupt Descriptor Table
    idt_init();
    
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("IDT initialized, testing...\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    // Initialize Programmable Interrupt Controller
    pic_init();
    
    // Initialize Timer (registers IRQ0 handler and unmasks IRQ0)
    timer_init();
    
    // Enable interrupts — GDT, IDT, PIC and timer are all set up
    __asm__ volatile("sti");
    
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] Interrupts enabled (STI)\n\n");
    
    // Let a few timer ticks accumulate to prove interrupts work
    timer_sleep_ms(50);
    
    uint64_t ticks = timer_get_ticks();
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("Timer test: ");
    char tick_buf[12];
    int pos = 0;
    uint64_t t = ticks;
    if (t == 0) {
        tick_buf[pos++] = '0';
    } else {
        char tmp[12];
        int len = 0;
        while (t > 0) { tmp[len++] = '0' + (t % 10); t /= 10; }
        for (int i = len - 1; i >= 0; i--) tick_buf[pos++] = tmp[i];
    }
    tick_buf[pos] = '\0';
    vga_puts(tick_buf);
    vga_puts(" ticks counted - interrupts working!\n\n");
    
    // Initialize PS/2 Keyboard (IRQ1)
    keyboard_init();
    
    // Initialize Virtual Memory Manager (enable paging)
    if (vmm_init() != 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("ERROR: Failed to initialize VMM!\n");
        goto halt;
    }
    
    // Initialize Virtual File System + RAM filesystem
    vfs_init();
    ramfs_init();
    
    // Initialize network (e1000 + IP stack)
    if (e1000_init() == 0) {
        net_init();
    }
    
    // Initialize task scheduler and register yield as keyboard wait
    task_init();
    keyboard_set_wait_func(task_yield);
    
    // Demo: spawn two short-lived tasks to demonstrate multitasking
    task_create(demo_task_a, "demo_a");
    task_create(demo_task_b, "demo_b");
    
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Running preemptive multitasking demo...\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    // Sleep while demo tasks are preemptively scheduled
    timer_sleep_ms(1000);
    
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("Demo complete. Entering shell.\n\n");
    
    // Start background network polling task
    if (e1000_link_up()) {
        task_create(net_task, "net");
    }
    
    klog("Boot complete, entering shell");
    
    // Enter interactive shell (never returns)
    shell_run();
    
halt:
    __asm__ volatile("cli");
    while (1) {
        __asm__ volatile("hlt");
    }
}


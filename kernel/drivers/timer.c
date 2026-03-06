/**
 * MatryoshkaOS - Programmable Interval Timer (PIT)
 * System timer driver
 */

#include <matryoshka/timer.h>
#include <matryoshka/idt.h>
#include <matryoshka/pic.h>
#include <matryoshka/vga.h>

// I/O port operations
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

// Timer state
static volatile uint64_t timer_ticks = 0;

/**
 * Timer interrupt handler (IRQ0)
 */
static void timer_irq_handler(interrupt_frame_t *frame) {
    (void)frame; // Unused
    
    // Increment tick counter
    timer_ticks++;
    
    // Send EOI to PIC
    pic_send_eoi(0);
}

/**
 * Initialize PIT
 */
void timer_init(void) {
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Initializing Timer (PIT)...\n");
    
    // Calculate divisor for desired frequency
    uint32_t divisor = PIT_FREQUENCY / TIMER_FREQUENCY;
    
    // Send command byte
    outb(PIT_COMMAND, PIT_SELECT_CHANNEL0 | PIT_ACCESS_LOHI | PIT_MODE_SQUARE | PIT_BINARY);
    
    // Send divisor (low byte, then high byte)
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
    
    // Register IRQ0 handler
    idt_register_handler(32, timer_irq_handler);
    
    // Enable IRQ0 (timer)
    pic_enable_irq(0);
    
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] PIT configured (");
    
    // Print frequency
    char buf[8];
    uint32_t freq = TIMER_FREQUENCY;
    int i = 0;
    if (freq >= 100) {
        buf[i++] = '0' + (freq / 100);
        freq %= 100;
    }
    if (i > 0 || freq >= 10) {
        buf[i++] = '0' + (freq / 10);
        freq %= 10;
    }
    buf[i++] = '0' + freq;
    buf[i++] = '\0';
    vga_puts(buf);
    
    vga_puts(" Hz)\n");
    vga_puts("  [OK] Timer IRQ0 handler registered\n");
    vga_puts("  [OK] System timer running\n\n");
}

/**
 * Get current tick count
 */
uint64_t timer_get_ticks(void) {
    return timer_ticks;
}

/**
 * Get uptime in milliseconds
 */
uint64_t timer_get_uptime_ms(void) {
    return (timer_ticks * 1000) / TIMER_FREQUENCY;
}

/**
 * Get uptime in seconds
 */
uint32_t timer_get_uptime_sec(void) {
    return (uint32_t)(timer_ticks / TIMER_FREQUENCY);
}

/**
 * Sleep for specified number of ticks
 */
void timer_sleep(uint32_t ticks) {
    uint64_t end_tick = timer_ticks + ticks;
    while (timer_ticks < end_tick) {
        __asm__ volatile("hlt");
    }
}

/**
 * Sleep for specified milliseconds
 */
void timer_sleep_ms(uint32_t ms) {
    uint32_t ticks = (ms * TIMER_FREQUENCY) / 1000;
    timer_sleep(ticks);
}

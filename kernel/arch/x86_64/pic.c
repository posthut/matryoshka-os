/**
 * MatryoshkaOS - Programmable Interrupt Controller (PIC)
 * 8259 PIC driver for IRQ management
 */

#include <matryoshka/pic.h>
#include <matryoshka/vga.h>

// I/O port operations
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Small delay for PIC operations
static inline void io_wait(void) {
    // Port 0x80 is used for POST codes, writing to it causes a delay
    outb(0x80, 0);
}

/**
 * Initialize PIC - remap IRQs
 */
void pic_init(void) {
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Initializing PIC...\n");
    
    // Save masks
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);
    
    // Start initialization sequence (ICW1)
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    
    // ICW2: Set interrupt vector offsets
    outb(PIC1_DATA, IRQ_OFFSET);        // Master PIC offset: 32
    io_wait();
    outb(PIC2_DATA, IRQ_OFFSET + 8);    // Slave PIC offset: 40
    io_wait();
    
    // ICW3: Tell Master PIC that Slave is at IRQ2
    outb(PIC1_DATA, 0x04);  // Slave on IRQ2
    io_wait();
    outb(PIC2_DATA, 0x02);  // Slave cascade identity
    io_wait();
    
    // ICW4: Set 8086 mode
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();
    
    // Restore saved masks
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
    
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] PIC remapped (IRQ 0-15 -> INT 32-47)\n");
    vga_puts("  [OK] Master PIC: IRQ 0-7\n");
    vga_puts("  [OK] Slave PIC: IRQ 8-15\n\n");
}

/**
 * Send End of Interrupt (EOI) signal
 */
void pic_send_eoi(uint8_t irq) {
    // If IRQ came from slave PIC, send EOI to slave
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    
    // Always send EOI to master PIC
    outb(PIC1_COMMAND, PIC_EOI);
}

/**
 * Enable specific IRQ
 */
void pic_enable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

/**
 * Disable specific IRQ
 */
void pic_disable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) | (1 << irq);
    outb(port, value);
}

/**
 * Get combined IRQ mask
 */
uint16_t pic_get_mask(void) {
    return (inb(PIC2_DATA) << 8) | inb(PIC1_DATA);
}

/**
 * Set combined IRQ mask
 */
void pic_set_mask(uint16_t mask) {
    outb(PIC1_DATA, mask & 0xFF);
    outb(PIC2_DATA, (mask >> 8) & 0xFF);
}

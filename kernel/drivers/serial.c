/**
 * MatryoshkaOS - Serial Port Driver
 * COM1 serial output for debugging
 */

#include <matryoshka/serial.h>
#include <matryoshka/io.h>

/**
 * Initialize serial port (COM1)
 */
int serial_init(void) {
    // Disable interrupts
    outb(COM1_PORT + 1, 0x00);
    
    // Enable DLAB (set baud rate divisor)
    outb(COM1_PORT + 3, 0x80);
    
    // Set divisor to 3 (lo byte) 38400 baud
    outb(COM1_PORT + 0, 0x03);
    outb(COM1_PORT + 1, 0x00);
    
    // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 3, 0x03);
    
    // Enable FIFO, clear with 14-byte threshold
    outb(COM1_PORT + 2, 0xC7);
    
    // IRQs enabled, RTS/DSR set
    outb(COM1_PORT + 4, 0x0B);
    
    // Test serial chip (loopback test)
    outb(COM1_PORT + 4, 0x1E);
    outb(COM1_PORT + 0, 0xAE);
    
    if (inb(COM1_PORT + 0) != 0xAE) {
        return -1;  // Serial chip failed
    }
    
    // Set normal operation mode
    outb(COM1_PORT + 4, 0x0F);
    
    return 0;
}

/**
 * Check if transmit buffer is empty
 */
static int serial_is_transmit_empty(void) {
    return inb(COM1_PORT + 5) & 0x20;
}

/**
 * Write a character to serial port
 */
void serial_putc(char c) {
    // Wait for transmit buffer to be empty
    while (serial_is_transmit_empty() == 0);
    
    outb(COM1_PORT, c);
}

/**
 * Write a string to serial port
 */
void serial_puts(const char *str) {
    if (!str) return;
    
    while (*str) {
        if (*str == '\n') {
            serial_putc('\r');  // Add carriage return for terminals
        }
        serial_putc(*str++);
    }
}

/**
 * Write a string with newline
 */
void serial_println(const char *str) {
    serial_puts(str);
    serial_putc('\r');
    serial_putc('\n');
}

void serial_put_hex32(uint32_t v) {
    const char hex[] = "0123456789ABCDEF";
    serial_puts("0x");
    for (int i = 28; i >= 0; i -= 4)
        serial_putc(hex[(v >> i) & 0xF]);
}

void klog(const char *msg) {
    serial_puts("[klog] ");
    serial_puts(msg);
    serial_putc('\r');
    serial_putc('\n');
}

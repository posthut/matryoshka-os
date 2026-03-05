/**
 * MatryoshkaOS - VGA Text Mode Driver Implementation
 * Full implementation of VGA text mode output
 */

#include <matryoshka/vga.h>
#include <matryoshka/io.h>

// VGA state
static uint16_t *vga_buffer = (uint16_t*)VGA_MEMORY;
static uint8_t vga_cursor_x = 0;
static uint8_t vga_cursor_y = 0;
static uint8_t vga_current_color = 0;

/**
 * Create VGA entry (character + color attributes)
 */
static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

/**
 * Create color byte from foreground and background
 */
static inline uint8_t vga_color(vga_color_t fg, vga_color_t bg) {
    return fg | (bg << 4);
}

/**
 * Initialize VGA driver
 */
void vga_init(void) {
    vga_cursor_x = 0;
    vga_cursor_y = 0;
    vga_current_color = vga_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_enable_cursor(true);
}

/**
 * Clear screen with specified color
 */
void vga_clear(vga_color_t fg, vga_color_t bg) {
    uint8_t color = vga_color(fg, bg);
    uint16_t blank = vga_entry(' ', color);
    
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = blank;
    }
    
    vga_cursor_x = 0;
    vga_cursor_y = 0;
    vga_update_cursor(0, 0);
}

/**
 * Set foreground and background colors
 */
void vga_set_color(vga_color_t fg, vga_color_t bg) {
    vga_current_color = vga_color(fg, bg);
}

/**
 * Scroll screen up by one line
 */
void vga_scroll(void) {
    uint16_t blank = vga_entry(' ', vga_current_color);
    
    // Move all lines up
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            size_t src = (y + 1) * VGA_WIDTH + x;
            size_t dst = y * VGA_WIDTH + x;
            vga_buffer[dst] = vga_buffer[src];
        }
    }
    
    // Clear last line
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        vga_buffer[index] = blank;
    }
}

/**
 * Write single character at current position
 */
void vga_putchar(char c) {
    // Handle special characters
    if (c == '\n') {
        vga_cursor_x = 0;
        vga_cursor_y++;
    } else if (c == '\r') {
        vga_cursor_x = 0;
    } else if (c == '\t') {
        vga_cursor_x = (vga_cursor_x + 8) & ~7;  // Align to 8
    } else if (c == '\b') {
        if (vga_cursor_x > 0) {
            vga_cursor_x--;
        }
    } else {
        // Regular character
        size_t index = vga_cursor_y * VGA_WIDTH + vga_cursor_x;
        vga_buffer[index] = vga_entry(c, vga_current_color);
        vga_cursor_x++;
    }
    
    // Handle line wrap
    if (vga_cursor_x >= VGA_WIDTH) {
        vga_cursor_x = 0;
        vga_cursor_y++;
    }
    
    // Handle screen scroll
    if (vga_cursor_y >= VGA_HEIGHT) {
        vga_scroll();
        vga_cursor_y = VGA_HEIGHT - 1;
    }
    
    vga_update_cursor(vga_cursor_x, vga_cursor_y);
}

/**
 * Write string at current position
 */
void vga_puts(const char *str) {
    if (!str) return;
    
    while (*str) {
        vga_putchar(*str);
        str++;
    }
}

/**
 * Write character at specific position
 */
void vga_putchar_at(char c, uint8_t x, uint8_t y, vga_color_t fg, vga_color_t bg) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) return;
    
    size_t index = y * VGA_WIDTH + x;
    uint8_t color = vga_color(fg, bg);
    vga_buffer[index] = vga_entry(c, color);
}

/**
 * Enable/disable cursor
 */
void vga_enable_cursor(bool enable) {
    if (enable) {
        outb(0x3D4, 0x0A);
        outb(0x3D5, (inb(0x3D5) & 0xC0) | 0);   // Cursor start line
        outb(0x3D4, 0x0B);
        outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);  // Cursor end line
    } else {
        outb(0x3D4, 0x0A);
        outb(0x3D5, 0x20);  // Disable cursor
    }
}

/**
 * Update cursor position
 */
void vga_update_cursor(uint8_t x, uint8_t y) {
    uint16_t pos = y * VGA_WIDTH + x;
    
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}


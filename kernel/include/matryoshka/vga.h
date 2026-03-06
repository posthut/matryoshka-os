/**
 * MatryoshkaOS - VGA Text Mode Driver
 * Simple text output to VGA display
 */

#ifndef MATRYOSHKA_VGA_H
#define MATRYOSHKA_VGA_H

#include <matryoshka/types.h>

// VGA text mode constants
#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

// VGA color codes
typedef enum {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW = 14,
    VGA_COLOR_WHITE = 15,
} vga_color_t;

/**
 * Initialize VGA driver
 */
void vga_init(void);

/**
 * Clear screen with specified color
 */
void vga_clear(vga_color_t fg, vga_color_t bg);

/**
 * Set foreground and background colors
 */
void vga_set_color(vga_color_t fg, vga_color_t bg);

/**
 * Write single character at current position
 */
void vga_putchar(char c);

/**
 * Write string at current position
 */
void vga_puts(const char *str);

/**
 * Write character at specific position
 */
void vga_putchar_at(char c, uint8_t x, uint8_t y, vga_color_t fg, vga_color_t bg);

/**
 * Scroll screen up by one line
 */
void vga_scroll(void);

/**
 * Enable/disable cursor
 */
void vga_enable_cursor(bool enable);

/**
 * Update cursor position
 */
void vga_update_cursor(uint8_t x, uint8_t y);

/**
 * Set cursor position (and move write position)
 */
void vga_set_cursor_pos(uint8_t x, uint8_t y);

#endif // MATRYOSHKA_VGA_H


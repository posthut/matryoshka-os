/**
 * MatryoshkaOS - Mock stubs for kernel subsystems.
 *
 * Linked into host-side unit tests so that kernel code calling
 * VGA / IO helpers resolves without pulling in real drivers.
 */

#include "compat.h"
#include <matryoshka/vga.h>

/* ── VGA stubs ────────────────────────────────────────────────────── */

void vga_init(void)                                      {}
void vga_clear(vga_color_t fg, vga_color_t bg)           { (void)fg; (void)bg; }
void vga_set_color(vga_color_t fg, vga_color_t bg)       { (void)fg; (void)bg; }
void vga_putchar(char c)                                  { (void)c; }
void vga_puts(const char *s)                              { (void)s; }
void vga_putchar_at(char c, uint8_t x, uint8_t y,
                    vga_color_t fg, vga_color_t bg)       { (void)c; (void)x; (void)y; (void)fg; (void)bg; }
void vga_scroll(void)                                     {}
void vga_enable_cursor(bool e)                            { (void)e; }
void vga_update_cursor(uint8_t x, uint8_t y)             { (void)x; (void)y; }
void vga_set_cursor_pos(uint8_t x, uint8_t y)            { (void)x; (void)y; }

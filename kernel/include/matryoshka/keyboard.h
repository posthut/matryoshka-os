/**
 * MatryoshkaOS - PS/2 Keyboard Driver
 * IRQ1 handler, scancode translation, input buffering
 */

#ifndef MATRYOSHKA_KEYBOARD_H
#define MATRYOSHKA_KEYBOARD_H

#include <matryoshka/types.h>

#define KB_DATA_PORT    0x60
#define KB_STATUS_PORT  0x64

#define KB_BUFFER_SIZE  256

/* Modifier flags (bitmask) */
#define KB_MOD_LSHIFT   0x01
#define KB_MOD_RSHIFT   0x02
#define KB_MOD_CTRL     0x04
#define KB_MOD_ALT      0x08
#define KB_MOD_CAPSLOCK 0x10

/**
 * Initialize PS/2 keyboard driver and register IRQ1 handler.
 * Must be called after idt_init() and pic_init().
 */
void keyboard_init(void);

/**
 * Blocking read — waits (via HLT) until a key is available.
 * Returns the ASCII character, or 0 for non-printable keys.
 */
char keyboard_getchar(void);

/**
 * Non-blocking check for buffered input.
 */
bool keyboard_has_input(void);

/**
 * Current modifier state (KB_MOD_* flags).
 */
uint8_t keyboard_get_modifiers(void);

/**
 * Set a custom wait function called while getchar() spins.
 * Pass NULL to revert to HLT (default).
 */
void keyboard_set_wait_func(void (*func)(void));

#endif /* MATRYOSHKA_KEYBOARD_H */

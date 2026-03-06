/**
 * MatryoshkaOS - Serial Port Driver
 * COM1 serial output for debugging and UEFI compatibility
 */

#ifndef MATRYOSHKA_SERIAL_H
#define MATRYOSHKA_SERIAL_H

#include <matryoshka/types.h>

// COM1 port
#define COM1_PORT 0x3F8

/**
 * Initialize serial port
 * @return 0 on success
 */
int serial_init(void);

/**
 * Write a character to serial port
 * @param c Character to write
 */
void serial_putc(char c);

/**
 * Write a string to serial port
 * @param str String to write
 */
void serial_puts(const char *str);

/**
 * Write a string with newline to serial port
 * @param str String to write
 */
void serial_println(const char *str);

#endif // MATRYOSHKA_SERIAL_H

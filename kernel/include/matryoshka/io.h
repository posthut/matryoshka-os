/**
 * MatryoshkaOS - x86_64 I/O Port Operations
 * Assembly inline functions for hardware I/O
 */

#ifndef MATRYOSHKA_IO_H
#define MATRYOSHKA_IO_H

#include <matryoshka/types.h>

/**
 * Read byte from I/O port
 */
static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/**
 * Write byte to I/O port
 */
static inline void outb(uint16_t port, uint8_t data) {
    __asm__ volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

/**
 * Read word (16-bit) from I/O port
 */
static inline uint16_t inw(uint16_t port) {
    uint16_t result;
    __asm__ volatile("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/**
 * Write word (16-bit) to I/O port
 */
static inline void outw(uint16_t port, uint16_t data) {
    __asm__ volatile("outw %0, %1" : : "a"(data), "Nd"(port));
}

/**
 * Read double word (32-bit) from I/O port
 */
static inline uint32_t inl(uint16_t port) {
    uint32_t result;
    __asm__ volatile("inl %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/**
 * Write double word (32-bit) to I/O port
 */
static inline void outl(uint16_t port, uint32_t data) {
    __asm__ volatile("outl %0, %1" : : "a"(data), "Nd"(port));
}

/**
 * I/O wait - small delay for slow devices
 */
static inline void io_wait(void) {
    outb(0x80, 0);  // Write to unused port 0x80
}

#endif // MATRYOSHKA_IO_H


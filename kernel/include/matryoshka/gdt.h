/**
 * MatryoshkaOS - Global Descriptor Table (GDT)
 * Segment descriptors for 32-bit protected mode
 */

#ifndef MATRYOSHKA_GDT_H
#define MATRYOSHKA_GDT_H

#include <matryoshka/types.h>

#define GDT_ENTRIES 3

#define GDT_KERNEL_CODE_SEL 0x08
#define GDT_KERNEL_DATA_SEL 0x10

/**
 * GDT entry structure (8 bytes)
 */
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed)) gdt_entry_t;

/**
 * GDT pointer structure (6 bytes)
 * Loaded into GDTR register via LGDT
 */
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

/**
 * Initialize GDT with flat 32-bit segments and reload segment registers.
 * Must be called before IDT/PIC/any interrupt setup.
 */
void gdt_init(void);

/**
 * Assembly routine: load GDTR and reload all segment registers.
 * Defined in entry.asm.
 */
extern void gdt_flush(uint32_t gdt_ptr_addr);

#endif // MATRYOSHKA_GDT_H

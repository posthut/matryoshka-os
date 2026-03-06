/**
 * MatryoshkaOS - Global Descriptor Table (GDT)
 * Segment descriptors for 32-bit protected mode with Ring 0/3 support.
 */

#ifndef MATRYOSHKA_GDT_H
#define MATRYOSHKA_GDT_H

#include <matryoshka/types.h>

#define GDT_ENTRIES 6

/* Segment selectors (GDT index * 8, ORed with RPL for user segments) */
#define GDT_KERNEL_CODE_SEL 0x08    /* Index 1, RPL 0 */
#define GDT_KERNEL_DATA_SEL 0x10    /* Index 2, RPL 0 */
#define GDT_USER_CODE_SEL   0x1B    /* Index 3, RPL 3 (0x18 | 3) */
#define GDT_USER_DATA_SEL   0x23    /* Index 4, RPL 3 (0x20 | 3) */
#define GDT_TSS_SEL          0x28    /* Index 5 */

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

/**
 * Task State Segment — the CPU reads ESP0/SS0 on ring 3→0 transitions.
 */
typedef struct {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1, ss1;
    uint32_t esp2, ss2;
    uint32_t cr3;
    uint32_t eip, eflags;
    uint32_t eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed)) tss_entry_t;

void gdt_init(void);

/**
 * Update TSS.ESP0 — called on every task switch so the CPU
 * uses the correct kernel stack for ring transitions.
 */
void tss_set_esp0(uint32_t esp0);

extern void gdt_flush(uint32_t gdt_ptr_addr);
extern void tss_flush(void);

#endif // MATRYOSHKA_GDT_H

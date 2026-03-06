/**
 * MatryoshkaOS - Global Descriptor Table (GDT)
 * Flat 32-bit segments for protected mode
 */

#include <matryoshka/gdt.h>
#include <matryoshka/vga.h>

static gdt_entry_t gdt[GDT_ENTRIES];
static gdt_ptr_t   gdt_ptr;

static void gdt_set_gate(int num, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t gran) {
    gdt[num].base_low    = base & 0xFFFF;
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    gdt[num].limit_low   = limit & 0xFFFF;
    gdt[num].granularity  = ((limit >> 16) & 0x0F) | (gran & 0xF0);

    gdt[num].access = access;
}

void gdt_init(void) {
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Initializing GDT...\n");

    gdt_ptr.limit = (sizeof(gdt_entry_t) * GDT_ENTRIES) - 1;
    gdt_ptr.base  = (uint32_t)&gdt;

    /* Entry 0 — null descriptor (required by CPU) */
    gdt_set_gate(0, 0, 0, 0, 0);

    /* Entry 1 (0x08) — kernel code: base 0, limit 4 GB, ring 0, exec+read */
    gdt_set_gate(1, 0, 0xFFFFF, 0x9A, 0xCF);

    /* Entry 2 (0x10) — kernel data: base 0, limit 4 GB, ring 0, read+write */
    gdt_set_gate(2, 0, 0xFFFFF, 0x92, 0xCF);

    gdt_flush((uint32_t)&gdt_ptr);

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] GDT loaded (3 entries: null, code 0x08, data 0x10)\n");
    vga_puts("  [OK] Segment registers reloaded\n\n");
}

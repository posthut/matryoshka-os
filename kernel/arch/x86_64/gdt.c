/**
 * MatryoshkaOS - Global Descriptor Table (GDT)
 * Flat 32-bit segments: ring 0 + ring 3 + TSS
 */

#include <matryoshka/gdt.h>
#include <matryoshka/vga.h>
#include <matryoshka/string.h>

static gdt_entry_t gdt[GDT_ENTRIES];
static gdt_ptr_t   gdt_ptr;
static tss_entry_t tss;

static void gdt_set_gate(int num, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t gran) {
    gdt[num].base_low    = base & 0xFFFF;
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;
    gdt[num].limit_low   = limit & 0xFFFF;
    gdt[num].granularity  = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access = access;
}

static void write_tss(int num) {
    uint32_t base  = (uint32_t)&tss;
    uint32_t limit = sizeof(tss_entry_t) - 1;

    memset(&tss, 0, sizeof(tss));
    tss.ss0  = GDT_KERNEL_DATA_SEL;
    tss.esp0 = 0;
    tss.iomap_base = sizeof(tss_entry_t);

    /* TSS descriptor: present, DPL 0, type 0x9 (available 32-bit TSS) */
    gdt_set_gate(num, base, limit, 0x89, 0x00);
}

void gdt_init(void) {
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Initializing GDT...\n");

    gdt_ptr.limit = (sizeof(gdt_entry_t) * GDT_ENTRIES) - 1;
    gdt_ptr.base  = (uint32_t)&gdt;

    /* 0 (0x00): null */
    gdt_set_gate(0, 0, 0, 0, 0);
    /* 1 (0x08): kernel code — DPL 0, exec+read */
    gdt_set_gate(1, 0, 0xFFFFF, 0x9A, 0xCF);
    /* 2 (0x10): kernel data — DPL 0, read+write */
    gdt_set_gate(2, 0, 0xFFFFF, 0x92, 0xCF);
    /* 3 (0x18): user code   — DPL 3, exec+read */
    gdt_set_gate(3, 0, 0xFFFFF, 0xFA, 0xCF);
    /* 4 (0x20): user data   — DPL 3, read+write */
    gdt_set_gate(4, 0, 0xFFFFF, 0xF2, 0xCF);
    /* 5 (0x28): TSS */
    write_tss(5);

    gdt_flush((uint32_t)&gdt_ptr);
    tss_flush();

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] GDT loaded (6 entries: null, kcode, kdata, ucode, udata, TSS)\n");
    vga_puts("  [OK] TSS loaded\n\n");
}

void tss_set_esp0(uint32_t esp0) {
    tss.esp0 = esp0;
}

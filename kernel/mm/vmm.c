/**
 * MatryoshkaOS - Virtual Memory Manager (VMM)
 * 32-bit two-level paging with identity mapping.
 *
 * On init we identity-map all physical RAM (virtual == physical) so
 * existing kernel code, heap, PMM bitmap, VGA buffer, etc. keep
 * working transparently.  Afterwards vmm_map / vmm_unmap can add
 * arbitrary mappings.
 */

#include <matryoshka/vmm.h>
#include <matryoshka/pmm.h>
#include <matryoshka/idt.h>
#include <matryoshka/vga.h>
#include <matryoshka/string.h>

/* ── Page Directory (must be 4KB-aligned) ─────────────────────────── */

static uint32_t page_directory[TABLES_PER_DIR]
    __attribute__((aligned(PAGE_SIZE)));

static vmm_stats_t stats;

/* ── Helpers ──────────────────────────────────────────────────────── */

static void print_hex32(uint32_t v) {
    const char hex[] = "0123456789ABCDEF";
    char buf[9];
    for (int i = 7; i >= 0; i--) {
        buf[i] = hex[v & 0xF];
        v >>= 4;
    }
    buf[8] = '\0';
    vga_puts(buf);
}

/* ── Page-fault handler (ISR 14) ──────────────────────────────────── */

static void page_fault_handler(interrupt_frame_t *frame) {
    uint32_t fault_addr;
    __asm__ volatile("mov %%cr2, %0" : "=r"(fault_addr));

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    vga_puts("\n\n!!! PAGE FAULT !!!\n\n");
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);

    vga_puts("Address: 0x");
    print_hex32(fault_addr);
    vga_putchar('\n');

    vga_puts("Error:   0x");
    print_hex32(frame->err_code);
    vga_puts("  [");
    if (!(frame->err_code & 1)) vga_puts("not-present ");
    if (frame->err_code & 2)    vga_puts("write ");
    if (frame->err_code & 4)    vga_puts("user ");
    if (frame->err_code & 8)    vga_puts("reserved-bit ");
    if (frame->err_code & 16)   vga_puts("instruction-fetch");
    vga_puts("]\n");

    vga_puts("EIP:     0x");
    print_hex32(frame->eip);
    vga_puts("\n\nSystem halted.\n");

    while (1) __asm__ volatile("cli; hlt");
}

/* ── Initialisation ───────────────────────────────────────────────── */

int vmm_init(void) {
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Initializing Virtual Memory Manager...\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    memset(page_directory, 0, sizeof(page_directory));
    memset(&stats, 0, sizeof(stats));

    /* Determine how much physical memory to identity-map. */
    pmm_stats_t pmm;
    pmm_get_stats(&pmm);
    uint32_t mem_bytes = (uint32_t)pmm.total_memory;
    if (pmm.total_memory > 0xFFFFFFFFULL)
        mem_bytes = 0xFFFFFFFF;

    /* Round up to 4MB boundary (each PDE covers 4MB). */
    uint32_t num_pde = (mem_bytes + 0x3FFFFF) >> 22;
    if (num_pde > TABLES_PER_DIR) num_pde = TABLES_PER_DIR;
    /* Minimum: at least first 4MB to cover kernel + heap */
    if (num_pde == 0) num_pde = 1;

    vga_puts("  Identity-mapping ");
    print_hex32(num_pde * 4);
    vga_puts(" MB (");
    print_hex32(num_pde);
    vga_puts(" page tables)\n");

    for (uint32_t i = 0; i < num_pde; i++) {
        uint64_t pt_phys = pmm_alloc_frame();
        if (pt_phys == 0) {
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_puts("  ERROR: PMM out of frames for page tables\n");
            return -1;
        }

        uint32_t *pt = (uint32_t *)(uint32_t)pt_phys;

        for (uint32_t j = 0; j < PAGES_PER_TABLE; j++) {
            uint32_t phys = (i << 22) | (j << 12);
            pt[j] = phys | PTE_PRESENT | PTE_WRITABLE;
        }

        page_directory[i] = (uint32_t)pt_phys | PTE_PRESENT | PTE_WRITABLE;
        stats.page_tables++;
    }

    stats.mapped_pages  = num_pde * PAGES_PER_TABLE;
    stats.identity_end  = num_pde << 22;  /* first unmapped address */

    /* Register page-fault handler BEFORE enabling paging. */
    idt_register_handler(14, page_fault_handler);

    /* Load page directory into CR3. */
    __asm__ volatile("mov %0, %%cr3" : : "r"((uint32_t)page_directory));

    /* Enable paging: set CR0.PG (bit 31). */
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] Paging enabled (CR0.PG set)\n");
    vga_puts("  [OK] Page directory at 0x");
    print_hex32((uint32_t)page_directory);
    vga_puts("\n");
    vga_puts("  [OK] ");
    print_hex32(stats.mapped_pages);
    vga_puts(" pages mapped, ");
    print_hex32(stats.page_tables);
    vga_puts(" page tables\n");
    vga_puts("  [OK] Page-fault handler registered (ISR 14)\n\n");

    return 0;
}

/* ── Map / Unmap ──────────────────────────────────────────────────── */

int vmm_map(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    uint32_t pd_idx = VMM_PD_INDEX(virtual_addr);
    uint32_t pt_idx = VMM_PT_INDEX(virtual_addr);

    if (!(page_directory[pd_idx] & PTE_PRESENT)) {
        uint64_t pt_phys = pmm_alloc_frame();
        if (pt_phys == 0) return -1;

        memset((void *)(uint32_t)pt_phys, 0, PAGE_SIZE);
        page_directory[pd_idx] = (uint32_t)pt_phys
                                 | PTE_PRESENT | PTE_WRITABLE;
        stats.page_tables++;
    }

    /* Propagate PTE_USER to the PDE so hardware allows ring 3 access */
    if (flags & PTE_USER)
        page_directory[pd_idx] |= PTE_USER;

    uint32_t *pt = (uint32_t *)(page_directory[pd_idx] & PTE_ADDR_MASK);
    pt[pt_idx] = (physical_addr & PTE_ADDR_MASK) | (flags & 0xFFF);
    stats.mapped_pages++;

    vmm_flush_tlb(virtual_addr);
    return 0;
}

int vmm_unmap(uint32_t virtual_addr) {
    uint32_t pd_idx = VMM_PD_INDEX(virtual_addr);
    uint32_t pt_idx = VMM_PT_INDEX(virtual_addr);

    if (!(page_directory[pd_idx] & PTE_PRESENT))
        return -1;

    uint32_t *pt = (uint32_t *)(page_directory[pd_idx] & PTE_ADDR_MASK);
    if (!(pt[pt_idx] & PTE_PRESENT))
        return -1;

    pt[pt_idx] = 0;
    if (stats.mapped_pages > 0)
        stats.mapped_pages--;

    vmm_flush_tlb(virtual_addr);
    return 0;
}

/* ── Query ────────────────────────────────────────────────────────── */

uint32_t vmm_get_physical(uint32_t virtual_addr) {
    uint32_t pd_idx = VMM_PD_INDEX(virtual_addr);
    uint32_t pt_idx = VMM_PT_INDEX(virtual_addr);

    if (!(page_directory[pd_idx] & PTE_PRESENT))
        return 0;

    uint32_t *pt = (uint32_t *)(page_directory[pd_idx] & PTE_ADDR_MASK);
    if (!(pt[pt_idx] & PTE_PRESENT))
        return 0;

    return (pt[pt_idx] & PTE_ADDR_MASK) | VMM_OFFSET(virtual_addr);
}

bool vmm_is_mapped(uint32_t virtual_addr) {
    uint32_t pd_idx = VMM_PD_INDEX(virtual_addr);
    uint32_t pt_idx = VMM_PT_INDEX(virtual_addr);

    if (!(page_directory[pd_idx] & PTE_PRESENT))
        return false;

    uint32_t *pt = (uint32_t *)(page_directory[pd_idx] & PTE_ADDR_MASK);
    return (pt[pt_idx] & PTE_PRESENT) != 0;
}

void vmm_get_stats(vmm_stats_t *out) {
    if (out) *out = stats;
}

/* ── TLB flush ────────────────────────────────────────────────────── */

int vmm_set_user(uint32_t virtual_addr) {
    uint32_t pd_idx = VMM_PD_INDEX(virtual_addr);
    uint32_t pt_idx = VMM_PT_INDEX(virtual_addr);

    if (!(page_directory[pd_idx] & PTE_PRESENT))
        return -1;

    page_directory[pd_idx] |= PTE_USER;

    uint32_t *pt = (uint32_t *)(page_directory[pd_idx] & PTE_ADDR_MASK);
    if (!(pt[pt_idx] & PTE_PRESENT))
        return -1;

    pt[pt_idx] |= PTE_USER;
    vmm_flush_tlb(virtual_addr);
    return 0;
}

void vmm_flush_tlb(uint32_t virtual_addr) {
    __asm__ volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
}

void vmm_flush_tlb_all(void) {
    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
}

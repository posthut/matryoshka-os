/**
 * MatryoshkaOS - Virtual Memory Manager (VMM)
 * 32-bit two-level paging: Page Directory → Page Table → 4KB Page
 *
 * Virtual address layout (32-bit):
 *   [31:22] Page Directory index (10 bits, 1024 entries)
 *   [21:12] Page Table index    (10 bits, 1024 entries)
 *   [11: 0] Offset within page  (12 bits, 4096 bytes)
 */

#ifndef MATRYOSHKA_VMM_H
#define MATRYOSHKA_VMM_H

#include <matryoshka/types.h>

#define PAGE_SIZE           4096
#define PAGE_SHIFT          12
#define PAGES_PER_TABLE     1024
#define TABLES_PER_DIR      1024

/* Page Directory / Page Table entry flags (identical bit layout) */
#define PTE_PRESENT         (1u << 0)
#define PTE_WRITABLE        (1u << 1)
#define PTE_USER            (1u << 2)
#define PTE_WRITE_THROUGH   (1u << 3)
#define PTE_CACHE_DISABLE   (1u << 4)
#define PTE_ACCESSED        (1u << 5)
#define PTE_DIRTY           (1u << 6)
#define PTE_PAGE_SIZE       (1u << 7)   /* 4MB page (PDE only) */
#define PTE_GLOBAL          (1u << 8)

/* Mask to extract 4KB-aligned physical address from entry */
#define PTE_ADDR_MASK       0xFFFFF000u

/* Extract indices from a 32-bit virtual address */
#define VMM_PD_INDEX(va)    (((va) >> 22) & 0x3FFu)
#define VMM_PT_INDEX(va)    (((va) >> 12) & 0x3FFu)
#define VMM_OFFSET(va)      ((va) & 0xFFFu)

/* Alignment helpers */
#define PAGE_ALIGN_DOWN(a)  ((a) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_UP(a)    (((a) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define IS_PAGE_ALIGNED(a)  (((a) & (PAGE_SIZE - 1)) == 0)

typedef struct {
    uint32_t mapped_pages;
    uint32_t page_tables;
    uint32_t identity_end;     /* highest identity-mapped address */
} vmm_stats_t;

int      vmm_init(void);
int      vmm_map(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
int      vmm_unmap(uint32_t virtual_addr);
uint32_t vmm_get_physical(uint32_t virtual_addr);
bool     vmm_is_mapped(uint32_t virtual_addr);
void     vmm_get_stats(vmm_stats_t *stats);

void     vmm_flush_tlb(uint32_t virtual_addr);
void     vmm_flush_tlb_all(void);

#endif /* MATRYOSHKA_VMM_H */

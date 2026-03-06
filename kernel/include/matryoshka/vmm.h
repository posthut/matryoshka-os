/**
 * MatryoshkaOS - Virtual Memory Manager (VMM)
 * 4-level paging for x86_64 architecture
 * 
 * Page table hierarchy:
 * PML4 (Level 4) -> PDPT (Level 3) -> PD (Level 2) -> PT (Level 1) -> Physical Page
 */

#ifndef MATRYOSHKA_VMM_H
#define MATRYOSHKA_VMM_H

#include <matryoshka/types.h>

// Page size constants
#define PAGE_SIZE           4096        // 4KB pages
#define PAGE_SHIFT          12          // log2(PAGE_SIZE)
#define ENTRIES_PER_TABLE   512         // 512 entries per table

// Page table entry flags
#define PTE_PRESENT         (1 << 0)    // Page is present in memory
#define PTE_WRITABLE        (1 << 1)    // Page is writable
#define PTE_USER            (1 << 2)    // Page is user-accessible
#define PTE_WRITE_THROUGH   (1 << 3)    // Write-through caching
#define PTE_CACHE_DISABLE   (1 << 4)    // Disable caching
#define PTE_ACCESSED        (1 << 5)    // Page was accessed
#define PTE_DIRTY           (1 << 6)    // Page was written to
#define PTE_HUGE            (1 << 7)    // Huge page (2MB or 1GB)
#define PTE_GLOBAL          (1 << 8)    // Global page (not flushed on CR3 change)
#define PTE_NO_EXECUTE      (1ULL << 63) // No execute bit

// Virtual address layout
#define KERNEL_VIRTUAL_BASE 0xFFFFFFFF80000000ULL  // Higher half kernel (-2GB)
#define KERNEL_HEAP_START   0xFFFFFFFFC0000000ULL  // Kernel heap start
#define USER_SPACE_END      0x0000800000000000ULL  // User space ends at 128TB

// Page table entry type
typedef uint64_t pte_t;

// Page table structure (512 entries)
typedef struct page_table {
    pte_t entries[ENTRIES_PER_TABLE];
} __attribute__((aligned(PAGE_SIZE))) page_table_t;

// VMM statistics
typedef struct vmm_stats {
    uint64_t total_virtual_pages;      // Total virtual pages available
    uint64_t mapped_pages;             // Currently mapped pages
    uint64_t kernel_pages;             // Pages used by kernel
    uint64_t user_pages;               // Pages used by user space
} vmm_stats_t;

/**
 * Initialize Virtual Memory Manager
 * Sets up page tables and enables paging
 * 
 * @return 0 on success, -1 on failure
 */
int vmm_init(void);

/**
 * Map a virtual address to a physical address
 * 
 * @param virtual_addr Virtual address to map
 * @param physical_addr Physical address to map to
 * @param flags Page flags (PTE_PRESENT, PTE_WRITABLE, etc.)
 * @return 0 on success, -1 on failure
 */
int vmm_map(uint64_t virtual_addr, uint64_t physical_addr, uint32_t flags);

/**
 * Unmap a virtual address
 * 
 * @param virtual_addr Virtual address to unmap
 * @return 0 on success, -1 on failure
 */
int vmm_unmap(uint64_t virtual_addr);

/**
 * Get physical address for a given virtual address
 * 
 * @param virtual_addr Virtual address to translate
 * @return Physical address, or 0 if not mapped
 */
uint64_t vmm_get_physical(uint64_t virtual_addr);

/**
 * Check if a virtual address is mapped
 * 
 * @param virtual_addr Virtual address to check
 * @return true if mapped, false otherwise
 */
bool vmm_is_mapped(uint64_t virtual_addr);

/**
 * Get VMM statistics
 * 
 * @param stats Pointer to stats structure to fill
 */
void vmm_get_stats(vmm_stats_t *stats);

/**
 * Flush TLB for a specific virtual address
 * 
 * @param virtual_addr Virtual address to flush
 */
void vmm_flush_tlb(uint64_t virtual_addr);

/**
 * Flush entire TLB
 */
void vmm_flush_tlb_all(void);

// Helper macros for address manipulation
#define PAGE_ALIGN_DOWN(addr)   ((addr) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_UP(addr)     (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define IS_PAGE_ALIGNED(addr)   (((addr) & (PAGE_SIZE - 1)) == 0)

// Extract page table indices from virtual address
#define PML4_INDEX(addr)        (((addr) >> 39) & 0x1FF)
#define PDPT_INDEX(addr)        (((addr) >> 30) & 0x1FF)
#define PD_INDEX(addr)          (((addr) >> 21) & 0x1FF)
#define PT_INDEX(addr)          (((addr) >> 12) & 0x1FF)
#define PAGE_OFFSET(addr)       ((addr) & 0xFFF)

// Extract physical address from PTE
#define PTE_TO_PHYS(pte)        ((pte) & 0x000FFFFFFFFFF000ULL)

#endif // MATRYOSHKA_VMM_H

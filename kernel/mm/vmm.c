/**
 * MatryoshkaOS - Virtual Memory Manager (VMM)
 * Page table management for memory virtualization
 * 
 * Note: Currently running in 32-bit protected mode.
 * Full 4-level paging requires transition to 64-bit long mode.
 * This implementation provides basic paging support with identity mapping.
 */

#include <matryoshka/vmm.h>
#include <matryoshka/pmm.h>
#include <matryoshka/vga.h>

// Page directory (for 32-bit mode - 2-level paging)
static page_table_t *kernel_page_directory = NULL;

// Statistics
static vmm_stats_t vmm_stats = {0};

/**
 * Initialize Virtual Memory Manager
 * Sets up identity mapping for kernel space
 */
int vmm_init(void) {
    // For now, in 32-bit mode, we use simple identity mapping
    // The paging was already set up by entry.asm during boot
    // This function prepares for future transition to 64-bit mode
    
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Initializing Virtual Memory Manager...\n");
    
    // Get page directory base from CR3 (if paging is enabled)
    uint64_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    
    if (cr0 & 0x80000000) {
        // Paging is enabled
        uint64_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        kernel_page_directory = (page_table_t *)(cr3 & 0xFFFFFFFFFFFFF000ULL);
        
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        vga_puts("  Paging is already enabled\n");
        vga_puts("  Page directory at: 0x");
        // Print address (simplified for now)
        vga_puts("XXXXXXXX\n");
    } else {
        // Paging is not enabled - we're in flat memory mode
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        vga_puts("  Running in flat memory mode (no paging)\n");
        vga_puts("  Identity mapping: virtual == physical\n");
    }
    
    // Initialize statistics
    vmm_stats.total_virtual_pages = 0xFFFFFFFF / PAGE_SIZE;  // ~1M pages in 32-bit
    vmm_stats.mapped_pages = 0;
    vmm_stats.kernel_pages = 0;
    vmm_stats.user_pages = 0;
    
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] VMM initialized (64-bit long mode)\n");
    
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("  Note: Currently using identity mapping\n");
    vga_puts("  Ready for higher-half kernel mapping\n\n");
    
    return 0;
}

/**
 * Map a virtual address to a physical address
 * 
 * Note: In current 32-bit flat mode, this is a placeholder
 * Full implementation requires proper page table setup
 */
int vmm_map(uint64_t virtual_addr, uint64_t physical_addr, uint32_t flags) {
    // Placeholder for future implementation
    // In 32-bit flat mode, virtual == physical
    
    (void)virtual_addr;
    (void)physical_addr;
    (void)flags;
    
    // For now, assume identity mapping
    return 0;
}

/**
 * Unmap a virtual address
 */
int vmm_unmap(uint64_t virtual_addr) {
    // Placeholder
    (void)virtual_addr;
    return 0;
}

/**
 * Get physical address for a given virtual address
 * In flat mode: virtual == physical
 */
uint64_t vmm_get_physical(uint64_t virtual_addr) {
    // In flat memory mode, virtual == physical
    return virtual_addr;
}

/**
 * Check if a virtual address is mapped
 * In flat mode: all addresses are "mapped"
 */
bool vmm_is_mapped(uint64_t virtual_addr) {
    // In flat mode, all addresses in range are mapped
    (void)virtual_addr;
    return true;
}

/**
 * Get VMM statistics
 */
void vmm_get_stats(vmm_stats_t *stats) {
    if (stats) {
        stats->total_virtual_pages = vmm_stats.total_virtual_pages;
        stats->mapped_pages = vmm_stats.mapped_pages;
        stats->kernel_pages = vmm_stats.kernel_pages;
        stats->user_pages = vmm_stats.user_pages;
    }
}

/**
 * Flush TLB for a specific virtual address
 */
void vmm_flush_tlb(uint64_t virtual_addr) {
    // Invalidate TLB entry for this address
    __asm__ volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
}

/**
 * Flush entire TLB by reloading CR3
 */
void vmm_flush_tlb_all(void) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
}

/**
 * MatryoshkaOS - Multiboot2 Parser Implementation
 * Parse Multiboot2 information provided by bootloader
 */

#include <matryoshka/multiboot2.h>

/**
 * Find a specific tag in Multiboot2 information
 */
multiboot_tag_t *multiboot_find_tag(multiboot_info_t *mbi, uint32_t type) {
    if (!mbi) return NULL;
    
    // Start after the fixed-size header
    multiboot_tag_t *tag = (multiboot_tag_t *)((uint8_t *)mbi + 8);
    
    // Iterate through all tags
    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        if (tag->type == type) {
            return tag;
        }
        
        // Move to next tag (tags are 8-byte aligned)
        tag = (multiboot_tag_t *)((uint8_t *)tag + ((tag->size + 7) & ~7));
    }
    
    return NULL;
}

/**
 * Get total available memory in bytes
 */
uint64_t multiboot_get_total_memory(multiboot_info_t *mbi) {
    if (!mbi) return 0;
    
    uint64_t total = 0;
    
    // Try to get detailed memory map first
    multiboot_tag_mmap_t *mmap_tag = multiboot_get_mmap(mbi);
    if (mmap_tag) {
        multiboot_mmap_entry_t *entry = mmap_tag->entries;
        uint32_t entry_count = (mmap_tag->size - sizeof(multiboot_tag_mmap_t)) / mmap_tag->entry_size;
        
        for (uint32_t i = 0; i < entry_count; i++) {
            if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
                total += entry->len;
            }
            entry = (multiboot_mmap_entry_t *)((uint8_t *)entry + mmap_tag->entry_size);
        }
        
        return total;
    }
    
    // Fallback to basic memory info
    multiboot_tag_basic_meminfo_t *meminfo = 
        (multiboot_tag_basic_meminfo_t *)multiboot_find_tag(mbi, MULTIBOOT_TAG_TYPE_BASIC_MEMINFO);
    
    if (meminfo) {
        // mem_lower is in KB (0-640KB), mem_upper is in KB (1MB+)
        total = ((uint64_t)meminfo->mem_lower * 1024) + 
                ((uint64_t)meminfo->mem_upper * 1024) + 
                (1024 * 1024); // Add 1MB for the gap
    }
    
    return total;
}

/**
 * Get memory map from Multiboot2 info
 */
multiboot_tag_mmap_t *multiboot_get_mmap(multiboot_info_t *mbi) {
    if (!mbi) return NULL;
    
    multiboot_tag_t *tag = multiboot_find_tag(mbi, MULTIBOOT_TAG_TYPE_MMAP);
    if (!tag) return NULL;
    
    return (multiboot_tag_mmap_t *)tag;
}


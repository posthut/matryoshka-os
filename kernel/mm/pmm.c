/**
 * MatryoshkaOS - Physical Memory Manager Implementation
 * Bitmap-based frame allocator for physical memory
 */

#include <matryoshka/pmm.h>
#include <matryoshka/vga.h>

// Bitmap for tracking frame allocation
static uint32_t *frame_bitmap = NULL;
static uint64_t total_frames = 0;
static uint64_t used_frames = 0;

// Memory information
static uint64_t total_memory = 0;
static uint64_t highest_address = 0;

// Bitmap location (placed after kernel)
extern char _kernel_end[];  // Defined in linker script
static uint8_t *bitmap_location = (uint8_t*)_kernel_end;

/**
 * Set a bit in the bitmap (mark frame as used)
 */
static inline void bitmap_set(uint64_t frame) {
    uint64_t index = frame / 32;
    uint32_t bit = frame % 32;
    frame_bitmap[index] |= (1 << bit);
}

/**
 * Clear a bit in the bitmap (mark frame as free)
 */
static inline void bitmap_clear(uint64_t frame) {
    uint64_t index = frame / 32;
    uint32_t bit = frame % 32;
    frame_bitmap[index] &= ~(1 << bit);
}

/**
 * Test a bit in the bitmap
 */
static inline bool bitmap_test(uint64_t frame) {
    uint64_t index = frame / 32;
    uint32_t bit = frame % 32;
    return (frame_bitmap[index] & (1 << bit)) != 0;
}

/**
 * Find first free frame in bitmap
 */
static uint64_t find_free_frame(void) {
    for (uint64_t i = 0; i < total_frames / 32; i++) {
        if (frame_bitmap[i] != 0xFFFFFFFF) {
            // This uint32 has at least one free bit
            for (uint32_t j = 0; j < 32; j++) {
                uint64_t frame = i * 32 + j;
                if (frame < total_frames && !bitmap_test(frame)) {
                    return frame;
                }
            }
        }
    }
    return (uint64_t)-1;  // No free frames
}

/**
 * Initialize Physical Memory Manager
 */
void pmm_init(multiboot_info_t *mbi) {
    if (!mbi) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("ERROR: No Multiboot info\n");
        return;
    }
    
    // Get memory map
    multiboot_tag_mmap_t *mmap_tag = multiboot_get_mmap(mbi);
    if (!mmap_tag) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("ERROR: No memory map\n");
        return;
    }
    
    // Calculate total memory and highest address
    multiboot_mmap_entry_t *entry = mmap_tag->entries;
    uint32_t entry_count = (mmap_tag->size - sizeof(multiboot_tag_mmap_t)) / mmap_tag->entry_size;
    
    for (uint32_t i = 0; i < entry_count; i++) {
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            total_memory += entry->len;
            uint64_t end_addr = entry->addr + entry->len;
            if (end_addr > highest_address) {
                highest_address = end_addr;
            }
        }
        entry = (multiboot_mmap_entry_t *)((uint8_t *)entry + mmap_tag->entry_size);
    }
    
    // Calculate number of frames
    total_frames = highest_address / PMM_FRAME_SIZE;
    if (highest_address % PMM_FRAME_SIZE != 0) {
        total_frames++;
    }
    
    // Calculate bitmap size (1 bit per frame)
    uint64_t bitmap_size = total_frames / 8;
    if (total_frames % 8 != 0) {
        bitmap_size++;
    }
    
    // Align bitmap size to 32-bit boundaries
    if (bitmap_size % 4 != 0) {
        bitmap_size += 4 - (bitmap_size % 4);
    }
    
    // Place bitmap after kernel
    frame_bitmap = (uint32_t *)bitmap_location;
    
    // Initialize all frames as used
    for (uint64_t i = 0; i < bitmap_size / 4; i++) {
        frame_bitmap[i] = 0xFFFFFFFF;
    }
    
    used_frames = total_frames;
    
    // Mark available regions as free
    entry = mmap_tag->entries;
    for (uint32_t i = 0; i < entry_count; i++) {
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            uint64_t start_frame = entry->addr / PMM_FRAME_SIZE;
            uint64_t end_frame = (entry->addr + entry->len) / PMM_FRAME_SIZE;
            
            for (uint64_t frame = start_frame; frame < end_frame && frame < total_frames; frame++) {
                bitmap_clear(frame);
                used_frames--;
            }
        }
        entry = (multiboot_mmap_entry_t *)((uint8_t *)entry + mmap_tag->entry_size);
    }
    
    // Reserve kernel and bitmap area
    uint64_t bitmap_end = (uint64_t)bitmap_location + bitmap_size;
    pmm_reserve_region(0, bitmap_end);
}

/**
 * Allocate a physical frame
 */
uint64_t pmm_alloc_frame(void) {
    uint64_t frame = find_free_frame();
    
    if (frame == (uint64_t)-1) {
        return 0;  // Out of memory
    }
    
    bitmap_set(frame);
    used_frames++;
    
    return PMM_FRAME_TO_ADDR(frame);
}

/**
 * Allocate multiple contiguous frames
 */
uint64_t pmm_alloc_frames(size_t count) {
    if (count == 0) return 0;
    
    // Find contiguous free frames
    uint64_t start_frame = 0;
    uint64_t found_count = 0;
    
    for (uint64_t frame = 0; frame < total_frames; frame++) {
        if (!bitmap_test(frame)) {
            if (found_count == 0) {
                start_frame = frame;
            }
            found_count++;
            
            if (found_count == count) {
                // Found enough contiguous frames
                for (uint64_t i = 0; i < count; i++) {
                    bitmap_set(start_frame + i);
                    used_frames++;
                }
                return PMM_FRAME_TO_ADDR(start_frame);
            }
        } else {
            found_count = 0;
        }
    }
    
    return 0;  // Not enough contiguous frames
}

/**
 * Free a physical frame
 */
void pmm_free_frame(uint64_t addr) {
    uint64_t frame = PMM_ADDR_TO_FRAME(addr);
    
    if (frame >= total_frames) {
        return;  // Invalid frame
    }
    
    if (!bitmap_test(frame)) {
        return;  // Already free (double-free protection)
    }
    
    bitmap_clear(frame);
    used_frames--;
}

/**
 * Free multiple contiguous frames
 */
void pmm_free_frames(uint64_t addr, size_t count) {
    for (size_t i = 0; i < count; i++) {
        pmm_free_frame(addr + (i * PMM_FRAME_SIZE));
    }
}

/**
 * Check if a frame is allocated
 */
bool pmm_is_frame_allocated(uint64_t addr) {
    uint64_t frame = PMM_ADDR_TO_FRAME(addr);
    
    if (frame >= total_frames) {
        return true;  // Invalid frames are considered "allocated"
    }
    
    return bitmap_test(frame);
}

/**
 * Get PMM statistics
 */
void pmm_get_stats(pmm_stats_t *stats) {
    if (!stats) return;
    
    stats->total_frames = total_frames;
    stats->used_frames = used_frames;
    stats->free_frames = total_frames - used_frames;
    stats->total_memory = total_memory;
    stats->used_memory = used_frames * PMM_FRAME_SIZE;
    stats->free_memory = (total_frames - used_frames) * PMM_FRAME_SIZE;
}

/**
 * Reserve a physical frame
 */
void pmm_reserve_frame(uint64_t addr) {
    uint64_t frame = PMM_ADDR_TO_FRAME(addr);
    
    if (frame >= total_frames) {
        return;
    }
    
    if (!bitmap_test(frame)) {
        bitmap_set(frame);
        used_frames++;
    }
}

/**
 * Reserve a region of physical memory
 */
void pmm_reserve_region(uint64_t start, uint64_t end) {
    uint64_t start_frame = PMM_ADDR_TO_FRAME(start);
    uint64_t end_frame = PMM_ADDR_TO_FRAME(end);
    
    if (end % PMM_FRAME_SIZE != 0) {
        end_frame++;
    }
    
    for (uint64_t frame = start_frame; frame <= end_frame && frame < total_frames; frame++) {
        pmm_reserve_frame(PMM_FRAME_TO_ADDR(frame));
    }
}


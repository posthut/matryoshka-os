/**
 * MatryoshkaOS - Physical Memory Manager
 * Bitmap-based physical frame allocator
 */

#ifndef MATRYOSHKA_PMM_H
#define MATRYOSHKA_PMM_H

#include <matryoshka/types.h>
#include <matryoshka/multiboot2.h>

// Frame size (4KB pages)
#define PMM_FRAME_SIZE 4096

// Convert address to frame number
#define PMM_ADDR_TO_FRAME(addr) ((addr) / PMM_FRAME_SIZE)

// Convert frame number to address
#define PMM_FRAME_TO_ADDR(frame) ((frame) * PMM_FRAME_SIZE)

/**
 * PMM statistics
 */
typedef struct pmm_stats {
    uint64_t total_frames;      // Total number of frames
    uint64_t used_frames;       // Currently used frames
    uint64_t free_frames;       // Currently free frames
    uint64_t total_memory;      // Total memory in bytes
    uint64_t used_memory;       // Used memory in bytes
    uint64_t free_memory;       // Free memory in bytes
} pmm_stats_t;

/**
 * Initialize Physical Memory Manager
 * @param mbi Pointer to Multiboot2 info structure
 */
void pmm_init(multiboot_info_t *mbi);

/**
 * Allocate a physical frame
 * @return Physical address of allocated frame, or 0 if out of memory
 */
uint64_t pmm_alloc_frame(void);

/**
 * Allocate multiple contiguous physical frames
 * @param count Number of frames to allocate
 * @return Physical address of first frame, or 0 if not enough memory
 */
uint64_t pmm_alloc_frames(size_t count);

/**
 * Free a physical frame
 * @param addr Physical address of frame to free
 */
void pmm_free_frame(uint64_t addr);

/**
 * Free multiple contiguous physical frames
 * @param addr Physical address of first frame
 * @param count Number of frames to free
 */
void pmm_free_frames(uint64_t addr, size_t count);

/**
 * Check if a frame is allocated
 * @param addr Physical address to check
 * @return true if frame is allocated, false if free
 */
bool pmm_is_frame_allocated(uint64_t addr);

/**
 * Get PMM statistics
 * @param stats Pointer to stats structure to fill
 */
void pmm_get_stats(pmm_stats_t *stats);

/**
 * Reserve a physical frame (mark as used without allocation)
 * Used for kernel code, data, and other reserved areas
 * @param addr Physical address to reserve
 */
void pmm_reserve_frame(uint64_t addr);

/**
 * Reserve a region of physical memory
 * @param start Start physical address
 * @param end End physical address
 */
void pmm_reserve_region(uint64_t start, uint64_t end);

#endif // MATRYOSHKA_PMM_H


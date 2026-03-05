/**
 * MatryoshkaOS - Kernel Heap Allocator
 * Simple linked-list based memory allocator
 */

#include <matryoshka/heap.h>
#include <matryoshka/pmm.h>
#include <matryoshka/vga.h>

// Block header structure
typedef struct heap_block {
    uint32_t magic;              // Magic number for validation
    size_t size;                 // Size of block (excluding header)
    bool is_free;                // Is block free?
    struct heap_block *next;     // Next block in list
} heap_block_t;

// Heap state
static heap_block_t *heap_start = NULL;
static size_t heap_total = 0;
static size_t heap_used = 0;

/**
 * Initialize kernel heap
 */
int heap_init(void) {
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Initializing Kernel Heap...\n");
    
    // Allocate initial heap memory from PMM
    // For simplicity, use static address for now
    heap_start = (heap_block_t *)HEAP_START;
    heap_total = HEAP_INITIAL;
    heap_used = sizeof(heap_block_t);
    
    // Initialize first block
    heap_start->magic = HEAP_MAGIC;
    heap_start->size = HEAP_INITIAL - sizeof(heap_block_t);
    heap_start->is_free = true;
    heap_start->next = NULL;
    
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("  Heap Start: 0x00200000\n");
    vga_puts("  Initial Size: 1 MB\n");
    
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] Heap initialized\n\n");
    
    return 0;
}

/**
 * Find free block that fits size
 */
static heap_block_t *find_free_block(size_t size) {
    heap_block_t *current = heap_start;
    
    while (current) {
        // Check magic number for corruption detection
        if (current->magic != HEAP_MAGIC) {
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_puts("ERROR: Heap corruption detected!\n");
            return NULL;
        }
        
        if (current->is_free && current->size >= size) {
            return current;
        }
        
        current = current->next;
    }
    
    return NULL;
}

/**
 * Split block if it's too large
 */
static void split_block(heap_block_t *block, size_t size) {
    // Only split if remainder is large enough for header + some data
    if (block->size >= size + sizeof(heap_block_t) + 16) {
        heap_block_t *new_block = (heap_block_t *)((uint8_t *)block + sizeof(heap_block_t) + size);
        new_block->magic = HEAP_MAGIC;
        new_block->size = block->size - size - sizeof(heap_block_t);
        new_block->is_free = true;
        new_block->next = block->next;
        
        block->size = size;
        block->next = new_block;
    }
}

/**
 * Merge adjacent free blocks
 */
static void merge_free_blocks(void) {
    heap_block_t *current = heap_start;
    
    while (current && current->next) {
        if (current->is_free && current->next->is_free) {
            // Merge current with next
            current->size += sizeof(heap_block_t) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

/**
 * Allocate memory from kernel heap
 */
void *kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    // Align size to 8 bytes
    size = (size + 7) & ~7;
    
    // Find free block
    heap_block_t *block = find_free_block(size);
    
    if (!block) {
        // Out of memory
        return NULL;
    }
    
    // Split block if too large
    split_block(block, size);
    
    // Mark as used
    block->is_free = false;
    heap_used += sizeof(heap_block_t) + block->size;
    
    // Return pointer after header
    return (void *)((uint8_t *)block + sizeof(heap_block_t));
}

/**
 * Allocate and zero memory
 */
void *kzalloc(size_t size) {
    void *ptr = kmalloc(size);
    
    if (ptr) {
        // Zero out memory
        uint8_t *bytes = (uint8_t *)ptr;
        for (size_t i = 0; i < size; i++) {
            bytes[i] = 0;
        }
    }
    
    return ptr;
}

/**
 * Free memory
 */
void kfree(void *ptr) {
    if (!ptr) {
        return;
    }
    
    // Get block header
    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));
    
    // Validate magic
    if (block->magic != HEAP_MAGIC) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("ERROR: Invalid free - bad magic!\n");
        return;
    }
    
    // Check double free
    if (block->is_free) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("ERROR: Double free detected!\n");
        return;
    }
    
    // Mark as free
    block->is_free = true;
    heap_used -= sizeof(heap_block_t) + block->size;
    
    // Merge adjacent free blocks
    merge_free_blocks();
}

/**
 * Reallocate memory
 */
void *krealloc(void *ptr, size_t size) {
    if (!ptr) {
        return kmalloc(size);
    }
    
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    // Get current block
    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));
    
    if (block->size >= size) {
        // Current block is large enough
        return ptr;
    }
    
    // Allocate new block
    void *new_ptr = kmalloc(size);
    if (!new_ptr) {
        return NULL;
    }
    
    // Copy old data
    uint8_t *src = (uint8_t *)ptr;
    uint8_t *dst = (uint8_t *)new_ptr;
    for (size_t i = 0; i < block->size && i < size; i++) {
        dst[i] = src[i];
    }
    
    // Free old block
    kfree(ptr);
    
    return new_ptr;
}

/**
 * Get heap statistics
 */
void heap_get_stats(size_t *total_bytes, size_t *used_bytes, size_t *free_bytes) {
    if (total_bytes) *total_bytes = heap_total;
    if (used_bytes) *used_bytes = heap_used;
    if (free_bytes) *free_bytes = heap_total - heap_used;
}

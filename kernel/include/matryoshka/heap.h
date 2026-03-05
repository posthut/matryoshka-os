/**
 * MatryoshkaOS - Kernel Heap Allocator
 * Dynamic memory allocation (kmalloc/kfree)
 * 
 * Simple linked-list based allocator for kernel heap
 */

#ifndef MATRYOSHKA_HEAP_H
#define MATRYOSHKA_HEAP_H

#include <matryoshka/types.h>

// Heap configuration
#define HEAP_START      0x00200000  // Heap starts at 2MB
#define HEAP_INITIAL    0x00100000  // Initial heap size: 1MB
#define HEAP_MAX        0x01000000  // Max heap size: 16MB

// Block header magic for validation
#define HEAP_MAGIC      0xDEADBEEF

/**
 * Initialize kernel heap
 * @return 0 on success, -1 on failure
 */
int heap_init(void);

/**
 * Allocate memory from kernel heap
 * @param size Size in bytes to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void *kmalloc(size_t size);

/**
 * Allocate and zero memory from kernel heap
 * @param size Size in bytes to allocate
 * @return Pointer to zeroed memory, or NULL on failure
 */
void *kzalloc(size_t size);

/**
 * Free previously allocated memory
 * @param ptr Pointer to memory to free
 */
void kfree(void *ptr);

/**
 * Reallocate memory (change size)
 * @param ptr Pointer to existing allocation
 * @param size New size in bytes
 * @return Pointer to reallocated memory, or NULL on failure
 */
void *krealloc(void *ptr, size_t size);

/**
 * Get heap statistics
 * @param total_bytes Total heap size
 * @param used_bytes Used heap memory
 * @param free_bytes Free heap memory
 */
void heap_get_stats(size_t *total_bytes, size_t *used_bytes, size_t *free_bytes);

#endif // MATRYOSHKA_HEAP_H

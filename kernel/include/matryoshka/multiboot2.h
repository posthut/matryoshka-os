/**
 * MatryoshkaOS - Multiboot2 Definitions
 * Structures and constants for parsing Multiboot2 information
 */

#ifndef MATRYOSHKA_MULTIBOOT2_H
#define MATRYOSHKA_MULTIBOOT2_H

#include <matryoshka/types.h>

// Multiboot2 magic value passed in EAX
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289

// Multiboot2 tag types
#define MULTIBOOT_TAG_TYPE_END              0
#define MULTIBOOT_TAG_TYPE_CMDLINE          1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME 2
#define MULTIBOOT_TAG_TYPE_MODULE           3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO    4
#define MULTIBOOT_TAG_TYPE_BOOTDEV          5
#define MULTIBOOT_TAG_TYPE_MMAP             6
#define MULTIBOOT_TAG_TYPE_VBE              7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER      8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS     9
#define MULTIBOOT_TAG_TYPE_APM              10
#define MULTIBOOT_TAG_TYPE_EFI32            11
#define MULTIBOOT_TAG_TYPE_EFI64            12
#define MULTIBOOT_TAG_TYPE_SMBIOS           13
#define MULTIBOOT_TAG_TYPE_ACPI_OLD         14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW         15
#define MULTIBOOT_TAG_TYPE_NETWORK          16
#define MULTIBOOT_TAG_TYPE_EFI_MMAP         17
#define MULTIBOOT_TAG_TYPE_EFI_BS           18
#define MULTIBOOT_TAG_TYPE_EFI32_IH         19
#define MULTIBOOT_TAG_TYPE_EFI64_IH         20
#define MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR   21

// Memory map entry types
#define MULTIBOOT_MEMORY_AVAILABLE          1
#define MULTIBOOT_MEMORY_RESERVED           2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE   3
#define MULTIBOOT_MEMORY_NVS                4
#define MULTIBOOT_MEMORY_BADRAM             5

/**
 * Multiboot2 information structure
 */
typedef struct multiboot_info {
    uint32_t total_size;
    uint32_t reserved;
} __attribute__((packed)) multiboot_info_t;

/**
 * Multiboot2 tag header
 */
typedef struct multiboot_tag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed)) multiboot_tag_t;

/**
 * Basic memory information tag
 */
typedef struct multiboot_tag_basic_meminfo {
    uint32_t type;
    uint32_t size;
    uint32_t mem_lower;  // KB of lower memory
    uint32_t mem_upper;  // KB of upper memory
} __attribute__((packed)) multiboot_tag_basic_meminfo_t;

/**
 * Memory map entry
 */
typedef struct multiboot_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
} __attribute__((packed)) multiboot_mmap_entry_t;

/**
 * Memory map tag
 */
typedef struct multiboot_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    multiboot_mmap_entry_t entries[0];
} __attribute__((packed)) multiboot_tag_mmap_t;

/**
 * Boot loader name tag
 */
typedef struct multiboot_tag_string {
    uint32_t type;
    uint32_t size;
    char string[0];
} __attribute__((packed)) multiboot_tag_string_t;

/**
 * Find a specific tag in Multiboot2 information
 * @param mbi Pointer to Multiboot2 info structure
 * @param type Tag type to find
 * @return Pointer to tag or NULL if not found
 */
multiboot_tag_t *multiboot_find_tag(multiboot_info_t *mbi, uint32_t type);

/**
 * Get total available memory in bytes
 * @param mbi Pointer to Multiboot2 info structure
 * @return Total available memory in bytes
 */
uint64_t multiboot_get_total_memory(multiboot_info_t *mbi);

/**
 * Get memory map from Multiboot2 info
 * @param mbi Pointer to Multiboot2 info structure
 * @return Pointer to memory map tag or NULL
 */
multiboot_tag_mmap_t *multiboot_get_mmap(multiboot_info_t *mbi);

#endif // MATRYOSHKA_MULTIBOOT2_H


/**
 * MatryoshkaOS - Unit tests for kernel/mm/pmm.c
 *
 * Tests: bitmap allocator, frame alloc/free, contiguous alloc, stats
 *
 * Strategy: mmap memory at a low address for _kernel_end (bitmap),
 * and build a fake Multiboot2 memory map above the bitmap region.
 * Link with: -Wl,--defsym,_kernel_end=0x400000
 */

#include "compat.h"
#include "framework.h"

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <matryoshka/pmm.h>

/*
 * _kernel_end is provided via --defsym at link time (0x400000).
 * We mmap that address to make it writable.
 * The fake memory map puts available RAM at 0x500000..0x2000000
 * (27 MB), so the bitmap/kernel region is safely below.
 */

#define BITMAP_ADDR  0x400000
#define BITMAP_LEN   0x100000   /* 1 MB guard */
#define AVAIL_START  0x500000   /* available memory starts here */
#define AVAIL_LEN    0x1B00000  /* 27 MB */

static int pmm_ready;

static void ensure_bitmap_mapped(void) {
    if (pmm_ready) return;
    void *p = mmap((void *)(uintptr_t)BITMAP_ADDR, BITMAP_LEN,
                   PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (p == MAP_FAILED) {
        fprintf(stderr, "SKIP: cannot mmap bitmap area at 0x%x\n", BITMAP_ADDR);
        exit(77);
    }
    pmm_ready = 1;
}

/*
 * Build a fake Multiboot2 info with one available region.
 */
static uint8_t fake_mbi_buf[256] __attribute__((aligned(8)));

static multiboot_info_t *build_fake_mbi(void) {
    memset(fake_mbi_buf, 0, sizeof(fake_mbi_buf));
    uint8_t *p = fake_mbi_buf;

    multiboot_info_t *mbi = (multiboot_info_t *)p;
    p += 8;

    multiboot_tag_mmap_t *mmap = (multiboot_tag_mmap_t *)p;
    mmap->type         = MULTIBOOT_TAG_TYPE_MMAP;
    mmap->entry_size   = sizeof(multiboot_mmap_entry_t);
    mmap->entry_version = 0;

    mmap->entries[0].addr = AVAIL_START;
    mmap->entries[0].len  = AVAIL_LEN;
    mmap->entries[0].type = MULTIBOOT_MEMORY_AVAILABLE;
    mmap->entries[0].zero = 0;

    mmap->size = sizeof(multiboot_tag_mmap_t) +
                 1 * sizeof(multiboot_mmap_entry_t);
    p += (mmap->size + 7) & ~7u;

    multiboot_tag_t *end = (multiboot_tag_t *)p;
    end->type = MULTIBOOT_TAG_TYPE_END;
    end->size = 8;
    p += 8;

    mbi->total_size = (uint32_t)(p - fake_mbi_buf);
    mbi->reserved   = 0;
    return mbi;
}

static void reinit(void) {
    ensure_bitmap_mapped();
    memset((void *)(uintptr_t)BITMAP_ADDR, 0, BITMAP_LEN);
    pmm_init(build_fake_mbi());
}

/* ── Allocation ───────────────────────────────────────────────────── */

static void test_alloc_single(void) {
    reinit();
    uint64_t addr = pmm_alloc_frame();
    ASSERT_NE(addr, 0);
    ASSERT_EQ(addr % PMM_FRAME_SIZE, 0);
}

static void test_alloc_in_avail_range(void) {
    reinit();
    uint64_t addr = pmm_alloc_frame();
    ASSERT_TRUE(addr >= AVAIL_START);
    ASSERT_TRUE(addr < AVAIL_START + AVAIL_LEN);
}

static void test_alloc_returns_unique(void) {
    reinit();
    uint64_t a = pmm_alloc_frame();
    uint64_t b = pmm_alloc_frame();
    uint64_t c = pmm_alloc_frame();
    ASSERT_NE(a, 0);
    ASSERT_NE(b, 0);
    ASSERT_NE(c, 0);
    ASSERT_NE(a, b);
    ASSERT_NE(b, c);
}

/* ── Free and re-allocate ─────────────────────────────────────────── */

static void test_free_and_realloc(void) {
    reinit();
    uint64_t a = pmm_alloc_frame();
    ASSERT_NE(a, 0);
    pmm_free_frame(a);

    uint64_t b = pmm_alloc_frame();
    ASSERT_EQ(a, b);
}

static void test_double_free_safe(void) {
    reinit();
    uint64_t a = pmm_alloc_frame();
    pmm_free_frame(a);
    pmm_free_frame(a);   /* should not crash or corrupt */
}

/* ── is_frame_allocated ───────────────────────────────────────────── */

static void test_is_allocated(void) {
    reinit();
    uint64_t a = pmm_alloc_frame();
    ASSERT_TRUE(pmm_is_frame_allocated(a));
    pmm_free_frame(a);
    ASSERT_FALSE(pmm_is_frame_allocated(a));
}

/* ── Contiguous allocation ────────────────────────────────────────── */

static void test_alloc_contiguous(void) {
    reinit();
    uint64_t base = pmm_alloc_frames(4);
    ASSERT_NE(base, 0);
    ASSERT_EQ(base % PMM_FRAME_SIZE, 0);

    for (int i = 0; i < 4; i++)
        ASSERT_TRUE(pmm_is_frame_allocated(base + i * PMM_FRAME_SIZE));

    pmm_free_frames(base, 4);
    for (int i = 0; i < 4; i++)
        ASSERT_FALSE(pmm_is_frame_allocated(base + i * PMM_FRAME_SIZE));
}

static void test_alloc_zero_frames(void) {
    reinit();
    ASSERT_EQ(pmm_alloc_frames(0), 0);
}

/* ── Reserve ──────────────────────────────────────────────────────── */

static void test_reserve_frame(void) {
    reinit();
    uint64_t addr = AVAIL_START + PMM_FRAME_SIZE * 100;
    ASSERT_FALSE(pmm_is_frame_allocated(addr));
    pmm_reserve_frame(addr);
    ASSERT_TRUE(pmm_is_frame_allocated(addr));
}

/* ── Stats ────────────────────────────────────────────────────────── */

static void test_stats_initial(void) {
    reinit();
    pmm_stats_t st;
    pmm_get_stats(&st);
    ASSERT_TRUE(st.total_frames > 0);
    ASSERT_TRUE(st.free_frames > 0);
    ASSERT_TRUE(st.free_frames <= st.total_frames);
}

static void test_stats_after_alloc(void) {
    reinit();
    pmm_stats_t before, after;
    pmm_get_stats(&before);

    pmm_alloc_frame();
    pmm_get_stats(&after);

    ASSERT_EQ(after.used_frames, before.used_frames + 1);
    ASSERT_EQ(after.free_frames, before.free_frames - 1);
}

/* ── Stress ───────────────────────────────────────────────────────── */

static void test_alloc_many(void) {
    reinit();
    pmm_stats_t st;
    pmm_get_stats(&st);

    uint64_t count = (st.free_frames < 500) ? st.free_frames : 500;
    for (uint64_t i = 0; i < count; i++) {
        uint64_t a = pmm_alloc_frame();
        ASSERT_NE(a, 0);
    }
}

/* ── Null safety ──────────────────────────────────────────────────── */

static void test_null_mbi(void) {
    pmm_init(NULL);   /* should not crash */
}

static void test_null_stats(void) {
    reinit();
    pmm_get_stats(NULL);   /* should not crash */
}

/* ── main ─────────────────────────────────────────────────────────── */

int main(void) {
    TEST_SUITE_BEGIN("kernel/mm/pmm.c");

    RUN_TEST(test_alloc_single);
    RUN_TEST(test_alloc_in_avail_range);
    RUN_TEST(test_alloc_returns_unique);
    RUN_TEST(test_free_and_realloc);
    RUN_TEST(test_double_free_safe);
    RUN_TEST(test_is_allocated);
    RUN_TEST(test_alloc_contiguous);
    RUN_TEST(test_alloc_zero_frames);
    RUN_TEST(test_reserve_frame);
    RUN_TEST(test_stats_initial);
    RUN_TEST(test_stats_after_alloc);
    RUN_TEST(test_alloc_many);
    RUN_TEST(test_null_mbi);
    RUN_TEST(test_null_stats);

    TEST_SUITE_END();
}

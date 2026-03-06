/**
 * MatryoshkaOS - Unit tests for kernel/mm/heap.c
 *
 * Tests: kmalloc, kfree, kzalloc, krealloc
 * Strategy: mmap a region at HEAP_START so the kernel heap code
 *           works unmodified on the host.
 */

#include "compat.h"
#include "framework.h"

#include <stdlib.h>
#include <sys/mman.h>

#include <matryoshka/heap.h>
#include <matryoshka/string.h>

static int heap_area_mapped = 0;

static void setup(void) {
    if (heap_area_mapped) return;

    void *p = mmap((void *)(uintptr_t)HEAP_START, HEAP_INITIAL,
                   PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (p == MAP_FAILED) {
        fprintf(stderr, "SKIP: cannot mmap 0x%x (heap area)\n", HEAP_START);
        exit(77);  /* standard "skip" code */
    }
    heap_area_mapped = 1;
}

static void reinit(void) {
    memset((void *)(uintptr_t)HEAP_START, 0, HEAP_INITIAL);
    heap_init();
}

/* ── Basic allocation ─────────────────────────────────────────────── */

static void test_alloc_basic(void) {
    reinit();
    void *p = kmalloc(100);
    ASSERT_NOT_NULL(p);
    kfree(p);
}

static void test_alloc_zero(void) {
    reinit();
    ASSERT_NULL(kmalloc(0));
}

static void test_alloc_multiple(void) {
    reinit();
    void *a = kmalloc(64);
    void *b = kmalloc(128);
    void *c = kmalloc(256);
    ASSERT_NOT_NULL(a);
    ASSERT_NOT_NULL(b);
    ASSERT_NOT_NULL(c);
    ASSERT_NE((uintptr_t)a, (uintptr_t)b);
    ASSERT_NE((uintptr_t)b, (uintptr_t)c);
    kfree(a);
    kfree(b);
    kfree(c);
}

/* ── kzalloc zeroes memory ────────────────────────────────────────── */

static void test_kzalloc(void) {
    reinit();
    unsigned char *p = kzalloc(256);
    ASSERT_NOT_NULL(p);
    for (int i = 0; i < 256; i++)
        ASSERT_EQ(p[i], 0);
    kfree(p);
}

/* ── Free and re-allocate reuses memory ───────────────────────────── */

static void test_free_reuse(void) {
    reinit();
    void *a = kmalloc(64);
    ASSERT_NOT_NULL(a);
    kfree(a);

    void *b = kmalloc(64);
    ASSERT_NOT_NULL(b);
    /* After free + merge the same region should be reused */
    ASSERT_EQ((uintptr_t)a, (uintptr_t)b);
    kfree(b);
}

/* ── krealloc preserves data ──────────────────────────────────────── */

static void test_krealloc_grow(void) {
    reinit();
    char *p = kmalloc(16);
    ASSERT_NOT_NULL(p);
    memcpy(p, "hello world!", 13);

    char *q = krealloc(p, 256);
    ASSERT_NOT_NULL(q);
    ASSERT_EQ(memcmp(q, "hello world!", 13), 0);
    kfree(q);
}

static void test_krealloc_null(void) {
    reinit();
    void *p = krealloc(NULL, 100);
    ASSERT_NOT_NULL(p);
    kfree(p);
}

static void test_krealloc_zero(void) {
    reinit();
    void *p = kmalloc(64);
    void *q = krealloc(p, 0);
    ASSERT_NULL(q);
}

/* ── Block splitting ──────────────────────────────────────────────── */

static void test_splitting(void) {
    reinit();
    void *a = kmalloc(32);
    void *b = kmalloc(32);
    ASSERT_NOT_NULL(a);
    ASSERT_NOT_NULL(b);
    /* b should start shortly after a (header + 32 bytes, aligned) */
    ASSERT_TRUE((uintptr_t)b > (uintptr_t)a);
    ASSERT_TRUE((uintptr_t)b - (uintptr_t)a < 256);
    kfree(a);
    kfree(b);
}

/* ── Alignment ────────────────────────────────────────────────────── */

static void test_alignment(void) {
    reinit();
    for (int i = 1; i <= 17; i++) {
        void *p = kmalloc(i);
        ASSERT_NOT_NULL(p);
        ASSERT_EQ((uintptr_t)p % 8, 0);
        kfree(p);
    }
}

/* ── Stress: many small allocations ───────────────────────────────── */

static void test_many_allocs(void) {
    reinit();
    void *ptrs[128];
    for (int i = 0; i < 128; i++) {
        ptrs[i] = kmalloc(32);
        ASSERT_NOT_NULL(ptrs[i]);
    }
    for (int i = 0; i < 128; i++)
        kfree(ptrs[i]);

    /* After freeing everything, a big allocation should succeed */
    void *big = kmalloc(4096);
    ASSERT_NOT_NULL(big);
    kfree(big);
}

/* ── Stats ────────────────────────────────────────────────────────── */

static void test_stats(void) {
    reinit();
    size_t total, used1, free1, used2, free2;
    heap_get_stats(&total, &used1, &free1);
    ASSERT_EQ(total, HEAP_INITIAL);
    ASSERT_TRUE(free1 > 0);

    void *p = kmalloc(512);
    ASSERT_NOT_NULL(p);
    heap_get_stats(NULL, &used2, &free2);
    ASSERT_TRUE(used2 > used1);
    ASSERT_TRUE(free2 < free1);
    kfree(p);
}

/* ── main ─────────────────────────────────────────────────────────── */

int main(void) {
    setup();
    TEST_SUITE_BEGIN("kernel/mm/heap.c");

    RUN_TEST(test_alloc_basic);
    RUN_TEST(test_alloc_zero);
    RUN_TEST(test_alloc_multiple);
    RUN_TEST(test_kzalloc);
    RUN_TEST(test_free_reuse);
    RUN_TEST(test_krealloc_grow);
    RUN_TEST(test_krealloc_null);
    RUN_TEST(test_krealloc_zero);
    RUN_TEST(test_splitting);
    RUN_TEST(test_alignment);
    RUN_TEST(test_many_allocs);
    RUN_TEST(test_stats);

    TEST_SUITE_END();
}

/**
 * MatryoshkaOS - Unit tests for kernel/string.c
 *
 * Tests: strlen, strcmp, strncmp, strcpy, strncpy,
 *        memset, memcpy, memmove, memcmp
 */

#include "compat.h"
#include "framework.h"
#include <matryoshka/string.h>

/* ── strlen ───────────────────────────────────────────────────────── */

static void test_strlen_empty(void) {
    ASSERT_EQ(strlen(""), 0);
}

static void test_strlen_one(void) {
    ASSERT_EQ(strlen("A"), 1);
}

static void test_strlen_normal(void) {
    ASSERT_EQ(strlen("hello"), 5);
}

static void test_strlen_with_spaces(void) {
    ASSERT_EQ(strlen("hello world"), 11);
}

/* ── strcmp ────────────────────────────────────────────────────────── */

static void test_strcmp_equal(void) {
    ASSERT_EQ(strcmp("abc", "abc"), 0);
}

static void test_strcmp_empty(void) {
    ASSERT_EQ(strcmp("", ""), 0);
}

static void test_strcmp_less(void) {
    ASSERT_TRUE(strcmp("abc", "abd") < 0);
}

static void test_strcmp_greater(void) {
    ASSERT_TRUE(strcmp("abd", "abc") > 0);
}

static void test_strcmp_prefix(void) {
    ASSERT_TRUE(strcmp("ab", "abc") < 0);
}

static void test_strcmp_longer(void) {
    ASSERT_TRUE(strcmp("abc", "ab") > 0);
}

/* ── strncmp ──────────────────────────────────────────────────────── */

static void test_strncmp_equal_within(void) {
    ASSERT_EQ(strncmp("abcdef", "abcxyz", 3), 0);
}

static void test_strncmp_differ_within(void) {
    ASSERT_TRUE(strncmp("abcdef", "abxyz", 3) < 0);
}

static void test_strncmp_zero_count(void) {
    ASSERT_EQ(strncmp("abc", "xyz", 0), 0);
}

/* ── strcpy ───────────────────────────────────────────────────────── */

static void test_strcpy_basic(void) {
    char buf[32];
    strcpy(buf, "hello");
    ASSERT_STR_EQ(buf, "hello");
}

static void test_strcpy_empty(void) {
    char buf[8] = "old";
    strcpy(buf, "");
    ASSERT_EQ(buf[0], '\0');
}

/* ── strncpy ──────────────────────────────────────────────────────── */

static void test_strncpy_full(void) {
    char buf[8];
    strncpy(buf, "hi", 8);
    ASSERT_STR_EQ(buf, "hi");
    ASSERT_EQ(buf[3], '\0');  /* remaining bytes zeroed */
}

static void test_strncpy_truncate(void) {
    char buf[4];
    strncpy(buf, "hello", 3);
    /* strncpy does NOT null-terminate when src >= n */
    ASSERT_EQ(buf[0], 'h');
    ASSERT_EQ(buf[1], 'e');
    ASSERT_EQ(buf[2], 'l');
}

/* ── memset ───────────────────────────────────────────────────────── */

static void test_memset_zero(void) {
    char buf[16] = "abcdefghijklmno";
    memset(buf, 0, 16);
    for (int i = 0; i < 16; i++)
        ASSERT_EQ(buf[i], 0);
}

static void test_memset_pattern(void) {
    unsigned char buf[8];
    memset(buf, 0xAA, 8);
    for (int i = 0; i < 8; i++)
        ASSERT_EQ(buf[i], 0xAA);
}

static void test_memset_partial(void) {
    char buf[8] = {0};
    memset(buf + 2, 'X', 3);
    ASSERT_EQ(buf[0], 0);
    ASSERT_EQ(buf[1], 0);
    ASSERT_EQ(buf[2], 'X');
    ASSERT_EQ(buf[4], 'X');
    ASSERT_EQ(buf[5], 0);
}

/* ── memcpy ───────────────────────────────────────────────────────── */

static void test_memcpy_basic(void) {
    const char src[] = "hello";
    char dst[8] = {0};
    memcpy(dst, src, 6);
    ASSERT_STR_EQ(dst, "hello");
}

static void test_memcpy_zero_len(void) {
    char dst[4] = "abc";
    memcpy(dst, "XYZ", 0);
    ASSERT_STR_EQ(dst, "abc");
}

/* ── memmove ──────────────────────────────────────────────────────── */

static void test_memmove_no_overlap(void) {
    char buf[16] = "hello world";
    char dst[16];
    memmove(dst, buf, 12);
    ASSERT_STR_EQ(dst, "hello world");
}

static void test_memmove_overlap_forward(void) {
    char buf[16] = "abcdefgh";
    memmove(buf + 2, buf, 6);  /* "ab" + "abcdef" */
    ASSERT_EQ(buf[0], 'a');
    ASSERT_EQ(buf[1], 'b');
    ASSERT_EQ(buf[2], 'a');
    ASSERT_EQ(buf[3], 'b');
    ASSERT_EQ(buf[4], 'c');
    ASSERT_EQ(buf[5], 'd');
    ASSERT_EQ(buf[6], 'e');
    ASSERT_EQ(buf[7], 'f');
}

static void test_memmove_overlap_backward(void) {
    char buf[16] = "abcdefgh";
    memmove(buf, buf + 2, 6);  /* "cdefgh" + "gh" */
    ASSERT_EQ(buf[0], 'c');
    ASSERT_EQ(buf[1], 'd');
    ASSERT_EQ(buf[2], 'e');
    ASSERT_EQ(buf[3], 'f');
    ASSERT_EQ(buf[4], 'g');
    ASSERT_EQ(buf[5], 'h');
}

/* ── memcmp ───────────────────────────────────────────────────────── */

static void test_memcmp_equal(void) {
    ASSERT_EQ(memcmp("abc", "abc", 3), 0);
}

static void test_memcmp_differ(void) {
    ASSERT_TRUE(memcmp("abc", "abd", 3) < 0);
}

static void test_memcmp_zero_len(void) {
    ASSERT_EQ(memcmp("abc", "xyz", 0), 0);
}

static void test_memcmp_partial(void) {
    ASSERT_EQ(memcmp("abcX", "abcY", 3), 0);
}

/* ── main ─────────────────────────────────────────────────────────── */

int main(void) {
    TEST_SUITE_BEGIN("kernel/string.c");

    /* strlen */
    RUN_TEST(test_strlen_empty);
    RUN_TEST(test_strlen_one);
    RUN_TEST(test_strlen_normal);
    RUN_TEST(test_strlen_with_spaces);

    /* strcmp */
    RUN_TEST(test_strcmp_equal);
    RUN_TEST(test_strcmp_empty);
    RUN_TEST(test_strcmp_less);
    RUN_TEST(test_strcmp_greater);
    RUN_TEST(test_strcmp_prefix);
    RUN_TEST(test_strcmp_longer);

    /* strncmp */
    RUN_TEST(test_strncmp_equal_within);
    RUN_TEST(test_strncmp_differ_within);
    RUN_TEST(test_strncmp_zero_count);

    /* strcpy */
    RUN_TEST(test_strcpy_basic);
    RUN_TEST(test_strcpy_empty);

    /* strncpy */
    RUN_TEST(test_strncpy_full);
    RUN_TEST(test_strncpy_truncate);

    /* memset */
    RUN_TEST(test_memset_zero);
    RUN_TEST(test_memset_pattern);
    RUN_TEST(test_memset_partial);

    /* memcpy */
    RUN_TEST(test_memcpy_basic);
    RUN_TEST(test_memcpy_zero_len);

    /* memmove */
    RUN_TEST(test_memmove_no_overlap);
    RUN_TEST(test_memmove_overlap_forward);
    RUN_TEST(test_memmove_overlap_backward);

    /* memcmp */
    RUN_TEST(test_memcmp_equal);
    RUN_TEST(test_memcmp_differ);
    RUN_TEST(test_memcmp_zero_len);
    RUN_TEST(test_memcmp_partial);

    TEST_SUITE_END();
}

/**
 * MatryoshkaOS - Minimal Unit Test Framework
 *
 * Usage:
 *   void test_something(void) {
 *       ASSERT_TRUE(1 + 1 == 2);
 *       ASSERT_EQ(strlen("hi"), 2);
 *       ASSERT_STR_EQ(buf, "hello");
 *   }
 *
 *   int main(void) {
 *       TEST_SUITE_BEGIN("string");
 *       RUN_TEST(test_something);
 *       TEST_SUITE_END();
 *   }
 */

#ifndef MSHKA_TEST_FRAMEWORK_H
#define MSHKA_TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>

static int _tf_total;
static int _tf_passed;
static int _tf_failed;
static int _tf_cur_fail;

#define TEST_SUITE_BEGIN(name) do {                                   \
    _tf_total = _tf_passed = _tf_failed = 0;                         \
    printf("\n\033[1;34m=== %s ===\033[0m\n", (name));               \
} while (0)

#define TEST_SUITE_END() do {                                         \
    printf("\n\033[1;34m--- Results: %d tests, "                      \
           "\033[32m%d passed\033[1;34m, "                            \
           "\033[%sm%d failed\033[0m\n",                              \
           _tf_total, _tf_passed,                                     \
           _tf_failed ? "31" : "32", _tf_failed);                     \
    return _tf_failed ? 1 : 0;                                        \
} while (0)

#define RUN_TEST(fn) do {                                             \
    _tf_cur_fail = 0;                                                 \
    fn();                                                             \
    _tf_total++;                                                      \
    if (_tf_cur_fail) {                                               \
        _tf_failed++;                                                 \
    } else {                                                          \
        _tf_passed++;                                                 \
        printf("  \033[32mPASS\033[0m  %s\n", #fn);                  \
    }                                                                 \
} while (0)

#define _TF_FAIL(file, line, msg) do {                                \
    printf("  \033[31mFAIL\033[0m  %s:%d: %s\n", (file), (line), (msg)); \
    _tf_cur_fail = 1;                                                 \
    return;                                                           \
} while (0)

#define ASSERT_TRUE(expr)                                             \
    do { if (!(expr)) _TF_FAIL(__FILE__, __LINE__, #expr); } while (0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

#define ASSERT_EQ(a, b) do {                                          \
    long long _a = (long long)(a), _b = (long long)(b);               \
    if (_a != _b) {                                                   \
        char _msg[256];                                               \
        snprintf(_msg, sizeof(_msg),                                  \
                 "%s == %s  (got %lld vs %lld)", #a, #b, _a, _b);    \
        _TF_FAIL(__FILE__, __LINE__, _msg);                           \
    }                                                                 \
} while (0)

#define ASSERT_NE(a, b) do {                                          \
    long long _a = (long long)(a), _b = (long long)(b);               \
    if (_a == _b) {                                                   \
        char _msg[256];                                               \
        snprintf(_msg, sizeof(_msg),                                  \
                 "%s != %s  (both %lld)", #a, #b, _a);               \
        _TF_FAIL(__FILE__, __LINE__, _msg);                           \
    }                                                                 \
} while (0)

#define ASSERT_NOT_NULL(ptr)                                          \
    do { if ((ptr) == NULL)                                           \
        _TF_FAIL(__FILE__, __LINE__, #ptr " != NULL"); } while (0)

#define ASSERT_NULL(ptr)                                              \
    do { if ((ptr) != NULL)                                           \
        _TF_FAIL(__FILE__, __LINE__, #ptr " == NULL"); } while (0)

#define ASSERT_STR_EQ(a, b) do {                                      \
    if (strcmp((a), (b)) != 0) {                                      \
        char _msg[256];                                               \
        snprintf(_msg, sizeof(_msg),                                  \
                 "strcmp(\"%s\", \"%s\") == 0", (a), (b));            \
        _TF_FAIL(__FILE__, __LINE__, _msg);                           \
    }                                                                 \
} while (0)

#define ASSERT_MEM_EQ(a, b, n) do {                                   \
    if (memcmp((a), (b), (n)) != 0)                                   \
        _TF_FAIL(__FILE__, __LINE__, "memcmp(" #a ", " #b ") == 0");  \
} while (0)

#endif /* MSHKA_TEST_FRAMEWORK_H */

/**
 * MatryoshkaOS - Unit tests for kernel/proc/task.c
 *
 * Tests: task_init, task_create, task_count, task_get_info
 *
 * NOTE: task_yield/task_exit use privileged instructions (cli/sti/hlt)
 * and context_switch assembly, so they are tested via QEMU integration
 * tests rather than host-side unit tests.
 */

#include "compat.h"
#include "framework.h"

#include <stdlib.h>
#include <string.h>

#include <matryoshka/task.h>

/* ── Mocks ────────────────────────────────────────────────────────── */

void *kmalloc(size_t size)  { return malloc(size); }
void  kfree(void *ptr)      { free(ptr); }

void context_switch(uint32_t *old_esp, uint32_t new_esp) {
    if (old_esp) *old_esp = new_esp;
}

void task_entry_trampoline(void) {}

/* ── Tests ────────────────────────────────────────────────────────── */

static void test_init(void) {
    task_init();
    ASSERT_EQ(task_count(), 1);
    ASSERT_EQ(task_current_id(), 0);
}

static void test_init_kernel_task(void) {
    task_init();
    uint32_t id;
    const char *name;
    task_state_t st;
    ASSERT_TRUE(task_get_info(0, &id, &name, &st));
    ASSERT_EQ(id, 0);
    ASSERT_STR_EQ(name, "kernel");
    ASSERT_EQ(st, TASK_RUNNING);
}

static void test_create_single(void) {
    task_init();
    uint32_t id = task_create(task_entry_trampoline, "test_a");
    ASSERT_NE(id, (uint32_t)-1);
    ASSERT_EQ(id, 1);
    ASSERT_EQ(task_count(), 2);
}

static void test_create_state_ready(void) {
    task_init();
    uint32_t id = task_create(task_entry_trampoline, "worker");
    uint32_t qid;
    const char *name;
    task_state_t st;
    ASSERT_TRUE(task_get_info(id, &qid, &name, &st));
    ASSERT_EQ(st, TASK_READY);
    ASSERT_STR_EQ(name, "worker");
}

static void test_create_multiple(void) {
    task_init();
    uint32_t a = task_create(task_entry_trampoline, "a");
    uint32_t b = task_create(task_entry_trampoline, "b");
    uint32_t c = task_create(task_entry_trampoline, "c");
    ASSERT_EQ(a, 1);
    ASSERT_EQ(b, 2);
    ASSERT_EQ(c, 3);
    ASSERT_EQ(task_count(), 4);
}

static void test_get_info_invalid(void) {
    task_init();
    uint32_t id;
    const char *name;
    task_state_t st;
    ASSERT_FALSE(task_get_info(999, &id, &name, &st));
}

static void test_names_preserved(void) {
    task_init();
    task_create(task_entry_trampoline, "alpha");
    task_create(task_entry_trampoline, "beta");

    uint32_t id;
    const char *name;
    task_state_t st;

    task_get_info(1, &id, &name, &st);
    ASSERT_STR_EQ(name, "alpha");

    task_get_info(2, &id, &name, &st);
    ASSERT_STR_EQ(name, "beta");
}

static void test_create_max(void) {
    task_init();
    /* Fill up to MAX_TASKS */
    for (int i = 1; i < MAX_TASKS; i++) {
        uint32_t id = task_create(task_entry_trampoline, "t");
        ASSERT_NE(id, (uint32_t)-1);
    }
    ASSERT_EQ(task_count(), MAX_TASKS);

    /* One more should fail */
    uint32_t over = task_create(task_entry_trampoline, "overflow");
    ASSERT_EQ(over, (uint32_t)-1);
}

/* ── main ─────────────────────────────────────────────────────────── */

int main(void) {
    TEST_SUITE_BEGIN("kernel/proc/task.c");

    RUN_TEST(test_init);
    RUN_TEST(test_init_kernel_task);
    RUN_TEST(test_create_single);
    RUN_TEST(test_create_state_ready);
    RUN_TEST(test_create_multiple);
    RUN_TEST(test_get_info_invalid);
    RUN_TEST(test_names_preserved);
    RUN_TEST(test_create_max);

    TEST_SUITE_END();
}

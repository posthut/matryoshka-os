/**
 * MatryoshkaOS - Unit tests for kernel/proc/task.c
 *
 * Tests: task_init, task_create, task_count, task_get_info,
 *        task_schedule_if_needed, task_request_reschedule
 *
 * NOTE: task_yield/task_exit use INT 0x81 / HLT, so they are tested
 * via QEMU integration tests rather than host-side unit tests.
 */

#include "compat.h"
#include "framework.h"

#include <stdlib.h>
#include <string.h>

#include <matryoshka/task.h>

/* ── Mocks ────────────────────────────────────────────────────────── */

void *kmalloc(size_t size)  { return malloc(size); }
void  kfree(void *ptr)      { free(ptr); }

static void dummy_entry(void) {}

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
    uint32_t id = task_create(dummy_entry, "test_a");
    ASSERT_NE(id, (uint32_t)-1);
    ASSERT_EQ(id, 1);
    ASSERT_EQ(task_count(), 2);
}

static void test_create_state_ready(void) {
    task_init();
    uint32_t id = task_create(dummy_entry, "worker");
    uint32_t qid;
    const char *name;
    task_state_t st;
    ASSERT_TRUE(task_get_info(id, &qid, &name, &st));
    ASSERT_EQ(st, TASK_READY);
    ASSERT_STR_EQ(name, "worker");
}

static void test_create_multiple(void) {
    task_init();
    uint32_t a = task_create(dummy_entry, "a");
    uint32_t b = task_create(dummy_entry, "b");
    uint32_t c = task_create(dummy_entry, "c");
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
    task_create(dummy_entry, "alpha");
    task_create(dummy_entry, "beta");

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
        uint32_t id = task_create(dummy_entry, "t");
        ASSERT_NE(id, (uint32_t)-1);
    }
    ASSERT_EQ(task_count(), MAX_TASKS);

    /* One more should fail */
    uint32_t over = task_create(dummy_entry, "overflow");
    ASSERT_EQ(over, (uint32_t)-1);
}

static void test_schedule_no_reschedule(void) {
    task_init();
    task_create(dummy_entry, "worker");
    uint32_t esp = 0xDEAD0000;
    ASSERT_EQ(task_schedule_if_needed(esp), esp);
}

static void test_schedule_switches(void) {
    task_init();
    task_create(dummy_entry, "worker");
    task_request_reschedule();
    uint32_t new_esp = task_schedule_if_needed(0xDEAD0000);
    ASSERT_NE(new_esp, 0xDEAD0000);
    ASSERT_EQ(task_current_id(), 1);
}

static void test_schedule_sole_task(void) {
    task_init();
    task_request_reschedule();
    uint32_t esp = 0xBEEF0000;
    ASSERT_EQ(task_schedule_if_needed(esp), esp);
    ASSERT_EQ(task_current_id(), 0);
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
    RUN_TEST(test_schedule_no_reschedule);
    RUN_TEST(test_schedule_switches);
    RUN_TEST(test_schedule_sole_task);

    TEST_SUITE_END();
}

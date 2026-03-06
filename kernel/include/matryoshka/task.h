/**
 * MatryoshkaOS - Kernel Task (Thread) Management
 * Preemptive multitasking for ring-0 kernel threads
 */

#ifndef MATRYOSHKA_TASK_H
#define MATRYOSHKA_TASK_H

#include <matryoshka/types.h>

#define MAX_TASKS       64
#define TASK_STACK_SIZE 4096

typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_TERMINATED,
} task_state_t;

/**
 * Register the current (boot) thread as task 0 and enable
 * the scheduler.  Must be called after idt_init().
 */
void task_init(void);

/**
 * Spawn a new kernel thread (ring 0).
 * @return task id, or (uint32_t)-1 on failure.
 */
uint32_t task_create(void (*entry)(void), const char *name);

/**
 * Spawn a user-mode task (ring 3).
 * The function at @a entry must only use syscalls (INT 0x80) for I/O.
 * @return task id, or (uint32_t)-1 on failure.
 */
uint32_t task_create_user(void (*entry)(void), const char *name);

/**
 * Voluntarily give up the CPU (triggers INT 0x81).
 */
void task_yield(void);

/**
 * Terminate the current task.  Called automatically when a task's
 * entry function returns.  Never returns.
 */
void task_exit(void);

/**
 * Request the scheduler to run at the next opportunity
 * (typically called from the timer IRQ handler).
 */
void task_request_reschedule(void);

/**
 * Called from isr_handler after every interrupt.
 * If a reschedule is pending, saves the current ESP and returns
 * the next task's ESP; otherwise returns @a esp unchanged.
 */
uint32_t task_schedule_if_needed(uint32_t esp);

/* Query helpers */
uint32_t task_count(void);
uint32_t task_current_id(void);
bool     task_get_info(uint32_t index, uint32_t *id,
                       const char **name, task_state_t *state);

#endif /* MATRYOSHKA_TASK_H */

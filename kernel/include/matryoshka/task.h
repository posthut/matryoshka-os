/**
 * MatryoshkaOS - Kernel Task (Thread) Management
 * Cooperative multitasking for ring-0 kernel threads
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
 * Register the current (boot) thread as task 0 ("kernel").
 * Must be called before task_create().
 */
void task_init(void);

/**
 * Spawn a new kernel thread.
 * @return task id, or (uint32_t)-1 on failure.
 */
uint32_t task_create(void (*entry)(void), const char *name);

/**
 * Voluntarily give up the CPU.
 * If no other READY task exists, performs HLT and returns.
 */
void task_yield(void);

/**
 * Terminate the current task. Called automatically when a task's
 * entry function returns.  Never returns.
 */
void task_exit(void);

uint32_t task_count(void);
uint32_t task_current_id(void);
bool     task_get_info(uint32_t index, uint32_t *id,
                       const char **name, task_state_t *state);

/* Assembly helpers (context.asm) */
extern void context_switch(uint32_t *old_esp, uint32_t new_esp);
extern void task_entry_trampoline(void);

#endif /* MATRYOSHKA_TASK_H */

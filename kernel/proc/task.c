/**
 * MatryoshkaOS - Kernel Task (Thread) Scheduler
 * Preemptive round-robin for ring-0 threads.
 *
 * Context switches happen uniformly through the ISR path:
 *   - Timer IRQ (preemptive): timer handler requests reschedule
 *   - INT 0x81  (voluntary):  task_yield() triggers software interrupt
 * In both cases isr_handler() calls task_schedule_if_needed() which
 * may return a different task's ESP.
 */

#include <matryoshka/task.h>
#include <matryoshka/idt.h>
#include <matryoshka/heap.h>
#include <matryoshka/vga.h>
#include <matryoshka/string.h>

typedef struct {
    uint32_t      id;
    uint32_t      esp;
    uint8_t      *stack_base;
    task_state_t  state;
    const char   *name;
} task_t;

static task_t   tasks[MAX_TASKS];
static uint32_t current;
static uint32_t count;
static bool     scheduler_ready;
static volatile bool need_reschedule;

/* ── Internal helpers ─────────────────────────────────────────────── */

static uint32_t find_next_ready(void) {
    uint32_t id = (current + 1) % count;
    for (uint32_t i = 0; i < count; i++) {
        if (id != current &&
            (tasks[id].state == TASK_READY ||
             tasks[id].state == TASK_RUNNING))
            return id;
        id = (id + 1) % count;
    }
    return current;
}

static void yield_isr_handler(interrupt_frame_t *frame) {
    (void)frame;
    need_reschedule = true;
}

/* ── Public API ───────────────────────────────────────────────────── */

void task_init(void) {
    memset(tasks, 0, sizeof(tasks));

    tasks[0].id         = 0;
    tasks[0].state      = TASK_RUNNING;
    tasks[0].name       = "kernel";
    tasks[0].stack_base = NULL;

    current          = 0;
    count            = 1;
    need_reschedule  = false;

    idt_register_handler(129, yield_isr_handler);

    scheduler_ready = true;

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] Task scheduler initialized (preemptive)\n");
}

uint32_t task_create(void (*entry)(void), const char *name) {
    if (count >= MAX_TASKS) return (uint32_t)-1;

    uint32_t id = count++;
    uint8_t *stack = (uint8_t *)kmalloc(TASK_STACK_SIZE);
    if (!stack) { count--; return (uint32_t)-1; }

    /*
     * Build a fake ISR frame so that isr_common_stub can "return"
     * into this task the first time it is scheduled.
     *
     *   [top]       task_exit   <- entry()'s return address
     *   [top -  4]  EFLAGS     <- iret restores IF=1
     *   [top -  8]  CS 0x08
     *   [top - 12]  EIP=entry
     *   [top - 16]  err_code 0
     *   [top - 20]  int_no 0
     *   [top - 24]  EAX 0       <- pusha block
     *   [top - 28]  ECX 0
     *   [top - 32]  EDX 0
     *   [top - 36]  EBX 0
     *   [top - 40]  ESP 0 (ignored by popa)
     *   [top - 44]  EBP 0
     *   [top - 48]  ESI 0
     *   [top - 52]  EDI 0
     *   [top - 56]  DS 0x10     <- saved ESP points here
     */
    uint32_t *sp = (uint32_t *)(stack + TASK_STACK_SIZE);

    *(--sp) = (uint32_t)task_exit;
    *(--sp) = 0x202;                /* EFLAGS: IF=1, reserved bit 1 */
    *(--sp) = 0x08;                 /* CS */
    *(--sp) = (uint32_t)entry;      /* EIP */
    *(--sp) = 0;                    /* err_code */
    *(--sp) = 0;                    /* int_no */
    *(--sp) = 0;                    /* EAX */
    *(--sp) = 0;                    /* ECX */
    *(--sp) = 0;                    /* EDX */
    *(--sp) = 0;                    /* EBX */
    *(--sp) = 0;                    /* ESP (popa ignores) */
    *(--sp) = 0;                    /* EBP */
    *(--sp) = 0;                    /* ESI */
    *(--sp) = 0;                    /* EDI */
    *(--sp) = 0x10;                 /* DS */

    tasks[id].id         = id;
    tasks[id].esp        = (uint32_t)sp;
    tasks[id].stack_base = stack;
    tasks[id].state      = TASK_READY;
    tasks[id].name       = name;

    return id;
}

void task_yield(void) {
    __asm__ volatile("int $0x81");
}

void task_exit(void) {
    tasks[current].state = TASK_TERMINATED;
    task_yield();
    while (1) __asm__ volatile("hlt");
}

void task_request_reschedule(void) {
    need_reschedule = true;
}

uint32_t task_schedule_if_needed(uint32_t esp) {
    if (!scheduler_ready || !need_reschedule)
        return esp;
    need_reschedule = false;

    uint32_t next = find_next_ready();
    if (next == current)
        return esp;

    tasks[current].esp = esp;
    if (tasks[current].state == TASK_RUNNING)
        tasks[current].state = TASK_READY;

    current = next;
    tasks[current].state = TASK_RUNNING;

    return tasks[current].esp;
}

/* ── Query helpers ────────────────────────────────────────────────── */

uint32_t task_count(void)      { return count; }
uint32_t task_current_id(void) { return current; }

bool task_get_info(uint32_t index, uint32_t *id,
                   const char **name, task_state_t *state) {
    if (index >= count) return false;
    *id    = tasks[index].id;
    *name  = tasks[index].name;
    *state = tasks[index].state;
    return true;
}

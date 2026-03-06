/**
 * MatryoshkaOS - Kernel Task (Thread) Scheduler
 * Cooperative round-robin for ring-0 threads
 */

#include <matryoshka/task.h>
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

static task_t  tasks[MAX_TASKS];
static uint32_t current;
static uint32_t count;

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

/* ── Public API ───────────────────────────────────────────────────── */

void task_init(void) {
    memset(tasks, 0, sizeof(tasks));

    tasks[0].id         = 0;
    tasks[0].state      = TASK_RUNNING;
    tasks[0].name       = "kernel";
    tasks[0].stack_base = NULL;

    current = 0;
    count   = 1;

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] Task scheduler initialized (cooperative)\n");
}

uint32_t task_create(void (*entry)(void), const char *name) {
    if (count >= MAX_TASKS) return (uint32_t)-1;

    uint32_t id = count++;
    uint8_t *stack = (uint8_t *)kmalloc(TASK_STACK_SIZE);
    if (!stack) { count--; return (uint32_t)-1; }

    /*
     * Initial stack layout (grows downward):
     *
     *   [top - 4]   task_exit          <- entry()'s return address
     *   [top - 8]   entry              <- trampoline's ret target
     *   [top - 12]  task_entry_trampoline <- context_switch ret target
     *   [top - 16]  0  (ebp)
     *   [top - 20]  0  (ebx)
     *   [top - 24]  0  (esi)
     *   [top - 28]  0  (edi)  <- saved ESP points here
     */
    uint32_t *sp = (uint32_t *)(stack + TASK_STACK_SIZE);

    *(--sp) = (uint32_t)task_exit;
    *(--sp) = (uint32_t)entry;
    *(--sp) = (uint32_t)task_entry_trampoline;
    *(--sp) = 0;   /* ebp */
    *(--sp) = 0;   /* ebx */
    *(--sp) = 0;   /* esi */
    *(--sp) = 0;   /* edi */

    tasks[id].id         = id;
    tasks[id].esp        = (uint32_t)sp;
    tasks[id].stack_base = stack;
    tasks[id].state      = TASK_READY;
    tasks[id].name       = name;

    return id;
}

void task_yield(void) {
    __asm__ volatile("cli");

    uint32_t next = find_next_ready();
    if (next == current) {
        /* Sole runnable task — sleep until next interrupt */
        __asm__ volatile("sti; hlt");
        return;
    }

    uint32_t prev = current;
    current = next;

    if (tasks[prev].state == TASK_RUNNING)
        tasks[prev].state = TASK_READY;
    tasks[next].state = TASK_RUNNING;

    context_switch(&tasks[prev].esp, tasks[next].esp);

    /* Resumed — re-enable interrupts */
    __asm__ volatile("sti");
}

void task_exit(void) {
    tasks[current].state = TASK_TERMINATED;
    task_yield();
    /* Should never reach here */
    while (1) __asm__ volatile("cli; hlt");
}

/* ── Query helpers (used by shell `ps`) ───────────────────────────── */

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

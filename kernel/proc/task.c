/**
 * MatryoshkaOS - Kernel Task (Thread) Scheduler
 * Preemptive round-robin for ring-0 and ring-3 threads.
 *
 * Context switches happen uniformly through the ISR path:
 *   - Timer IRQ (preemptive): timer handler requests reschedule
 *   - INT 0x81  (voluntary):  task_yield() triggers software interrupt
 * In both cases isr_handler() calls task_schedule_if_needed() which
 * may return a different task's ESP.
 *
 * Ring-3 tasks get a separate user stack; the CPU switches to the
 * kernel stack (via TSS.ESP0) on INT 0x80 / hardware interrupts.
 */

#include <matryoshka/task.h>
#include <matryoshka/idt.h>
#include <matryoshka/gdt.h>
#include <matryoshka/heap.h>
#include <matryoshka/vmm.h>
#include <matryoshka/vga.h>
#include <matryoshka/string.h>
#include <matryoshka/serial.h>

typedef struct {
    uint32_t      id;
    uint32_t      esp;
    uint8_t      *stack_base;       /* kernel stack (always present) */
    uint8_t      *user_stack_base;  /* user stack (ring 3 only, else NULL) */
    uint32_t      kernel_stack_top; /* top of kernel stack for TSS.ESP0 */
    task_state_t  state;
    const char   *name;
    bool          is_user;
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

    tasks[0].id              = 0;
    tasks[0].state           = TASK_RUNNING;
    tasks[0].name            = "kernel";
    tasks[0].stack_base      = NULL;
    tasks[0].user_stack_base = NULL;
    tasks[0].kernel_stack_top = 0;
    tasks[0].is_user         = false;

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
     * Build a fake ISR frame (ring 0) so isr_common_stub can
     * "return" into this task the first time it is scheduled.
     *
     * Layout (high → low):
     *   task_exit     ← entry()'s return address
     *   EFLAGS (IF=1)
     *   CS 0x08
     *   EIP = entry
     *   err_code 0
     *   int_no 0
     *   pusha block (EAX..EDI = 0)
     *   DS 0x10      ← saved ESP points here
     */
    uint32_t *sp = (uint32_t *)(stack + TASK_STACK_SIZE);

    *(--sp) = (uint32_t)task_exit;
    *(--sp) = 0x202;                /* EFLAGS: IF=1 */
    *(--sp) = GDT_KERNEL_CODE_SEL;  /* CS */
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
    *(--sp) = GDT_KERNEL_DATA_SEL;  /* DS */

    tasks[id].id              = id;
    tasks[id].esp             = (uint32_t)sp;
    tasks[id].stack_base      = stack;
    tasks[id].user_stack_base = NULL;
    tasks[id].kernel_stack_top = (uint32_t)(stack + TASK_STACK_SIZE);
    tasks[id].state           = TASK_READY;
    tasks[id].name            = name;
    tasks[id].is_user         = false;

    return id;
}

uint32_t task_create_user(void (*entry)(void), const char *name) {
    if (count >= MAX_TASKS) return (uint32_t)-1;

    uint32_t id = count++;
    uint8_t *kstack = (uint8_t *)kmalloc(TASK_STACK_SIZE);
    uint8_t *ustack = (uint8_t *)kmalloc(TASK_STACK_SIZE);
    if (!kstack || !ustack) {
        if (kstack) kfree(kstack);
        if (ustack) kfree(ustack);
        count--;
        return (uint32_t)-1;
    }

    /* Mark user stack pages as user-accessible */
    for (uint32_t off = 0; off < TASK_STACK_SIZE; off += PAGE_SIZE) {
        vmm_set_user((uint32_t)ustack + off);
    }

    /* Mark pages containing the user function as user-accessible.
     * We mark two pages to cover the case where the function straddles
     * a page boundary (the function itself is small). */
    vmm_set_user(PAGE_ALIGN_DOWN((uint32_t)entry));
    vmm_set_user(PAGE_ALIGN_DOWN((uint32_t)entry) + PAGE_SIZE);

    /*
     * Build a ring-3 ISR frame on the KERNEL stack.
     * When the CPU irets to ring 3 it pops: EIP, CS, EFLAGS, ESP, SS.
     *
     * Layout (high → low):
     *   user SS       = 0x23  (GDT_USER_DATA_SEL)
     *   user ESP      = top of user stack
     *   EFLAGS (IF=1)
     *   CS 0x1B       (GDT_USER_CODE_SEL)
     *   EIP = entry
     *   err_code 0
     *   int_no 0
     *   pusha block   (all 0)
     *   DS 0x23       ← saved ESP points here
     */
    uint32_t *sp = (uint32_t *)(kstack + TASK_STACK_SIZE);

    *(--sp) = GDT_USER_DATA_SEL;                /* user SS */
    *(--sp) = (uint32_t)(ustack + TASK_STACK_SIZE); /* user ESP */
    *(--sp) = 0x202;                            /* EFLAGS: IF=1 */
    *(--sp) = GDT_USER_CODE_SEL;                /* CS */
    *(--sp) = (uint32_t)entry;                  /* EIP */
    *(--sp) = 0;                                /* err_code */
    *(--sp) = 0;                                /* int_no */
    *(--sp) = 0;                                /* EAX */
    *(--sp) = 0;                                /* ECX */
    *(--sp) = 0;                                /* EDX */
    *(--sp) = 0;                                /* EBX */
    *(--sp) = 0;                                /* ESP (popa ignores) */
    *(--sp) = 0;                                /* EBP */
    *(--sp) = 0;                                /* ESI */
    *(--sp) = 0;                                /* EDI */
    *(--sp) = GDT_USER_DATA_SEL;                /* DS */

    tasks[id].id              = id;
    tasks[id].esp             = (uint32_t)sp;
    tasks[id].stack_base      = kstack;
    tasks[id].user_stack_base = ustack;
    tasks[id].kernel_stack_top = (uint32_t)(kstack + TASK_STACK_SIZE);
    tasks[id].state           = TASK_READY;
    tasks[id].name            = name;
    tasks[id].is_user         = true;

    klog("task: created user-mode task");
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

    /* Update TSS.ESP0 so ring 3→0 transitions use the right stack */
    if (tasks[current].kernel_stack_top)
        tss_set_esp0(tasks[current].kernel_stack_top);

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

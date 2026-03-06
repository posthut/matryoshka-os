/**
 * MatryoshkaOS - System Call Handler
 *
 * INT 0x80 dispatches to the syscall table based on EAX.
 * Arguments arrive in EBX, ECX, EDX, ESI, EDI.
 * Return value is placed in frame->eax.
 */

#include <matryoshka/syscall.h>
#include <matryoshka/idt.h>
#include <matryoshka/vga.h>
#include <matryoshka/serial.h>
#include <matryoshka/keyboard.h>
#include <matryoshka/timer.h>
#include <matryoshka/task.h>
#include <matryoshka/heap.h>
#include <matryoshka/string.h>

/* ── Individual syscall implementations ──────────────────────────── */

static int32_t sys_exit(interrupt_frame_t *f) {
    (void)f;
    klog("syscall: exit");
    task_exit();
    return 0;
}

static int32_t sys_write(interrupt_frame_t *f) {
    int fd       = (int)f->ebx;
    const char *buf = (const char *)f->ecx;
    size_t len   = (size_t)f->edx;

    if (!buf || len == 0) return 0;

    for (size_t i = 0; i < len; i++) {
        if (fd == 1) {
            vga_putchar(buf[i]);
        } else if (fd == 2) {
            serial_putc(buf[i]);
        } else {
            return -1;
        }
    }
    return (int32_t)len;
}

static int32_t sys_read(interrupt_frame_t *f) {
    int fd     = (int)f->ebx;
    char *buf  = (char *)f->ecx;
    size_t len = (size_t)f->edx;

    if (fd != 0 || !buf || len == 0) return -1;

    char c = keyboard_getchar();
    if (c == 0) return 0;
    buf[0] = c;
    return 1;
}

static int32_t sys_getpid(interrupt_frame_t *f) {
    (void)f;
    return (int32_t)task_current_id();
}

static int32_t sys_uptime(interrupt_frame_t *f) {
    (void)f;
    return (int32_t)timer_get_ticks();
}

static int32_t sys_sbrk(interrupt_frame_t *f) {
    uint32_t increment = f->ebx;
    void *ptr = kmalloc(increment);
    return ptr ? (int32_t)(uint32_t)ptr : -1;
}

static int32_t sys_yield(interrupt_frame_t *f) {
    (void)f;
    task_request_reschedule();
    return 0;
}

/* ── Dispatch table ──────────────────────────────────────────────── */

typedef int32_t (*syscall_fn)(interrupt_frame_t *);

static syscall_fn syscall_table[SYS_MAX] = {
    [SYS_EXIT]   = sys_exit,
    [SYS_WRITE]  = sys_write,
    [SYS_READ]   = sys_read,
    [SYS_GETPID] = sys_getpid,
    [SYS_UPTIME] = sys_uptime,
    [SYS_SBRK]   = sys_sbrk,
    [SYS_YIELD]  = sys_yield,
};

/* ── INT 0x80 handler ────────────────────────────────────────────── */

static void syscall_handler(interrupt_frame_t *frame) {
    uint32_t num = frame->eax;

    if (num >= SYS_MAX || !syscall_table[num]) {
        frame->eax = (uint32_t)-1;
        return;
    }

    frame->eax = (uint32_t)syscall_table[num](frame);
}

void syscall_init(void) {
    idt_register_handler(128, syscall_handler);
    klog("syscall: INT 0x80 handler registered");
}

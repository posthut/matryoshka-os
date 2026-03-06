/**
 * MatryoshkaOS - System Call Interface
 *
 * Convention (Linux-like):
 *   EAX = syscall number
 *   EBX = arg1, ECX = arg2, EDX = arg3, ESI = arg4, EDI = arg5
 *   Return value in EAX (written to frame->eax)
 */

#ifndef MATRYOSHKA_SYSCALL_H
#define MATRYOSHKA_SYSCALL_H

#include <matryoshka/types.h>
#include <matryoshka/idt.h>

/* Syscall numbers */
#define SYS_EXIT     0
#define SYS_WRITE    1
#define SYS_READ     2
#define SYS_GETPID   3
#define SYS_UPTIME   4
#define SYS_SBRK     5
#define SYS_YIELD    6
#define SYS_MAX      7

void syscall_init(void);

/* ── User-space wrappers (inline asm) ─────────────────────────────── */

static inline int32_t _syscall0(uint32_t num) {
    int32_t ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num));
    return ret;
}

static inline int32_t _syscall1(uint32_t num, uint32_t a1) {
    int32_t ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(a1));
    return ret;
}

static inline int32_t _syscall3(uint32_t num,
                                uint32_t a1, uint32_t a2, uint32_t a3) {
    int32_t ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(num), "b"(a1), "c"(a2), "d"(a3));
    return ret;
}

#endif /* MATRYOSHKA_SYSCALL_H */

; MatryoshkaOS - Legacy cooperative context switch stubs
; Kept as no-op symbols so existing code that references them
; still links.  All context switching now goes through the ISR path
; (see isr.asm / task.c).

section .note.GNU-stack noalloc noexec nowrite progbits

section .text
bits 32

global context_switch
context_switch:
    ret

global task_entry_trampoline
task_entry_trampoline:
    ret

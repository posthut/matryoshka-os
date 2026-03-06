; MatryoshkaOS - Context switch for kernel threads
; Saves/restores callee-saved registers and switches stacks.

section .note.GNU-stack noalloc noexec nowrite progbits

section .text
bits 32

; void context_switch(uint32_t *old_esp, uint32_t new_esp)
;   Saves EBP/EBX/ESI/EDI on the current stack, stores ESP into
;   *old_esp, loads new_esp into ESP, restores registers and returns
;   into the new task's saved context.
global context_switch
context_switch:
    push ebp
    push ebx
    push esi
    push edi

    mov eax, [esp + 20]        ; old_esp pointer
    mov [eax], esp              ; save current ESP

    mov esp, [esp + 24]         ; load new task's ESP

    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

; Trampoline for newly created tasks.
; context_switch "returns" here; we enable interrupts then fall
; through (ret) into the real entry function whose address sits
; on the stack just above us.
global task_entry_trampoline
task_entry_trampoline:
    sti
    ret

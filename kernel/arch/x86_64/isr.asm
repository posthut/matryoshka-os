; MatryoshkaOS - Interrupt Service Routines (ISR)
; Assembly stubs for CPU exceptions and hardware interrupts

section .note.GNU-stack noalloc noexec nowrite progbits

section .text

; Macro for ISRs without error code
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push 0              ; Push dummy error code
    push %1             ; Push interrupt number
    jmp isr_common_stub
%endmacro

; Macro for ISRs with error code
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    ; CPU already pushed error code
    push %1             ; Push interrupt number
    jmp isr_common_stub
%endmacro

; Macro for IRQ handlers
%macro IRQ 2
global irq%1
irq%1:
    push 0              ; Push dummy error code
    push %2             ; Push interrupt number (32 + IRQ number)
    jmp isr_common_stub
%endmacro

; CPU Exceptions (0-31)
ISR_NOERRCODE 0     ; Division by zero
ISR_NOERRCODE 1     ; Debug
ISR_NOERRCODE 2     ; Non-maskable interrupt
ISR_NOERRCODE 3     ; Breakpoint
ISR_NOERRCODE 4     ; Overflow
ISR_NOERRCODE 5     ; Bound range exceeded
ISR_NOERRCODE 6     ; Invalid opcode
ISR_NOERRCODE 7     ; Device not available
ISR_ERRCODE   8     ; Double fault (error code)
ISR_NOERRCODE 9     ; Coprocessor segment overrun
ISR_ERRCODE   10    ; Invalid TSS (error code)
ISR_ERRCODE   11    ; Segment not present (error code)
ISR_ERRCODE   12    ; Stack-segment fault (error code)
ISR_ERRCODE   13    ; General protection fault (error code)
ISR_ERRCODE   14    ; Page fault (error code)
ISR_NOERRCODE 15    ; Reserved
ISR_NOERRCODE 16    ; x87 FPU error
ISR_ERRCODE   17    ; Alignment check (error code)
ISR_NOERRCODE 18    ; Machine check
ISR_NOERRCODE 19    ; SIMD floating-point exception
ISR_NOERRCODE 20    ; Virtualization exception
ISR_NOERRCODE 21    ; Reserved
ISR_NOERRCODE 22    ; Reserved
ISR_NOERRCODE 23    ; Reserved
ISR_NOERRCODE 24    ; Reserved
ISR_NOERRCODE 25    ; Reserved
ISR_NOERRCODE 26    ; Reserved
ISR_NOERRCODE 27    ; Reserved
ISR_NOERRCODE 28    ; Reserved
ISR_NOERRCODE 29    ; Reserved
ISR_ERRCODE   30    ; Security exception (error code)
ISR_NOERRCODE 31    ; Reserved

; Hardware Interrupts (IRQs 0-15, mapped to interrupts 32-47)
IRQ 0,  32    ; Timer
IRQ 1,  33    ; Keyboard
IRQ 2,  34    ; Cascade
IRQ 3,  35    ; COM2
IRQ 4,  36    ; COM1
IRQ 5,  37    ; LPT2
IRQ 6,  38    ; Floppy
IRQ 7,  39    ; LPT1
IRQ 8,  40    ; RTC
IRQ 9,  41    ; Free
IRQ 10, 42    ; Free
IRQ 11, 43    ; Free
IRQ 12, 44    ; PS/2 Mouse
IRQ 13, 45    ; FPU
IRQ 14, 46    ; Primary ATA
IRQ 15, 47    ; Secondary ATA

; Software interrupts
ISR_NOERRCODE 128   ; INT 0x80 — system calls
ISR_NOERRCODE 129   ; INT 0x81 — task yield

; Common ISR stub
; Saves CPU state, calls C handler (which may switch tasks by
; returning a different ESP), restores state and irets.
extern isr_handler

isr_common_stub:
    pusha

    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; pass frame pointer
    call isr_handler    ; returns (possibly new) ESP in EAX
    mov esp, eax        ; switch to returned stack

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8
    iret

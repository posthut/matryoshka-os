; MatryoshkaOS - Minimal Kernel Entry Point
; 32-bit to 64-bit transition test

; Multiboot2 header
section .multiboot
    align 8
multiboot_header_start:
    dd 0xE85250D6                   ; Multiboot2 magic number
    dd 0                            ; Architecture: i386 protected mode
    dd multiboot_header_end - multiboot_header_start  ; Header length
    dd -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header_start))  ; Checksum
    
    ; Entry address tag
    align 8
    dw 3                            ; Type: entry address
    dw 0                            ; Flags
    dd 12                           ; Size
    dd _start                       ; Entry point address
    
    ; End tag
    align 8
    dw 0                            ; Type
    dw 0                            ; Flags
    dd 8                            ; Size
multiboot_header_end:

; Boot section
section .text.boot
bits 32
global _start
extern kernel_main

_start:
    ; Test 1: Write 'OK' to screen
    mov dword [0xB8000], 0x2F4B2F4F  ; 'OK' green on black
    
    ; Save Multiboot info pointer (GRUB passes it in EBX)
    mov esi, ebx                    ; Save Multiboot info address
    
    ; Setup stack
    mov esp, stack_top
    
    ; Write '2' to show stack is ready
    mov dword [0xB8004], 0x2F322F32  ; '22' green
    
    ; Call kernel with Multiboot info
    push esi                        ; Pass Multiboot address as parameter
    call kernel_main
    
    ; If kernel returns, halt
    cli
.hang:
    hlt
    jmp .hang

; BSS section for stack
section .bss
align 16
stack_bottom:
    resb 16384                      ; 16KB stack
stack_top:

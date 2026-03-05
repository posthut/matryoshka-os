; MatryoshkaOS - Kernel Entry Point
; x86_64 Assembly - Multiboot2 compatible

; Multiboot2 header
section .multiboot
    align 8
multiboot_header_start:
    dd 0xE85250D6                   ; Multiboot2 magic number
    dd 0                            ; Architecture: i386 protected mode
    dd multiboot_header_end - multiboot_header_start  ; Header length
    dd -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header_start))  ; Checksum

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
    ; Setup stack
    mov esp, stack_top
    
    ; Save Multiboot information
    mov edi, ebx                    ; EBX contains Multiboot info pointer
    mov esi, eax                    ; EAX contains Multiboot magic
    
    ; Check CPU compatibility
    call check_cpuid
    call check_long_mode
    
    ; Setup paging for long mode
    call setup_page_tables
    call enable_paging
    
    ; Load GDT
    lgdt [gdt64.pointer]
    
    ; Jump to long mode
    jmp gdt64.code:long_mode_start

; Check if CPUID is supported
check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_cpuid
    ret
.no_cpuid:
    mov al, 'C'
    jmp error

; Check if long mode is available
check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode
    
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode
    ret
.no_long_mode:
    mov al, 'L'
    jmp error

; Setup page tables for identity mapping
setup_page_tables:
    ; Map P4[0] -> P3
    mov eax, p3_table
    or eax, 0b11                    ; Present + writable
    mov [p4_table], eax
    
    ; Map P3[0] -> P2
    mov eax, p2_table
    or eax, 0b11
    mov [p3_table], eax
    
    ; Map P2 entries (2MB pages)
    mov ecx, 0
.map_p2_table:
    mov eax, 0x200000               ; 2MB
    mul ecx
    or eax, 0b10000011              ; Present + writable + huge page
    mov [p2_table + ecx * 8], eax
    
    inc ecx
    cmp ecx, 512
    jne .map_p2_table
    
    ret

; Enable paging and long mode
enable_paging:
    ; Load P4 to CR3
    mov eax, p4_table
    mov cr3, eax
    
    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    
    ; Set long mode bit in EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    
    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    
    ret

; Error handler
error:
    ; Print "ERR: X" to screen where X is error code in AL
    mov dword [0xB8000], 0x4F524F45  ; "ER" in white on red
    mov dword [0xB8004], 0x4F3A4F52  ; "R:" in white on red
    mov byte [0xB8008], al
    mov byte [0xB8009], 0x4F
    hlt

; 64-bit long mode entry
bits 64
long_mode_start:
    ; Load data segments
    mov ax, gdt64.data
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Debug: Write 'A' to screen (top-left corner)
    mov word [0xB8000], 0x0F41      ; 'A' in white on black
    
    ; Clear the screen (skip first character)
    mov rax, 0x0F200F200F200F20     ; Space with white on black
    mov rdi, 0xB8002                ; Start from second character
    mov rcx, 499                     ; 80*25 / 4 - 1
    rep stosq
    
    ; Debug: Write 'B' after spaces
    mov word [0xB8002], 0x0F42      ; 'B' in white on black
    
    ; Restore Multiboot info pointer to RDI (first parameter in System V AMD64 ABI)
    ; EDI was saved in 32-bit mode, now extend to RDI in 64-bit mode
    mov edi, edi                     ; Zero-extend EDI to RDI
    
    ; Debug: Write 'C' before calling kernel
    mov word [0xB8004], 0x0F43      ; 'C' in white on black
    
    ; Call kernel main with Multiboot info
    call kernel_main
    
    ; Debug: Write 'D' if kernel returns
    mov word [0xB8006], 0x0F44      ; 'D' in white on black
    
    ; If kernel returns, halt
.halt:
    hlt
    jmp .halt

; GDT for 64-bit mode
section .rodata
gdt64:
    dq 0                            ; Null descriptor
.code: equ $ - gdt64
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53)  ; Code segment
.data: equ $ - gdt64
    dq (1<<44) | (1<<47)            ; Data segment
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

; BSS section for page tables and stack
section .bss
align 4096
p4_table:
    resb 4096
p3_table:
    resb 4096
p2_table:
    resb 4096
stack_bottom:
    resb 16384                      ; 16KB stack
stack_top:


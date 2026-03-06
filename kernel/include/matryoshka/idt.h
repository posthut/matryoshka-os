/**
 * MatryoshkaOS - Interrupt Descriptor Table (IDT)
 * CPU interrupt and exception handling
 */

#ifndef MATRYOSHKA_IDT_H
#define MATRYOSHKA_IDT_H

#include <matryoshka/types.h>

// IDT entry count (256 entries: 0-31 exceptions, 32-255 interrupts)
#define IDT_ENTRIES 256

// IDT gate types
#define IDT_GATE_TASK       0x5  // Task gate
#define IDT_GATE_INT16      0x6  // 16-bit interrupt gate
#define IDT_GATE_TRAP16     0x7  // 16-bit trap gate
#define IDT_GATE_INT32      0xE  // 32-bit interrupt gate
#define IDT_GATE_TRAP32     0xF  // 32-bit trap gate

// IDT flags
#define IDT_FLAG_PRESENT    0x80 // Present bit
#define IDT_FLAG_DPL0       0x00 // Ring 0 (kernel)
#define IDT_FLAG_DPL3       0x60 // Ring 3 (user)

/**
 * IDT entry structure (8 bytes)
 * Describes one interrupt handler
 */
typedef struct {
    uint16_t base_low;      // Lower 16 bits of handler address
    uint16_t selector;      // Code segment selector (0x08)
    uint8_t  zero;          // Always 0
    uint8_t  flags;         // Type and attributes
    uint16_t base_high;     // Upper 16 bits of handler address
} __attribute__((packed)) idt_entry_t;

/**
 * IDT pointer structure (6 bytes)
 * Loaded into IDTR register
 */
typedef struct {
    uint16_t limit;         // Size of IDT - 1
    uint32_t base;          // Address of IDT
} __attribute__((packed)) idt_ptr_t;

/**
 * Interrupt frame structure
 * Pushed by CPU and our ISR stub
 */
typedef struct {
    // Pushed by our ISR stub
    uint32_t ds;                        // Data segment
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // General purpose registers
    uint32_t int_no, err_code;          // Interrupt number and error code
    
    // Pushed by CPU automatically
    uint32_t eip, cs, eflags, useresp, ss;
} __attribute__((packed)) interrupt_frame_t;

/**
 * Interrupt handler function type (called per-vector by isr_handler)
 */
typedef void (*isr_handler_t)(interrupt_frame_t *frame);

/**
 * Master ISR dispatcher (called from assembly).
 * Returns the ESP to use for the iret path; the scheduler may
 * return a different task's ESP to perform a context switch.
 */
uint32_t isr_handler(uint32_t esp);

/**
 * Initialize IDT
 */
void idt_init(void);

/**
 * Set IDT gate (entry)
 * @param num Interrupt number (0-255)
 * @param base Handler function address
 * @param selector Code segment selector (usually 0x08)
 * @param flags Gate type and attributes
 */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags);

/**
 * Register interrupt handler
 * @param num Interrupt number
 * @param handler Handler function
 */
void idt_register_handler(uint8_t num, isr_handler_t handler);

// ISR declarations (defined in isr.asm)
// CPU Exceptions (0-31)
extern void isr0(void);   // Division by zero
extern void isr1(void);   // Debug
extern void isr2(void);   // Non-maskable interrupt
extern void isr3(void);   // Breakpoint
extern void isr4(void);   // Overflow
extern void isr5(void);   // Bound range exceeded
extern void isr6(void);   // Invalid opcode
extern void isr7(void);   // Device not available
extern void isr8(void);   // Double fault
extern void isr9(void);   // Coprocessor segment overrun
extern void isr10(void);  // Invalid TSS
extern void isr11(void);  // Segment not present
extern void isr12(void);  // Stack-segment fault
extern void isr13(void);  // General protection fault
extern void isr14(void);  // Page fault
extern void isr15(void);  // Reserved
extern void isr16(void);  // x87 FPU error
extern void isr17(void);  // Alignment check
extern void isr18(void);  // Machine check
extern void isr19(void);  // SIMD floating-point exception
extern void isr20(void);  // Virtualization exception
extern void isr21(void);  // Reserved
extern void isr22(void);  // Reserved
extern void isr23(void);  // Reserved
extern void isr24(void);  // Reserved
extern void isr25(void);  // Reserved
extern void isr26(void);  // Reserved
extern void isr27(void);  // Reserved
extern void isr28(void);  // Reserved
extern void isr29(void);  // Reserved
extern void isr30(void);  // Security exception
extern void isr31(void);  // Reserved

// Hardware interrupts (IRQs 32-47)
extern void irq0(void);   // Timer
extern void irq1(void);   // Keyboard
extern void irq2(void);   // Cascade
extern void irq3(void);   // COM2
extern void irq4(void);   // COM1
extern void irq5(void);   // LPT2
extern void irq6(void);   // Floppy
extern void irq7(void);   // LPT1
extern void irq8(void);   // RTC
extern void irq9(void);   // Free
extern void irq10(void);  // Free
extern void irq11(void);  // Free
extern void irq12(void);  // PS/2 Mouse
extern void irq13(void);  // FPU
extern void irq14(void);  // Primary ATA
extern void irq15(void);  // Secondary ATA

// Software interrupts
extern void isr129(void);  // INT 0x81 — task yield

#endif // MATRYOSHKA_IDT_H

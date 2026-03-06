/**
 * MatryoshkaOS - Programmable Interrupt Controller (PIC)
 * 8259 PIC configuration and IRQ management
 */

#ifndef MATRYOSHKA_PIC_H
#define MATRYOSHKA_PIC_H

#include <matryoshka/types.h>

// PIC I/O ports
#define PIC1_COMMAND    0x20    // Master PIC command port
#define PIC1_DATA       0x21    // Master PIC data port
#define PIC2_COMMAND    0xA0    // Slave PIC command port
#define PIC2_DATA       0xA1    // Slave PIC data port

// PIC commands
#define PIC_EOI         0x20    // End of interrupt

// ICW1 (Initialization Command Word 1)
#define ICW1_ICW4       0x01    // ICW4 needed
#define ICW1_SINGLE     0x02    // Single mode (no cascade)
#define ICW1_INTERVAL4  0x04    // Call address interval 4
#define ICW1_LEVEL      0x08    // Level triggered mode
#define ICW1_INIT       0x10    // Initialization

// ICW4 (Initialization Command Word 4)
#define ICW4_8086       0x01    // 8086/88 mode
#define ICW4_AUTO       0x02    // Auto EOI
#define ICW4_BUF_SLAVE  0x08    // Buffered mode (slave)
#define ICW4_BUF_MASTER 0x0C    // Buffered mode (master)
#define ICW4_SFNM       0x10    // Special fully nested mode

// IRQ offset in IDT
#define IRQ_OFFSET      32      // IRQs start at interrupt 32

/**
 * Initialize PIC (remap IRQs)
 * Remaps hardware IRQs 0-15 to interrupts 32-47
 */
void pic_init(void);

/**
 * Send End of Interrupt (EOI) signal
 * @param irq IRQ number (0-15)
 */
void pic_send_eoi(uint8_t irq);

/**
 * Enable specific IRQ
 * @param irq IRQ number (0-15)
 */
void pic_enable_irq(uint8_t irq);

/**
 * Disable specific IRQ
 * @param irq IRQ number (0-15)
 */
void pic_disable_irq(uint8_t irq);

/**
 * Get combined IRQ mask
 * @return 16-bit mask (master + slave)
 */
uint16_t pic_get_mask(void);

/**
 * Set combined IRQ mask
 * @param mask 16-bit mask (master + slave)
 */
void pic_set_mask(uint16_t mask);

#endif // MATRYOSHKA_PIC_H

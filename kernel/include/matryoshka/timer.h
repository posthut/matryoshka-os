/**
 * MatryoshkaOS - Programmable Interval Timer (PIT)
 * 8253/8254 PIT driver for system timer (IRQ0)
 */

#ifndef MATRYOSHKA_TIMER_H
#define MATRYOSHKA_TIMER_H

#include <matryoshka/types.h>

// PIT I/O ports
#define PIT_CHANNEL0    0x40    // Channel 0 data port (system timer)
#define PIT_CHANNEL1    0x41    // Channel 1 data port (RAM refresh, obsolete)
#define PIT_CHANNEL2    0x42    // Channel 2 data port (PC speaker)
#define PIT_COMMAND     0x43    // Mode/Command register

// PIT frequency
#define PIT_FREQUENCY   1193182 // PIT base frequency in Hz

// Timer frequency (ticks per second)
#define TIMER_FREQUENCY 100     // 100 Hz = 10ms per tick

// PIT command register bits
#define PIT_SELECT_CHANNEL0 0x00    // Select channel 0
#define PIT_ACCESS_LOHI     0x30    // Access mode: lobyte/hibyte
#define PIT_MODE_SQUARE     0x06    // Mode 3: Square wave generator
#define PIT_BINARY          0x00    // Binary mode (not BCD)

/**
 * Initialize PIT timer
 * Sets up timer to generate IRQ0 at TIMER_FREQUENCY Hz
 */
void timer_init(void);

/**
 * Get current tick count
 * @return Number of timer ticks since boot
 */
uint64_t timer_get_ticks(void);

/**
 * Get uptime in milliseconds
 * @return System uptime in milliseconds
 */
uint64_t timer_get_uptime_ms(void);

/**
 * Get uptime in seconds
 * @return System uptime in seconds
 */
uint32_t timer_get_uptime_sec(void);

/**
 * Sleep for specified number of ticks
 * @param ticks Number of timer ticks to sleep
 */
void timer_sleep(uint32_t ticks);

/**
 * Sleep for specified milliseconds
 * @param ms Milliseconds to sleep
 */
void timer_sleep_ms(uint32_t ms);

/**
 * Timer interrupt handler (called from IRQ0)
 */
void timer_handler(void);

#endif // MATRYOSHKA_TIMER_H

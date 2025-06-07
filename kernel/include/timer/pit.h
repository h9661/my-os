#ifndef PIT_H
#define PIT_H

#include "../common/types.h"

/* PIT (Programmable Interval Timer) 8253/8254 chip */

/* PIT I/O ports */
#define PIT_CHANNEL_0_DATA      0x40    /* Channel 0 data port (system timer) */
#define PIT_CHANNEL_1_DATA      0x41    /* Channel 1 data port (RAM refresh) */
#define PIT_CHANNEL_2_DATA      0x42    /* Channel 2 data port (PC speaker) */
#define PIT_COMMAND_PORT        0x43    /* Command/mode control port */

/* PIT operating modes */
#define PIT_MODE_0              0x00    /* Interrupt on terminal count */
#define PIT_MODE_1              0x02    /* Hardware retriggerable one-shot */
#define PIT_MODE_2              0x04    /* Rate generator */
#define PIT_MODE_3              0x06    /* Square wave generator */
#define PIT_MODE_4              0x08    /* Software triggered strobe */
#define PIT_MODE_5              0x0A    /* Hardware triggered strobe */

/* PIT access modes */
#define PIT_ACCESS_LATCH        0x00    /* Latch count value command */
#define PIT_ACCESS_LOBYTE       0x10    /* Access low byte only */
#define PIT_ACCESS_HIBYTE       0x20    /* Access high byte only */
#define PIT_ACCESS_LOHIBYTE     0x30    /* Access low byte then high byte */

/* PIT channel select */
#define PIT_CHANNEL_0           0x00    /* Select counter 0 */
#define PIT_CHANNEL_1           0x40    /* Select counter 1 */
#define PIT_CHANNEL_2           0x80    /* Select counter 2 */

/* PIT binary/BCD mode */
#define PIT_BINARY_MODE         0x00    /* Binary mode */
#define PIT_BCD_MODE            0x01    /* BCD mode */

/* Standard PIT frequency and common timer frequencies */
#define PIT_BASE_FREQUENCY      1193182 /* Base frequency of PIT (approximately 1.193182 MHz) */
#define TIMER_FREQUENCY_1000HZ  1000    /* 1000 Hz = 1ms intervals */
#define TIMER_FREQUENCY_100HZ   100     /* 100 Hz = 10ms intervals */
#define TIMER_FREQUENCY_50HZ    50      /* 50 Hz = 20ms intervals */
#define TIMER_FREQUENCY_18HZ    18      /* 18.2 Hz = default BIOS frequency */

/* Timer variables */
extern volatile uint32_t system_ticks;      /* System timer tick counter */
extern volatile uint32_t timer_frequency;   /* Current timer frequency in Hz */

/* Function prototypes */
void pit_initialize(void);
void pit_set_frequency(uint32_t frequency);
uint32_t pit_calculate_divisor(uint32_t frequency);
void pit_set_divisor(uint16_t divisor);
void timer_tick(void);
uint32_t timer_get_ticks(void);
uint32_t timer_get_seconds(void);
uint32_t timer_get_milliseconds(void);
void timer_reset(void);

/* Timer callback function type */
typedef void (*timer_callback_t)(void);

/* Timer callback management */
void timer_set_callback(timer_callback_t callback);
void timer_clear_callback(void);

#endif /* PIT_H */

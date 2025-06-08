#include "../../include/timer/pit.h"
#include "../../include/terminal/terminal.h"
#include "../../include/common/utils.h"
#include "../../include/process/process.h"

/* Global timer variables */
volatile uint32_t system_ticks = 0;         /* Number of timer ticks since boot */
volatile uint32_t timer_frequency = 0;      /* Current timer frequency */
static timer_callback_t timer_callback = NULL; /* Optional timer callback */

/* Port I/O helper functions */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Initialize PIT with default frequency */
void pit_initialize(void) {
    terminal_writeline("Initializing PIT (Programmable Interval Timer)...");
    
    /* Reset system tick counter */
    system_ticks = 0;
    timer_callback = NULL;
    
    /* Set default timer frequency to 1000 Hz (1ms intervals) */
    pit_set_frequency(TIMER_FREQUENCY_1000HZ);
    
    terminal_writeline("PIT initialized successfully!");
    
    char freq_str[32];
    strcpy(freq_str, "Timer frequency set to: ");
    char num_str[16];
    int_to_string(timer_frequency, num_str);
    strcat(freq_str, num_str);
    strcat(freq_str, " Hz");
    terminal_writeline(freq_str);
}

/* Set timer frequency */
void pit_set_frequency(uint32_t frequency) {
    if (frequency == 0) {
        terminal_writeline("Error: Timer frequency cannot be zero!");
        return;
    }
    
    /* Calculate divisor */
    uint16_t divisor = pit_calculate_divisor(frequency);
    
    /* Set the divisor */
    pit_set_divisor(divisor);
    
    /* Store the actual frequency */
    timer_frequency = PIT_BASE_FREQUENCY / divisor;
    
    char msg[64];
    strcpy(msg, "Timer frequency changed to ");
    char freq_str[16];
    int_to_string(timer_frequency, freq_str);
    strcat(msg, freq_str);
    strcat(msg, " Hz");
    terminal_writeline(msg);
}

/* Calculate divisor for given frequency */
uint32_t pit_calculate_divisor(uint32_t frequency) {
    if (frequency == 0) {
        return 65535; /* Maximum divisor for minimum frequency */
    }
    
    uint32_t divisor = PIT_BASE_FREQUENCY / frequency;
    
    /* Clamp divisor to valid range (1-65535) */
    if (divisor == 0) {
        divisor = 1;
    } else if (divisor > 65535) {
        divisor = 65535;
    }
    
    return divisor;
}

/* Set PIT divisor directly */
void pit_set_divisor(uint16_t divisor) {
    /* Disable interrupts during PIT programming */
    __asm__ volatile("cli");
    
    /* Send command byte to PIT command port */
    /* Channel 0, Access mode: lobyte/hibyte, Mode 3: square wave, Binary mode */
    uint8_t command = PIT_CHANNEL_0 | PIT_ACCESS_LOHIBYTE | PIT_MODE_3 | PIT_BINARY_MODE;
    outb(PIT_COMMAND_PORT, command);
    
    /* Send divisor low byte first, then high byte */
    outb(PIT_CHANNEL_0_DATA, divisor & 0xFF);        /* Low byte */
    outb(PIT_CHANNEL_0_DATA, (divisor >> 8) & 0xFF); /* High byte */
    
    /* Re-enable interrupts */
    __asm__ volatile("sti");
}

/* Get current tick count */
uint32_t timer_get_ticks(void) {
    return system_ticks;
}

/* Get seconds since boot */
uint32_t timer_get_seconds(void) {
    if (timer_frequency == 0) return 0;
    return system_ticks / timer_frequency;
}

/* Get milliseconds since boot */
uint32_t timer_get_milliseconds(void) {
    if (timer_frequency == 0) return 0;
    return (system_ticks * 1000) / timer_frequency;
}

/* Reset timer counter */
void timer_reset(void) {
    __asm__ volatile("cli"); /* Disable interrupts */
    system_ticks = 0;
    __asm__ volatile("sti"); /* Re-enable interrupts */
    terminal_writeline("System timer reset to 0");
}

/* Set timer callback function */
void timer_set_callback(timer_callback_t callback) {
    timer_callback = callback;
    if (callback) {
        terminal_writeline("Timer callback function registered");
    }
}

/* Clear timer callback function */
void timer_clear_callback(void) {
    timer_callback = NULL;
    terminal_writeline("Timer callback function cleared");
}

/* Timer tick handler - called from interrupt handler */
void timer_tick(void) {
    /* Increment system tick counter */
    system_ticks++;
    
    /* Call scheduler tick for multitasking */
    scheduler_tick();
    
    /* Call user callback if registered */
    if (timer_callback) {
        timer_callback();
    }
}

/* Display timer information */
void timer_display_info(void) {
    terminal_writeline("========== Timer Information ==========");
    
    char line[64];
    char num_str[16];
    
    /* Current frequency */
    strcpy(line, "Frequency: ");
    int_to_string(timer_frequency, num_str);
    strcat(line, num_str);
    strcat(line, " Hz");
    terminal_writeline(line);
    
    /* Current ticks */
    strcpy(line, "System ticks: ");
    int_to_string(system_ticks, num_str);
    strcat(line, num_str);
    terminal_writeline(line);
    
    /* Uptime in seconds */
    strcpy(line, "Uptime: ");
    int_to_string(timer_get_seconds(), num_str);
    strcat(line, num_str);
    strcat(line, " seconds");
    terminal_writeline(line);
    
    /* Callback status */
    strcpy(line, "Callback: ");
    strcat(line, timer_callback ? "Registered" : "None");
    terminal_writeline(line);
    
    terminal_writeline("======================================");
}

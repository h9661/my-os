#include "../../include/interrupts/interrupts.h"
#include "../../include/vga/vga.h"
#include "../../include/common/utils.h"
#include "../../include/keyboard/keyboard.h"

/* Helper function to send EOI signal to PIC */
static inline void send_eoi(uint32_t int_no) {
    /* For IRQs 0-7 (interrupts 32-39), send EOI to master PIC only */
    /* For IRQs 8-15 (interrupts 40-47), send EOI to both slave and master PIC */
    if (int_no >= 40) {
        /* Slave PIC */
        __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0xA0));
    }
    /* Master PIC - always send for any IRQ */
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0x20));
}

/* Timer interrupt handler */
void c_irq_handler_timer(void) {
    /* Uncomment this to see timer interrupts (can cause a lot of output) */
    /*
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("Timer Interrupt (IRQ 0)");
    terminal_writeline("");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    */
    
    /* Send EOI to PIC */
    send_eoi(32);
}

/* Keyboard interrupt handler */
void c_irq_handler_keyboard(void) {
    /* Call keyboard handler function */
    keyboard_handler();
    
    /* Send EOI to PIC */
    send_eoi(33);
}
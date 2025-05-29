#include "../../include/interrupts/interrupts.h"
#include "../../include/vga/vga.h"
#include "../../include/common/utils.h"
#include "../../include/keyboard/keyboard.h"

/* Exception names */
static const char* exception_messages[] = {
    "Divide by Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception"
};

/* Common exception handler */
void exception_handler_common(uint32_t int_no, uint32_t err_code) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
    terminal_writestring("EXCEPTION: ");
    
    if (int_no < sizeof(exception_messages) / sizeof(exception_messages[0])) {
        terminal_writestring(exception_messages[int_no]);
    } else {
        terminal_writestring("Unknown Exception");
    }
    
    terminal_writestring(" (");
    char num_str[16];
    int_to_string(int_no, num_str);
    terminal_writestring(num_str);
    terminal_writestring(")");
    
    if (err_code) {
        terminal_writestring(" - Error Code: ");
        int_to_string(err_code, num_str);
        terminal_writestring(num_str);
    }
    
    terminal_writeline("");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    /* Halt system */
    while (1) {
        __asm__ volatile("hlt");
    }
}

/* Common IRQ handler */
void irq_handler_common(uint32_t int_no, uint32_t err_code __attribute__((unused))) {
    switch (int_no) {
      case 32: /* IRQ 0 - Timer */
          /* Handle timer interrupt - comment out to reduce spam */
          /* terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
          terminal_writestring("Timer Interrupt (IRQ 0)");
          terminal_writeline("");
          terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK)); */
          break;

      case 33: /* IRQ 1 - Keyboard */
          /* Handle keyboard interrupt */
          terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
          terminal_writestring("[KBD] ");
          terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
          keyboard_handler();
          break;
    }
    
    /* Send End of Interrupt signal to PIC */
    /* For IRQs 0-7 (interrupts 32-39), send EOI to master PIC only */
    /* For IRQs 8-15 (interrupts 40-47), send EOI to both slave and master PIC */
    if (int_no >= 40) {
        /* Slave PIC */
        __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0xA0));
    }
    /* Master PIC - always send for any IRQ */
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0x20));
}
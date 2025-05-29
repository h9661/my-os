#include "../../include/interrupts/interrupts.h"
#include "../../include/vga/vga.h"
#include "../../include/common/utils.h"

/* Helper functions for port I/O */
static inline void __asm_outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t __asm_inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* IDT table (256 entries) */
static idt_entry_t idt[256];
static idt_ptr_t idt_ptr;

/* Set IDT gate */
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags) {
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].offset_high = (handler >> 16) & 0xFFFF;
    idt[num].selector = selector;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
}

/* Load IDT */
static void idt_load(void) {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint32_t)&idt;
    
    __asm__ volatile("lidt %0" : : "m"(idt_ptr));
}

/* Initialize IDT */
void interrupts_initialize(void) {
    /* Clear IDT */
    memset(&idt, 0, sizeof(idt));
    
    /* Set exception handlers */
    idt_set_gate(0, (uint32_t)exception_handler_0, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(1, (uint32_t)exception_handler_1, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(2, (uint32_t)exception_handler_2, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(3, (uint32_t)exception_handler_3, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(4, (uint32_t)exception_handler_4, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(5, (uint32_t)exception_handler_5, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(6, (uint32_t)exception_handler_6, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(7, (uint32_t)exception_handler_7, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(8, (uint32_t)exception_handler_8, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(9, (uint32_t)exception_handler_9, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(10, (uint32_t)exception_handler_10, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(11, (uint32_t)exception_handler_11, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(12, (uint32_t)exception_handler_12, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(13, (uint32_t)exception_handler_13, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(14, (uint32_t)exception_handler_14, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(15, (uint32_t)exception_handler_15, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(16, (uint32_t)exception_handler_16, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(17, (uint32_t)exception_handler_17, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(18, (uint32_t)exception_handler_18, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(19, (uint32_t)exception_handler_19, 0x08, IDT_TYPE_INTERRUPT_GATE);
    
    /* Set hardware interrupt handlers */
    idt_set_gate(32, (uint32_t)irq_handler_timer, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(33, (uint32_t)irq_handler_keyboard, 0x08, IDT_TYPE_INTERRUPT_GATE);
    
    /* Load IDT */
    idt_load();
    
    /* Initialize PIC */
    pic_initialize();
    
    terminal_writeline("IDT initialized successfully!");
}

/* PIC ports */
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

/* PIC initialization commands */
#define ICW1_ICW4       0x01    /* ICW4 (not) needed */
#define ICW1_SINGLE     0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4  0x04    /* Call address interval 4 (8) */
#define ICW1_LEVEL      0x08    /* Level triggered (edge) mode */
#define ICW1_INIT       0x10    /* Initialization - required! */

#define ICW4_8086       0x01    /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO       0x02    /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08    /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C    /* Buffered mode/master */
#define ICW4_SFNM       0x10    /* Special fully nested (not) */

/* Initialize PIC */
void pic_initialize(void) {
    /* Start initialization sequence in cascade mode */
    __asm_outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    __asm_outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    
    /* ICW2: Master PIC vector offset (IRQ 0-7 -> interrupts 32-39) */
    __asm_outb(PIC1_DATA, 32);
    /* ICW2: Slave PIC vector offset (IRQ 8-15 -> interrupts 40-47) */
    __asm_outb(PIC2_DATA, 40);
    
    /* ICW3: Tell Master PIC that there is a slave PIC at IRQ2 (0000 0100) */
    __asm_outb(PIC1_DATA, 4);
    /* ICW3: Tell Slave PIC its cascade identity (0000 0010) */
    __asm_outb(PIC2_DATA, 2);
    
    /* ICW4: Set 8086 mode */
    __asm_outb(PIC1_DATA, ICW4_8086);
    __asm_outb(PIC2_DATA, ICW4_8086);
    
    /* Mask unwanted interrupts */
    /* Master PIC: Enable only timer (IRQ 0) and keyboard (IRQ 1) and cascade (IRQ 2) */
    /* Mask pattern: 11111000 = 0xF8 (bits 0,1,2 = 0 for enabled IRQs) */
    // __asm_outb(PIC1_DATA, 0xF8);
    
    /* Slave PIC: Mask all interrupts for now */
    /* Mask pattern: 11111111 = 0xFF (all IRQs disabled) */
    // __asm_outb(PIC2_DATA, 0xFF);
    
    terminal_writeline("PIC initialized successfully!");
}

/* Enable interrupts */
void enable_interrupts(void) {
    __asm__ volatile("sti");
}

/* Disable interrupts */
void disable_interrupts(void) {
    __asm__ volatile("cli");
}
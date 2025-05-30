#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "../common/types.h"

/* IDT entry structure */
typedef struct {
    uint16_t offset_low;    /* Offset bits 0-15 */
    uint16_t selector;      /* Code segment selector */
    uint8_t  zero;          /* Must be zero */
    uint8_t  type_attr;     /* Type and attributes */
    uint16_t offset_high;   /* Offset bits 16-31 */
} __attribute__((packed)) idt_entry_t;

/* IDT pointer structure */
typedef struct {
    uint16_t limit;         /* Size of IDT - 1 */
    uint32_t base;          /* Base address of IDT */
} __attribute__((packed)) idt_ptr_t;

/* Interrupt types */
#define IDT_TYPE_INTERRUPT_GATE  0x8E
#define IDT_TYPE_TRAP_GATE       0x8F

/* Hardware interrupts (IRQ) */
#define IRQ_TIMER                32
#define IRQ_KEYBOARD             33

/* Function prototypes */
void interrupts_initialize(void);
void pic_initialize(void);
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags);
void enable_interrupts(void);
void disable_interrupts(void);
void pic_display_status(void);

/* Hardware interrupt handlers */
void irq_handler_timer(void);
void irq_handler_keyboard(void);

#endif /* INTERRUPTS_H */
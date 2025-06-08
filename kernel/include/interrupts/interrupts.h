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

/* Exception handlers */
void exception_handler_0(void);   /* Division by zero */
void exception_handler_1(void);   /* Debug */
void exception_handler_2(void);   /* Non-maskable interrupt */
void exception_handler_3(void);   /* Breakpoint */
void exception_handler_4(void);   /* Overflow */
void exception_handler_5(void);   /* Bound range exceeded */
void exception_handler_6(void);   /* Invalid opcode */
void exception_handler_7(void);   /* Device not available */
void exception_handler_8(void);   /* Double fault */
void exception_handler_9(void);   /* Coprocessor segment overrun */
void exception_handler_10(void);  /* Invalid TSS */
void exception_handler_11(void);  /* Segment not present */
void exception_handler_12(void);  /* Stack-segment fault */
void exception_handler_13(void);  /* General protection fault */
void exception_handler_14(void);  /* Page fault */
void exception_handler_15(void);  /* Reserved */
void exception_handler_16(void);  /* x87 FPU floating-point error */
void exception_handler_17(void);  /* Alignment check */
void exception_handler_18(void);  /* Machine check */
void exception_handler_19(void);  /* SIMD floating-point exception */

/* C interrupt handlers */
void c_irq_handler_timer(void);
void c_irq_handler_keyboard(void);
void c_exception_handler(void* ctx);

/* Interrupt statistics and debugging */
void get_interrupt_statistics(uint32_t* timer_count, uint32_t* keyboard_count, uint32_t* spurious_count);
void reset_interrupt_statistics(void);
void display_interrupt_statistics(void);

#endif /* INTERRUPTS_H */
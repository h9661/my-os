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

/* Exception types */
#define IDT_TYPE_INTERRUPT_GATE  0x8E
#define IDT_TYPE_TRAP_GATE       0x8F

/* Interrupt numbers */
#define INT_DIVIDE_ERROR         0
#define INT_DEBUG                1
#define INT_NMI                  2
#define INT_BREAKPOINT           3
#define INT_OVERFLOW             4
#define INT_BOUND_RANGE          5
#define INT_INVALID_OPCODE       6
#define INT_DEVICE_NOT_AVAILABLE 7
#define INT_DOUBLE_FAULT         8
#define INT_INVALID_TSS          10
#define INT_SEGMENT_NOT_PRESENT  11
#define INT_STACK_FAULT          12
#define INT_GENERAL_PROTECTION   13
#define INT_PAGE_FAULT           14
#define INT_RESERVED             15
#define INT_X87_FPU_ERROR        16
#define INT_ALIGNMENT_CHECK      17
#define INT_MACHINE_CHECK        18
#define INT_SIMD_FP_EXCEPTION    19

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

/* Exception handlers */
void exception_handler_0(void);
void exception_handler_1(void);
void exception_handler_2(void);
void exception_handler_3(void);
void exception_handler_4(void);
void exception_handler_5(void);
void exception_handler_6(void);
void exception_handler_7(void);
void exception_handler_8(void);
void exception_handler_9(void);
void exception_handler_10(void);
void exception_handler_11(void);
void exception_handler_12(void);
void exception_handler_13(void);
void exception_handler_14(void);
void exception_handler_15(void);
void exception_handler_16(void);
void exception_handler_17(void);
void exception_handler_18(void);
void exception_handler_19(void);

/* Hardware interrupt handlers */
void irq_handler_timer(void);
void irq_handler_keyboard(void);

#endif /* INTERRUPTS_H */
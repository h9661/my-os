#include "../../include/interrupts/interrupts.h"
#include "../../include/vga/vga.h"
#include "../../include/common/utils.h"
#include "../../include/keyboard/keyboard.h"
#include "../../include/process/process.h"
#include "../../include/timer/pit.h"
#include "../../include/terminal/terminal.h"

/* Interrupt statistics for debugging */
static uint32_t timer_interrupt_count = 0;
static uint32_t keyboard_interrupt_count = 0;
static uint32_t spurious_interrupt_count = 0;

/* Exception names for debugging */
static const char* exception_names[] = {
    "Division by zero",
    "Debug",
    "Non-maskable interrupt", 
    "Breakpoint",
    "Overflow",
    "Bound range exceeded",
    "Invalid opcode",
    "Device not available",
    "Double fault",
    "Coprocessor segment overrun",
    "Invalid TSS",
    "Segment not present",
    "Stack-segment fault",
    "General protection fault",
    "Page fault",
    "Reserved",
    "x87 FPU floating-point error",
    "Alignment check",
    "Machine check",
    "SIMD floating-point exception"
};

/* CPU exception context structure */
typedef struct {
    uint32_t gs, fs, es, ds;                    /* Segment registers */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* General registers (pusha order) */
    uint32_t exception_num, error_code;         /* Exception info */
    uint32_t eip, cs, eflags;                   /* CPU pushed registers */
    uint32_t user_esp, user_ss;                 /* Only if privilege change occurred */
} __attribute__((packed)) exception_context_t;

/* Helper function to send EOI signal to PIC */
static inline void send_eoi(uint32_t irq_num) {
    /* Validate IRQ number */
    if (irq_num < 32 || irq_num > 47) {
        return; /* Invalid IRQ number */
    }
    
    /* For IRQs 8-15 (interrupts 40-47), send EOI to both slave and master PIC */
    if (irq_num >= 40) {
        /* Send EOI to slave PIC */
        __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0xA0));
    }
    /* Send EOI to master PIC - always required for any IRQ */
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0x20));
}

/* Check for spurious interrupts */
static bool is_spurious_irq(uint32_t irq_num) {
    uint8_t irr;
    
    if (irq_num == 39) { /* Spurious IRQ 7 on master PIC */
        /* Read IRR (In-Service Register) of master PIC */
        __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x0B), "Nd"((uint16_t)0x20));
        __asm__ volatile("inb %1, %0" : "=a"(irr) : "Nd"((uint16_t)0x20));
        return !(irr & 0x80); /* Check if IRQ 7 bit is set */
    } else if (irq_num == 47) { /* Spurious IRQ 15 on slave PIC */
        /* Read IRR of slave PIC */
        __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x0B), "Nd"((uint16_t)0xA0));
        __asm__ volatile("inb %1, %0" : "=a"(irr) : "Nd"((uint16_t)0xA0));
        if (!(irr & 0x80)) { /* IRQ 15 not set, it's spurious */
            /* Still need to send EOI to master PIC for spurious slave interrupts */
            __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0x20));
            return true;
        }
    }
    return false;
}

/* Timer interrupt handler */
void c_irq_handler_timer(void) {
    /* Check for spurious interrupt */
    if (is_spurious_irq(32)) {
        spurious_interrupt_count++;
        return;
    }
    
    /* Increment statistics */
    timer_interrupt_count++;
    
    /* Update timer tick count - timer_tick() already calls scheduler_tick() */
    timer_tick();
    
    /* Send EOI to PIC */
    send_eoi(32);
}

/* Keyboard interrupt handler */
void c_irq_handler_keyboard(void) {
    /* Check for spurious interrupt */
    if (is_spurious_irq(33)) {
        spurious_interrupt_count++;
        return;
    }
    
    /* Increment statistics */
    keyboard_interrupt_count++;
    
    /* Call keyboard handler function */
    keyboard_handler();
    
    /* Send EOI to PIC */
    send_eoi(33);
}

/* Exception handler */
void c_exception_handler(void* context) {
    exception_context_t* ctx = (exception_context_t*)context;
    /* Disable interrupts during exception handling */
    __asm__ volatile("cli");
    
    /* Clear screen and display exception information */
    terminal_clear();
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
    
    terminal_writestring("=== KERNEL PANIC: CPU EXCEPTION ===\n");
    
    /* Display exception name */
    if (ctx->exception_num < 20) {
        terminal_writestring("Exception: ");
        terminal_writestring(exception_names[ctx->exception_num]);
    } else {
        terminal_writestring("Unknown Exception");
    }
    terminal_writestring(" (");
    
    /* Display exception number */
    char num_str[16];
    int_to_string(ctx->exception_num, num_str);
    terminal_writestring(num_str);
    terminal_writestring(")\n");
    
    /* Display error code if present */
    if (ctx->error_code != 0) {
        terminal_writestring("Error Code: 0x");
        int_to_hex(ctx->error_code, num_str);
        terminal_writestring(num_str);
        terminal_writestring("\n");
    }
    
    /* Display register values */
    terminal_writestring("\nRegisters:\n");
    terminal_writestring("EAX=0x"); int_to_hex(ctx->eax, num_str); terminal_writestring(num_str);
    terminal_writestring(" EBX=0x"); int_to_hex(ctx->ebx, num_str); terminal_writestring(num_str);
    terminal_writestring(" ECX=0x"); int_to_hex(ctx->ecx, num_str); terminal_writestring(num_str);
    terminal_writestring(" EDX=0x"); int_to_hex(ctx->edx, num_str); terminal_writeline(num_str);
    
    terminal_writestring("ESI=0x"); int_to_hex(ctx->esi, num_str); terminal_writestring(num_str);
    terminal_writestring(" EDI=0x"); int_to_hex(ctx->edi, num_str); terminal_writestring(num_str);
    terminal_writestring(" EBP=0x"); int_to_hex(ctx->ebp, num_str); terminal_writestring(num_str);
    terminal_writestring(" ESP=0x"); int_to_hex(ctx->esp, num_str); terminal_writeline(num_str);
    
    terminal_writestring("EIP=0x"); int_to_hex(ctx->eip, num_str); terminal_writestring(num_str);
    terminal_writestring(" CS=0x"); int_to_hex(ctx->cs, num_str); terminal_writestring(num_str);
    terminal_writestring(" EFLAGS=0x"); int_to_hex(ctx->eflags, num_str); terminal_writeline(num_str);
    
    terminal_writestring("DS=0x"); int_to_hex(ctx->ds, num_str); terminal_writestring(num_str);
    terminal_writestring(" ES=0x"); int_to_hex(ctx->es, num_str); terminal_writestring(num_str);
    terminal_writestring(" FS=0x"); int_to_hex(ctx->fs, num_str); terminal_writestring(num_str);
    terminal_writestring(" GS=0x"); int_to_hex(ctx->gs, num_str); terminal_writeline(num_str);
    
    /* Display additional info for specific exceptions */
    if (ctx->exception_num == 14) { /* Page fault */
        uint32_t fault_addr;
        __asm__ volatile("mov %%cr2, %0" : "=r"(fault_addr));
        terminal_writestring("\nPage Fault Address: 0x");
        int_to_hex(fault_addr, num_str);
        terminal_writeline(num_str);
        
        terminal_writestring("Fault Type: ");
        if (ctx->error_code & 0x1) terminal_writestring("Protection violation ");
        else terminal_writestring("Page not present ");
        if (ctx->error_code & 0x2) terminal_writestring("(Write) ");
        else terminal_writestring("(Read) ");
        if (ctx->error_code & 0x4) terminal_writestring("(User mode)");
        else terminal_writestring("(Kernel mode)");
        terminal_writeline("");
    }
    
    /* Display current process information if available */
    process_t* current = get_current_process();
    if (current) {
        terminal_writestring("\nCurrent Process: PID ");
        int_to_string(current->pid, num_str);
        terminal_writeline(num_str);
    }
    
    terminal_writeline("\nSystem halted. Please reboot.");
    
    /* Halt the system */
    while (1) {
        __asm__ volatile("cli; hlt");
    }
}

/* Get interrupt statistics for debugging */
void get_interrupt_statistics(uint32_t* timer_count, uint32_t* keyboard_count, uint32_t* spurious_count) {
    if (timer_count) *timer_count = timer_interrupt_count;
    if (keyboard_count) *keyboard_count = keyboard_interrupt_count;
    if (spurious_count) *spurious_count = spurious_interrupt_count;
}

/* Reset interrupt statistics */
void reset_interrupt_statistics(void) {
    timer_interrupt_count = 0;
    keyboard_interrupt_count = 0;
    spurious_interrupt_count = 0;
}

/* Display interrupt statistics for debugging */
void display_interrupt_statistics(void) {
    uint32_t timer_count, keyboard_count, spurious_count;
    char count_str[16];
    
    get_interrupt_statistics(&timer_count, &keyboard_count, &spurious_count);
    
    terminal_writeline("=== Interrupt Statistics ===");
    
    terminal_writestring("Timer interrupts: ");
    int_to_string(timer_count, count_str);
    terminal_writeline(count_str);
    
    terminal_writestring("Keyboard interrupts: ");
    int_to_string(keyboard_count, count_str);
    terminal_writeline(count_str);
    
    terminal_writestring("Spurious interrupts: ");
    int_to_string(spurious_count, count_str);
    terminal_writeline(count_str);
    
    terminal_writeline("============================");
}
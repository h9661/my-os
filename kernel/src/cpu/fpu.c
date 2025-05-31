#include "../../include/cpu/fpu.h"
#include "../../include/cpu/cpu.h"
#include "../../include/vga/vga.h"
#include "../../include/terminal/terminal.h"
#include "../../include/common/utils.h"

/* Check if FPU is available */
bool fpu_is_available(void) {
    cpu_info_t cpu_info;
    if (cpu_get_info(&cpu_info)) {
        return cpu_info.features.fpu;
    }
    return false;
}

/* Initialize FPU */
void fpu_initialize(void) {
    /* First, disable FPU to prevent spurious exceptions during boot */
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    
    /* Set EM bit (enable emulation) to disable FPU initially */
    cr0 |= CR0_EM_BIT;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
    
    if (!fpu_is_available()) {
        terminal_writeline("FPU not available - using emulation");
        return;
    }
    
    terminal_writeline("Initializing FPU...");
    
    /* Clear EM bit to enable FPU */
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~CR0_EM_BIT;
    
    /* IMPORTANT: Do NOT set NE bit to prevent FPU exceptions */
    /* cr0 |= CR0_NE_BIT; */ /* Commented out to disable native FPU error reporting */
    
    /* Set MP bit (bit 1) - monitor coprocessor */
    cr0 |= CR0_MP_BIT;
    
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
    
    /* Initialize FPU with FINIT instruction */
    __asm__ volatile("finit");
    
    /* Clear any pending exceptions */
    __asm__ volatile("fclex");
    
    /* Set control word to mask ALL exceptions to prevent interrupt 16 */
    uint16_t control_word = FPU_CW_PRECISION_64 | FPU_CW_ROUND_NEAREST |
                           FPU_CW_PRECISION_EXC | FPU_CW_UNDERFLOW_EXC |
                           FPU_CW_OVERFLOW_EXC | FPU_CW_ZERODIV_EXC |
                           FPU_CW_DENORMAL_EXC | FPU_CW_INVALID_EXC;
    fpu_set_control_word(control_word);
    
    /* Verify that exceptions are masked */
    uint16_t verify_cw = fpu_get_control_word();
    if ((verify_cw & 0x3F) != 0x3F) {
        terminal_writeline("Warning: FPU exceptions may not be fully masked");
    }
    
    terminal_writeline("FPU initialization complete (exceptions disabled)");
}

/* Enable FPU */
void fpu_enable(void) {
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~CR0_EM_BIT;  /* Clear EM bit */
    cr0 &= ~CR0_TS_BIT;  /* Clear TS bit */
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}

/* Disable FPU */
void fpu_disable(void) {
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= CR0_EM_BIT;   /* Set EM bit */
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}

/* Clear FPU exceptions */
void fpu_clear_exceptions(void) {
    __asm__ volatile("fclex");
}

/* Get FPU control word */
uint16_t fpu_get_control_word(void) {
    uint16_t cw;
    __asm__ volatile("fstcw %0" : "=m"(cw));
    return cw;
}

/* Set FPU control word */
void fpu_set_control_word(uint16_t cw) {
    __asm__ volatile("fldcw %0" : : "m"(cw));
}

/* Get FPU status word */
uint16_t fpu_get_status_word(void) {
    uint16_t sw;
    __asm__ volatile("fstsw %0" : "=a"(sw));
    return sw;
}

/* Save FPU state */
void fpu_save_state(void* state) {
    __asm__ volatile("fsave %0" : "=m"(*(char*)state));
}

/* Restore FPU state */
void fpu_restore_state(void* state) {
    __asm__ volatile("frstor %0" : : "m"(*(char*)state));
}

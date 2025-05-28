#include "../../include/cpu/cpu.h"
#include "../../include/vga/vga.h"
#include "../../include/common/utils.h"

/* Execute CPUID instruction */
void cpuid(uint32_t leaf, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
    __asm__ volatile("cpuid"
                    : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
                    : "0" (leaf));
}

/* Detect if CPUID instruction is available */
bool cpu_detect_cpuid(void) {
    uint32_t eflags_before, eflags_after;
    
    /* Get current EFLAGS */
    __asm__ volatile(
        "pushfl\n\t"
        "popl %0"
        : "=r" (eflags_before)
    );
    
    /* Try to flip the ID bit (bit 21) */
    __asm__ volatile(
        "pushfl\n\t"
        "popl %%eax\n\t"
        "xorl $0x200000, %%eax\n\t"
        "pushl %%eax\n\t"
        "popfl\n\t"
        "pushfl\n\t"
        "popl %0"
        : "=r" (eflags_after)
        :
        : "eax"
    );
    
    /* Restore original EFLAGS */
    __asm__ volatile(
        "pushl %0\n\t"
        "popfl"
        :
        : "r" (eflags_before)
    );
    
    /* If ID bit could be flipped, CPUID is available */
    return (eflags_before != eflags_after);
}

/* Get CPU vendor string */
void cpu_get_vendor(char* vendor) {
    uint32_t eax, ebx, ecx, edx;
    
    /* Execute CPUID instruction with EAX=0 */
    cpuid(0, &eax, &ebx, &ecx, &edx);
    
    /* Copy vendor ID string (12 bytes in EBX, EDX, ECX order) */
    ((uint32_t*)vendor)[0] = ebx;
    ((uint32_t*)vendor)[1] = edx;
    ((uint32_t*)vendor)[2] = ecx;
    vendor[12] = '\0'; /* Null-terminate string */
}

/* Get comprehensive CPU information */
bool cpu_get_info(cpu_info_t* info) {
    if (!info) {
        return false;
    }
    
    /* Initialize structure */
    memset(info, 0, sizeof(cpu_info_t));
    
    /* Check if CPUID is available */
    info->cpuid_available = cpu_detect_cpuid();
    if (!info->cpuid_available) {
        return false;
    }
    
    /* Get maximum CPUID level and vendor string */
    uint32_t eax, ebx, ecx, edx;
    cpuid(0, &eax, &ebx, &ecx, &edx);
    
    info->max_cpuid = eax;
    
    /* Copy vendor ID string */
    ((uint32_t*)info->vendor)[0] = ebx;
    ((uint32_t*)info->vendor)[1] = edx;
    ((uint32_t*)info->vendor)[2] = ecx;
    info->vendor[12] = '\0';
    
    return true;
}

/* Print CPU information */
void cpu_print_info(const cpu_info_t* info) {
    if (!info) {
        terminal_writeline("CPU information not available!");
        return;
    }
    
    if (!info->cpuid_available) {
        terminal_writeline("CPUID instruction not supported!");
        return;
    }
    
    char num_str[12];
    
    terminal_writestring("CPU Information:\n");
    
    /* Print vendor */
    terminal_writestring("  Vendor: ");
    terminal_writeline(info->vendor);
    
    /* Print maximum CPUID level */
    terminal_writestring("  Max CPUID Level: ");
    int_to_string(info->max_cpuid, num_str);
    terminal_writeline(num_str);
    
    /* Print CPUID availability */
    terminal_writestring("  CPUID Available: ");
    terminal_writeline(info->cpuid_available ? "Yes" : "No");
}

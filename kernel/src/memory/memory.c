#include "../../include/memory/memory.h"
#include "../../include/vga/vga.h"
#include "../../include/terminal/terminal.h"
#include "../../include/common/utils.h"

/* Read from CMOS to detect memory size */
uint32_t memory_detect_cmos(void) {
    uint32_t total_memory = 0;
    uint8_t low_byte, high_byte;
    
    /* Read extended memory size from CMOS */
    /* CMOS 0x17-0x18 contains extended memory size in KB */
    
    /* Read low byte from CMOS register 0x17 */
    __asm__ volatile(
        "movb $0x17, %%al\n\t"
        "outb %%al, $0x70\n\t"
        "inb $0x71, %%al"
        : "=a"(low_byte)
        :
        : "memory"
    );
    
    /* Read high byte from CMOS register 0x18 */
    __asm__ volatile(
        "movb $0x18, %%al\n\t"
        "outb %%al, $0x70\n\t"
        "inb $0x71, %%al"
        : "=a"(high_byte)
        :
        : "memory"
    );
    
    total_memory = (high_byte << 8) | low_byte; /* Combine bytes */
    total_memory += 1024; /* Add base 1MB */
    
    return total_memory; /* Return in KB */
}

/* Detect memory size by probing */
uint32_t memory_detect_probe(void) {
    volatile uint32_t *memory_ptr;
    uint32_t memory_mb = 0;
    uint32_t test_value = 0x12345678;
    uint32_t backup_value;
    
    /* Start probing from 1MB (0x100000) in 1MB increments */
    for (uint32_t mb = 1; mb < 4096; mb++) { /* Test up to 4GB */
        memory_ptr = (volatile uint32_t *)(MEMORY_BASE_1MB + (mb * 1024 * 1024));
        
        /* Backup original value */
        backup_value = *memory_ptr;
        
        /* Write test value */
        *memory_ptr = test_value;
        
        /* Check if we can read back the test value */
        if (*memory_ptr != test_value) {
            /* Memory not accessible, stop here */
            break;
        }
        
        /* Restore original value */
        *memory_ptr = backup_value;
        
        memory_mb = mb + 1; /* +1 because we started from 1MB */
    }
    
    return memory_mb;
}

/* Get comprehensive memory information */
bool memory_get_info(memory_info_t* info) {
    if (!info) {
        return false;
    }
    
    /* Initialize structure */
    memset(info, 0, sizeof(memory_info_t));
    
    /* Try CMOS method first */
    uint32_t cmos_kb = memory_detect_cmos();
    if (cmos_kb > 1024) { /* At least 1MB should be detected */
        info->total_kb = cmos_kb;
        info->total_mb = cmos_kb / 1024;
        info->method = MEMORY_DETECT_CMOS;
        info->valid = true;
        info->available_kb = cmos_kb - 1024; /* Reserve 1MB for kernel */
        info->available_mb = info->available_kb / 1024;
        return true;
    }
    
    /* Fall back to probing method */
    uint32_t probe_mb = memory_detect_probe();
    if (probe_mb > 0) {
        info->total_mb = probe_mb;
        info->total_kb = probe_mb * 1024;
        info->method = MEMORY_DETECT_PROBE;
        info->valid = true;
        info->available_mb = probe_mb - 1; /* Reserve 1MB for kernel */
        info->available_kb = info->available_mb * 1024;
        return true;
    }
    
    /* Memory detection failed */
    info->valid = false;
    return false;
}

/* Print memory information */
void memory_print_info(const memory_info_t* info) {
    if (!info || !info->valid) {
        terminal_writestring("Memory detection failed!\n");
        return;
    }
    
    char num_str[12];
    
    terminal_writestring("Memory Information:\n");
    
    /* Print detection method */
    terminal_writestring("  Detection Method: ");
    switch (info->method) {
        case MEMORY_DETECT_CMOS:
            terminal_writestring("CMOS\n");
            break;
        case MEMORY_DETECT_PROBE:
            terminal_writestring("Probing\n");
            break;
        case MEMORY_DETECT_E820:
            terminal_writestring("E820\n");
            break;
        default:
            terminal_writestring("Unknown\n");
            break;
    }
    
    /* Print total memory */
    terminal_writestring("  Total Memory: ");
    int_to_string(info->total_kb, num_str);
    terminal_writestring(num_str);
    terminal_writestring(" KB (");
    int_to_string(info->total_mb, num_str);
    terminal_writestring(num_str);
    terminal_writestring(" MB)\n");
    
    /* Print available memory */
    terminal_writestring("  Available Memory: ");
    int_to_string(info->available_kb, num_str);
    terminal_writestring(num_str);
    terminal_writestring(" KB (");
    int_to_string(info->available_mb, num_str);
    terminal_writestring(num_str);
    terminal_writestring(" MB)\n");
}

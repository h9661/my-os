#include "../../include/cpu/cpu.h"
#include "../../include/vga/vga.h"
#include "../../include/terminal/terminal.h"
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

/* Get CPU brand string */
void cpu_get_brand(char* brand) {
    uint32_t eax, ebx, ecx, edx;
    
    /* Check if extended CPUID is available */
    cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    if (eax < 0x80000004) {
        strcpy(brand, "Unknown");
        return;
    }
    
    /* Get brand string from CPUID 0x80000002-0x80000004 */
    cpuid(0x80000002, &eax, &ebx, &ecx, &edx);
    ((uint32_t*)brand)[0] = eax;
    ((uint32_t*)brand)[1] = ebx;
    ((uint32_t*)brand)[2] = ecx;
    ((uint32_t*)brand)[3] = edx;
    
    cpuid(0x80000003, &eax, &ebx, &ecx, &edx);
    ((uint32_t*)brand)[4] = eax;
    ((uint32_t*)brand)[5] = ebx;
    ((uint32_t*)brand)[6] = ecx;
    ((uint32_t*)brand)[7] = edx;
    
    cpuid(0x80000004, &eax, &ebx, &ecx, &edx);
    ((uint32_t*)brand)[8] = eax;
    ((uint32_t*)brand)[9] = ebx;
    ((uint32_t*)brand)[10] = ecx;
    ((uint32_t*)brand)[11] = edx;
    
    brand[48] = '\0'; /* Null-terminate string */
    
    /* Trim leading spaces */
    char* start = brand;
    while (*start == ' ') start++;
    if (start != brand) {
        strcpy(brand, start);
    }
}

/* Detect CPU features */
void cpu_detect_features(cpu_features_t* features) {
    uint32_t eax, ebx, ecx, edx;
    
    memset(features, 0, sizeof(cpu_features_t));
    
    /* Get feature information from CPUID leaf 1 */
    cpuid(1, &eax, &ebx, &ecx, &edx);
    
    /* EDX features (standard features) */
    features->fpu = (edx & (1 << 0)) != 0;
    features->vme = (edx & (1 << 1)) != 0;
    features->de = (edx & (1 << 2)) != 0;
    features->pse = (edx & (1 << 3)) != 0;
    features->tsc = (edx & (1 << 4)) != 0;
    features->msr = (edx & (1 << 5)) != 0;
    features->pae = (edx & (1 << 6)) != 0;
    features->mce = (edx & (1 << 7)) != 0;
    features->cx8 = (edx & (1 << 8)) != 0;
    features->apic = (edx & (1 << 9)) != 0;
    features->sep = (edx & (1 << 11)) != 0;
    features->mtrr = (edx & (1 << 12)) != 0;
    features->pge = (edx & (1 << 13)) != 0;
    features->mca = (edx & (1 << 14)) != 0;
    features->cmov = (edx & (1 << 15)) != 0;
    features->pat = (edx & (1 << 16)) != 0;
    features->pse36 = (edx & (1 << 17)) != 0;
    features->psn = (edx & (1 << 18)) != 0;
    features->clfsh = (edx & (1 << 19)) != 0;
    features->ds = (edx & (1 << 21)) != 0;
    features->acpi = (edx & (1 << 22)) != 0;
    features->mmx = (edx & (1 << 23)) != 0;
    features->fxsr = (edx & (1 << 24)) != 0;
    features->sse = (edx & (1 << 25)) != 0;
    features->sse2 = (edx & (1 << 26)) != 0;
    features->ss = (edx & (1 << 27)) != 0;
    features->htt = (edx & (1 << 28)) != 0;
    features->tm = (edx & (1 << 29)) != 0;
    features->ia64 = (edx & (1 << 30)) != 0;
    features->pbe = (edx & (1 << 31)) != 0;
    
    /* ECX features (extended features) */
    features->sse3 = (ecx & (1 << 0)) != 0;
    features->pclmulqdq = (ecx & (1 << 1)) != 0;
    features->dtes64 = (ecx & (1 << 2)) != 0;
    features->monitor = (ecx & (1 << 3)) != 0;
    features->ds_cpl = (ecx & (1 << 4)) != 0;
    features->vmx = (ecx & (1 << 5)) != 0;
    features->smx = (ecx & (1 << 6)) != 0;
    features->est = (ecx & (1 << 7)) != 0;
    features->tm2 = (ecx & (1 << 8)) != 0;
    features->ssse3 = (ecx & (1 << 9)) != 0;
    features->cnxt_id = (ecx & (1 << 10)) != 0;
    features->sdbg = (ecx & (1 << 11)) != 0;
    features->fma = (ecx & (1 << 12)) != 0;
    features->cx16 = (ecx & (1 << 13)) != 0;
    features->xtpr = (ecx & (1 << 14)) != 0;
    features->pdcm = (ecx & (1 << 15)) != 0;
    features->pcid = (ecx & (1 << 17)) != 0;
    features->dca = (ecx & (1 << 18)) != 0;
    features->sse4_1 = (ecx & (1 << 19)) != 0;
    features->sse4_2 = (ecx & (1 << 20)) != 0;
    features->x2apic = (ecx & (1 << 21)) != 0;
    features->movbe = (ecx & (1 << 22)) != 0;
    features->popcnt = (ecx & (1 << 23)) != 0;
    features->tsc_deadline = (ecx & (1 << 24)) != 0;
    features->aes = (ecx & (1 << 25)) != 0;
    features->xsave = (ecx & (1 << 26)) != 0;
    features->osxsave = (ecx & (1 << 27)) != 0;
    features->avx = (ecx & (1 << 28)) != 0;
    features->f16c = (ecx & (1 << 29)) != 0;
    features->rdrand = (ecx & (1 << 30)) != 0;
    features->hypervisor = (ecx & (1 << 31)) != 0;
}

/* Detect cache information */
void cpu_detect_cache_info(cpu_cache_info_t* cache) {
    uint32_t eax, ebx, ecx, edx;
    
    memset(cache, 0, sizeof(cpu_cache_info_t));
    
    /* Try to get cache info from CPUID leaf 4 (Intel) */
    cpuid(4, &eax, &ebx, &ecx, &edx);
    if (eax != 0) {
        /* Intel deterministic cache parameters */
        uint32_t cache_type = eax & 0x1F;
        uint32_t cache_level = (eax >> 5) & 0x7;
        uint32_t ways = ((ebx >> 22) & 0x3FF) + 1;
        uint32_t partitions = ((ebx >> 12) & 0x3FF) + 1;
        uint32_t line_size = (ebx & 0xFFF) + 1;
        uint32_t sets = ecx + 1;
        uint32_t cache_size = ways * partitions * line_size * sets / 1024;
        
        cache->cache_line_size = line_size;
        
        if (cache_level == 1 && cache_type == 1) {
            cache->l1_data_cache_size = cache_size;
        } else if (cache_level == 1 && cache_type == 2) {
            cache->l1_instruction_cache_size = cache_size;
        } else if (cache_level == 2) {
            cache->l2_cache_size = cache_size;
        } else if (cache_level == 3) {
            cache->l3_cache_size = cache_size;
        }
    }
    
    /* Try AMD cache info from extended CPUID */
    cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    if (eax >= 0x80000005) {
        cpuid(0x80000005, &eax, &ebx, &ecx, &edx);
        cache->l1_data_cache_size = (ecx >> 24) & 0xFF;
        cache->l1_instruction_cache_size = (edx >> 24) & 0xFF;
        if (cache->cache_line_size == 0) {
            cache->cache_line_size = ecx & 0xFF;
        }
    }
    
    if (eax >= 0x80000006) {
        cpuid(0x80000006, &eax, &ebx, &ecx, &edx);
        cache->l2_cache_size = (ecx >> 16) & 0xFFFF;
        cache->l3_cache_size = ((edx >> 18) & 0x3FFF) * 512; /* In 512KB units */
    }
}

/* Detect CPU topology (cores and threads) */
void cpu_detect_topology(cpu_info_t* info) {
    uint32_t eax, ebx, ecx, edx;
    
    /* Initialize with defaults */
    info->logical_processors = 1;
    info->physical_cores = 1;
    info->threads_per_core = 1;
    
    /* Get basic processor info */
    cpuid(1, &eax, &ebx, &ecx, &edx);
    
    /* Check if HTT is supported */
    if (edx & (1 << 28)) {
        info->logical_processors = (ebx >> 16) & 0xFF;
    }
    
    /* Try to get extended topology information */
    cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    if (eax >= 0x80000008) {
        cpuid(0x80000008, &eax, &ebx, &ecx, &edx);
        info->physical_cores = (ecx & 0xFF) + 1;
        if (info->physical_cores > 0) {
            info->threads_per_core = info->logical_processors / info->physical_cores;
        }
    }
    
    /* If extended topology didn't work, try Intel's method */
    if (info->physical_cores == 1) {
        cpuid(4, &eax, &ebx, &ecx, &edx);
        if (eax != 0) {
            info->physical_cores = ((eax >> 26) & 0x3F) + 1;
            if (info->physical_cores > 0) {
                info->threads_per_core = info->logical_processors / info->physical_cores;
            }
        }
    }
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
    
    /* Get maximum extended CPUID level */
    cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    info->max_extended_cpuid = eax;
    
    /* Get CPU brand string */
    cpu_get_brand(info->brand);
    
    /* Get CPU signature and family/model/stepping */
    if (info->max_cpuid >= 1) {
        cpuid(1, &eax, &ebx, &ecx, &edx);
        info->signature = eax;
        info->stepping = eax & 0xF;
        info->model = (eax >> 4) & 0xF;
        info->family = (eax >> 8) & 0xF;
        
        /* Extended family and model for newer CPUs */
        if (info->family == 0xF) {
            info->family += (eax >> 20) & 0xFF;
        }
        if (info->family == 0xF || info->family == 0x6) {
            info->model += ((eax >> 16) & 0xF) << 4;
        }
    }
    
    /* Detect CPU features */
    cpu_detect_features(&info->features);
    
    /* Detect cache information */
    cpu_detect_cache_info(&info->cache);
    
    /* Detect CPU topology */
    cpu_detect_topology(info);
    
    /* Try to get frequency information (if available) */
    if (info->max_cpuid >= 0x15) {
        cpuid(0x15, &eax, &ebx, &ecx, &edx);
        if (eax != 0 && ebx != 0 && ecx != 0) {
            info->base_frequency = (ecx * ebx) / eax / 1000000; /* Convert to MHz */
        }
    }
    
    if (info->max_cpuid >= 0x16) {
        cpuid(0x16, &eax, &ebx, &ecx, &edx);
        info->base_frequency = eax;
        info->max_frequency = ebx;
        info->bus_frequency = ecx;
    }
    
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
    
    char num_str[16];
    
    terminal_writeline("========== CPU Information ==========");
    
    /* Print vendor and brand */
    terminal_writestring("Vendor: ");
    terminal_writeline(info->vendor);
    
    terminal_writestring("Brand: ");
    terminal_writeline(info->brand);
    
    /* Print CPU identification */
    terminal_writestring("Family: ");
    int_to_string(info->family, num_str);
    terminal_writeline(num_str);
    
    terminal_writestring("Model: ");
    int_to_string(info->model, num_str);
    terminal_writeline(num_str);
    
    terminal_writestring("Stepping: ");
    int_to_string(info->stepping, num_str);
    terminal_writeline(num_str);
    
    /* Print core and thread information */
    terminal_writeline("\n--- Core/Thread Information ---");
    
    terminal_writestring("Physical Cores: ");
    int_to_string(info->physical_cores, num_str);
    terminal_writeline(num_str);
    
    terminal_writestring("Logical Processors: ");
    int_to_string(info->logical_processors, num_str);
    terminal_writeline(num_str);
    
    terminal_writestring("Threads per Core: ");
    int_to_string(info->threads_per_core, num_str);
    terminal_writeline(num_str);
    
    /* Print cache information */
    terminal_writeline("\n--- Cache Information ---");
    
    if (info->cache.l1_data_cache_size > 0) {
        terminal_writestring("L1 Data Cache: ");
        int_to_string(info->cache.l1_data_cache_size, num_str);
        terminal_writestring(num_str);
        terminal_writeline(" KB");
    }
    else {
        terminal_writeline("L1 Data Cache: Not Available");
    }
    
    if (info->cache.l1_instruction_cache_size > 0) {
        terminal_writestring("L1 Instruction Cache: ");
        int_to_string(info->cache.l1_instruction_cache_size, num_str);
        terminal_writestring(num_str);
        terminal_writeline(" KB");
    }
    else {
        terminal_writeline("L1 Instruction Cache: Not Available");
    }
    
    if (info->cache.l2_cache_size > 0) {
        terminal_writestring("L2 Cache: ");
        int_to_string(info->cache.l2_cache_size, num_str);
        terminal_writestring(num_str);
        terminal_writeline(" KB");
    }
    else {
        terminal_writeline("L2 Cache: Not Available");
    }
    
    if (info->cache.l3_cache_size > 0) {
        terminal_writestring("L3 Cache: ");
        int_to_string(info->cache.l3_cache_size, num_str);
        terminal_writestring(num_str);
        terminal_writeline(" KB");
    }
    else {
        terminal_writeline("L3 Cache: Not Available");
    }
    
    if (info->cache.cache_line_size > 0) {
        terminal_writestring("Cache Line Size: ");
        int_to_string(info->cache.cache_line_size, num_str);
        terminal_writestring(num_str);
        terminal_writeline(" bytes");
    }
    else {
        terminal_writeline("Cache Line Size: Not Available");
    }
    
    /* Print frequency information */
    if (info->base_frequency > 0 || info->max_frequency > 0) {
        terminal_writeline("\n--- Frequency Information ---");
        
        if (info->base_frequency > 0) {
            terminal_writestring("Base Frequency: ");
            int_to_string(info->base_frequency, num_str);
            terminal_writestring(num_str);
            terminal_writeline(" MHz");
        }
        
        if (info->max_frequency > 0) {
            terminal_writestring("Max Frequency: ");
            int_to_string(info->max_frequency, num_str);
            terminal_writestring(num_str);
            terminal_writeline(" MHz");
        }
        
        if (info->bus_frequency > 0) {
            terminal_writestring("Bus Frequency: ");
            int_to_string(info->bus_frequency, num_str);
            terminal_writestring(num_str);
            terminal_writeline(" MHz");
        }
    }
    
    /* Print CPU features */
    terminal_writeline("\n--- CPU Features ---");
    
    if (info->features.fpu) terminal_writeline("FPU: Yes");
    if (info->features.tsc) terminal_writeline("TSC: Yes");
    if (info->features.msr) terminal_writeline("MSR: Yes");
    if (info->features.pae) terminal_writeline("PAE: Yes");
    if (info->features.apic) terminal_writeline("APIC: Yes");
    if (info->features.mmx) terminal_writeline("MMX: Yes");
    if (info->features.sse) terminal_writeline("SSE: Yes");
    if (info->features.sse2) terminal_writeline("SSE2: Yes");
    if (info->features.sse3) terminal_writeline("SSE3: Yes");
    if (info->features.ssse3) terminal_writeline("SSSE3: Yes");
    if (info->features.sse4_1) terminal_writeline("SSE4.1: Yes");
    if (info->features.sse4_2) terminal_writeline("SSE4.2: Yes");
    if (info->features.avx) terminal_writeline("AVX: Yes");
    if (info->features.aes) terminal_writeline("AES: Yes");
    if (info->features.htt) terminal_writeline("Hyper-Threading: Yes");
    if (info->features.vmx) terminal_writeline("VT-x: Yes");
    if (info->features.rdrand) terminal_writeline("RDRAND: Yes");
    if (info->features.hypervisor) terminal_writeline("Hypervisor: Yes");
    
    /* Print CPUID levels */
    terminal_writeline("\n--- CPUID Information ---");
    
    terminal_writestring("Max CPUID Level: ");
    int_to_string(info->max_cpuid, num_str);
    terminal_writeline(num_str);
    
    terminal_writestring("Max Extended CPUID Level: 0x");
    /* Simple hex conversion for extended CPUID */
    if (info->max_extended_cpuid >= 0x80000000) {
        int_to_string(info->max_extended_cpuid - 0x80000000, num_str);
        terminal_writestring("8000");
        if (info->max_extended_cpuid - 0x80000000 < 16) {
            terminal_writestring("000");
        } else if (info->max_extended_cpuid - 0x80000000 < 256) {
            terminal_writestring("00");
        } else if (info->max_extended_cpuid - 0x80000000 < 4096) {
            terminal_writestring("0");
        }
        terminal_writeline(num_str);
    } else {
        terminal_writeline("Not Available");
    }
    
    terminal_writeline("=====================================");
}

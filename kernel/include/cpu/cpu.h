#ifndef CPU_H
#define CPU_H

#include "../common/types.h"

/* CPU vendor string length */
#define CPU_VENDOR_STRING_LEN 13

/* CPU information structure */
typedef struct {
    char vendor[CPU_VENDOR_STRING_LEN];  /* CPU vendor string */
    uint32_t max_cpuid;                  /* Maximum CPUID level */
    bool cpuid_available;                /* Whether CPUID is available */
} cpu_info_t;

/* CPU detection functions */
bool cpu_detect_cpuid(void);
void cpu_get_vendor(char* vendor);
bool cpu_get_info(cpu_info_t* info);
void cpu_print_info(const cpu_info_t* info);

/* CPUID instruction wrapper */
void cpuid(uint32_t leaf, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx);

#endif /* CPU_H */

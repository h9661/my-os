#ifndef CPU_H
#define CPU_H

#include "../common/types.h"

/* CPU vendor string length */
#define CPU_VENDOR_STRING_LEN 13
#define CPU_BRAND_STRING_LEN 49

/* CPU feature flags */
typedef struct {
    bool fpu;           /* Floating Point Unit */
    bool vme;           /* Virtual Mode Extension */
    bool de;            /* Debugging Extension */
    bool pse;           /* Page Size Extension */
    bool tsc;           /* Time Stamp Counter */
    bool msr;           /* Model Specific Registers */
    bool pae;           /* Physical Address Extension */
    bool mce;           /* Machine Check Exception */
    bool cx8;           /* CMPXCHG8 instruction */
    bool apic;          /* Advanced Programmable Interrupt Controller */
    bool sep;           /* Fast System Call */
    bool mtrr;          /* Memory Type Range Registers */
    bool pge;           /* Page Global Enable */
    bool mca;           /* Machine Check Architecture */
    bool cmov;          /* Conditional Move */
    bool pat;           /* Page Attribute Table */
    bool pse36;         /* 36-bit Page Size Extension */
    bool psn;           /* Processor Serial Number */
    bool clfsh;         /* Cache Line Flush */
    bool ds;            /* Debug Store */
    bool acpi;          /* ACPI support */
    bool mmx;           /* MMX Technology */
    bool fxsr;          /* Fast floating point save/restore */
    bool sse;           /* Streaming SIMD Extensions */
    bool sse2;          /* Streaming SIMD Extensions 2 */
    bool ss;            /* Self Snoop */
    bool htt;           /* Hyper-Threading Technology */
    bool tm;            /* Thermal Monitor */
    bool ia64;          /* IA64 processor emulating x86 */
    bool pbe;           /* Pending Break Enable */
    bool sse3;          /* Streaming SIMD Extensions 3 */
    bool pclmulqdq;     /* PCLMULQDQ instruction */
    bool dtes64;        /* 64-bit DS Area */
    bool monitor;       /* MONITOR/MWAIT */
    bool ds_cpl;        /* CPL Qualified Debug Store */
    bool vmx;           /* Virtual Machine Extensions */
    bool smx;           /* Safer Mode Extensions */
    bool est;           /* Enhanced SpeedStep */
    bool tm2;           /* Thermal Monitor 2 */
    bool ssse3;         /* Supplemental Streaming SIMD Extensions 3 */
    bool cnxt_id;       /* Context ID */
    bool sdbg;          /* Silicon Debug */
    bool fma;           /* Fused Multiply Add */
    bool cx16;          /* CMPXCHG16B */
    bool xtpr;          /* xTPR Update Control */
    bool pdcm;          /* Perfmon and Debug Capability */
    bool pcid;          /* Process-context identifiers */
    bool dca;           /* Direct Cache Access */
    bool sse4_1;        /* Streaming SIMD Extensions 4.1 */
    bool sse4_2;        /* Streaming SIMD Extensions 4.2 */
    bool x2apic;        /* Extended xAPIC */
    bool movbe;         /* MOVBE instruction */
    bool popcnt;        /* POPCNT instruction */
    bool tsc_deadline;  /* TSC-Deadline */
    bool aes;           /* AES instruction set */
    bool xsave;         /* XSAVE/XRSTOR/XSETBV/XGETBV */
    bool osxsave;       /* XSAVE enabled by OS */
    bool avx;           /* Advanced Vector Extensions */
    bool f16c;          /* F16C (half-precision) FP support */
    bool rdrand;        /* RDRAND instruction */
    bool hypervisor;    /* Running on a hypervisor */
} cpu_features_t;

/* CPU cache information */
typedef struct {
    uint32_t l1_data_cache_size;        /* L1 data cache size in KB */
    uint32_t l1_instruction_cache_size; /* L1 instruction cache size in KB */
    uint32_t l2_cache_size;             /* L2 cache size in KB */
    uint32_t l3_cache_size;             /* L3 cache size in KB */
    uint32_t cache_line_size;           /* Cache line size in bytes */
} cpu_cache_info_t;

/* CPU information structure */
typedef struct {
    char vendor[CPU_VENDOR_STRING_LEN];     /* CPU vendor string */
    char brand[CPU_BRAND_STRING_LEN];       /* CPU brand string */
    uint32_t max_cpuid;                     /* Maximum CPUID level */
    uint32_t max_extended_cpuid;            /* Maximum extended CPUID level */
    bool cpuid_available;                   /* Whether CPUID is available */
    
    /* CPU identification */
    uint32_t family;                        /* CPU family */
    uint32_t model;                         /* CPU model */
    uint32_t stepping;                      /* CPU stepping */
    uint32_t signature;                     /* CPU signature */
    
    /* Core and thread information */
    uint32_t logical_processors;           /* Number of logical processors */
    uint32_t physical_cores;               /* Number of physical cores */
    uint32_t threads_per_core;             /* Number of threads per core */
    
    /* CPU features */
    cpu_features_t features;               /* CPU feature flags */
    
    /* Cache information */
    cpu_cache_info_t cache;                /* CPU cache information */
    
    /* Frequency information */
    uint32_t base_frequency;               /* Base frequency in MHz */
    uint32_t max_frequency;                /* Max frequency in MHz */
    uint32_t bus_frequency;                /* Bus frequency in MHz */
} cpu_info_t;

/* CPU detection functions */
bool cpu_detect_cpuid(void);
void cpu_get_vendor(char* vendor);
void cpu_get_brand(char* brand);
bool cpu_get_info(cpu_info_t* info);
void cpu_print_info(const cpu_info_t* info);
void cpu_detect_features(cpu_features_t* features);
void cpu_detect_cache_info(cpu_cache_info_t* cache);
void cpu_detect_topology(cpu_info_t* info);

/* CPUID instruction wrapper */
void cpuid(uint32_t leaf, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx);

#endif /* CPU_H */

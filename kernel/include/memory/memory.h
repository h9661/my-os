#ifndef MEMORY_H
#define MEMORY_H

#include "../common/types.h"

/* Memory detection methods */
typedef enum {
    MEMORY_DETECT_CMOS = 0,
    MEMORY_DETECT_PROBE = 1,
    MEMORY_DETECT_E820 = 2
} memory_detect_method_t;

/* Memory information structure */
typedef struct {
    uint32_t total_kb;          /* Total memory in KB */
    uint32_t total_mb;          /* Total memory in MB */
    uint32_t available_kb;      /* Available memory in KB */
    uint32_t available_mb;      /* Available memory in MB */
    memory_detect_method_t method; /* Detection method used */
    bool valid;                 /* Whether the detection was successful */
} memory_info_t;

/* Memory detection functions */
uint32_t memory_detect_cmos(void);
uint32_t memory_detect_probe(void);
bool memory_get_info(memory_info_t* info);
void memory_print_info(const memory_info_t* info);

/* Memory management constants */
#define MEMORY_BASE_1MB     0x100000    /* 1MB boundary */
#define MEMORY_KERNEL_START 0x1000      /* Kernel load address */
#define MEMORY_STACK_TOP    0x90000     /* Stack top address */

/* CMOS register addresses */
#define CMOS_ADDR_PORT      0x70
#define CMOS_DATA_PORT      0x71
#define CMOS_MEM_LOW_REG    0x17
#define CMOS_MEM_HIGH_REG   0x18

#endif /* MEMORY_H */

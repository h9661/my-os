#ifndef KERNEL_H
#define KERNEL_H

/* Include all kernel subsystems */
#include "common/types.h"
#include "common/utils.h"
#include "memory/memory.h"
#include "vga/vga.h"
#include "terminal/terminal.h"
#include "cpu/cpu.h"
#include "cpu/fpu.h"
#include "interrupts/interrupts.h"  // 추가
#include "keyboard/keyboard.h"
#include "storage/hdd.h"

/* Kernel version information */
#define KERNEL_VERSION_MAJOR 0
#define KERNEL_VERSION_MINOR 1
#define KERNEL_VERSION_PATCH 0
#define KERNEL_NAME "Simple OS"

/* Kernel main function */
void kernel_main(void);

/* Kernel initialization functions */
void kernel_initialize(void);
void kernel_show_banner(void);
void kernel_show_hardware_info(void);

#endif /* KERNEL_H */

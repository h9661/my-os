#include "include/kernel.h"
#include "include/syscalls/syscalls.h"

/* Display kernel banner */
void kernel_show_banner(void) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writeline("Welcome to Simple OS!");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    char version_str[32];
    terminal_writestring("Version ");
    int_to_string(KERNEL_VERSION_MAJOR, version_str);
    terminal_writestring(version_str);
    terminal_writestring(".");
    int_to_string(KERNEL_VERSION_MINOR, version_str);
    terminal_writestring(version_str);
    terminal_writestring(".");
    int_to_string(KERNEL_VERSION_PATCH, version_str);
    terminal_writestring(version_str);
    terminal_writeline("");
    
    terminal_print_separator();
}

/* Show hardware information */
void kernel_show_hardware_info(void) {
    terminal_print_header("Hardware Information");
    
    /* CPU Information */
    cpu_info_t cpu_info;
    if (cpu_get_info(&cpu_info)) {
        cpu_print_info(&cpu_info);
    } else {
        terminal_writeline("Failed to get CPU information");
    }
    
    terminal_writeline("");
    
    /* Memory Information */
    memory_info_t memory_info;
    if (memory_get_info(&memory_info)) {
        memory_print_info(&memory_info);
    } else {
        terminal_writeline("Failed to get memory information");
    }
    
    terminal_print_separator();
}

/* Initialize kernel subsystems */
void kernel_initialize(void) {
    /* Initialize terminal first - this includes VGA initialization */
    terminal_initialize();
    
    /* Test VGA buffer integrity */
    terminal_test_vga_buffer();
    
    /* Test basic terminal functionality */
    terminal_writestring("Terminal initialized...\n");
    
    /* Initialize FPU before interrupts */
    fpu_initialize();
    terminal_writestring("FPU initialized...\n");
    
    /* Initialize process management system */
    process_init();
    terminal_writestring("Process system initialized...\n");
    
    /* Initialize system calls */
    syscalls_init();
    terminal_writestring("System calls initialized...\n");
    
    /* Initialize interrupts */
    interrupts_initialize();
    terminal_writestring("Interrupts initialized...\n");
    
    /* Initialize keyboard */
    keyboard_initialize();
    terminal_writestring("Keyboard initialized...\n");
    
    /* Initialize HDD subsystem */
    hdd_initialize();
    terminal_writestring("HDD initialized...\n");
    
    /* Initialize FAT32 file system on primary master */
    fat32_initialize(HDD_PRIMARY_MASTER);
    terminal_writestring("FAT32 file system initialized on primary master HDD...\n");
    
    /* Set default color scheme */
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("All subsystems initialized successfully!\n");
}

/* Main kernel entry point */
void kernel_main(void) {
    /* Initialize all kernel subsystems */
    kernel_initialize();
    
    /* Show kernel banner */
    kernel_show_banner();
    
    /* Show hardware information */
    kernel_show_hardware_info();
    
    /* Enable interrupts */
    enable_interrupts();
    
    /* Kernel main loop or additional initialization */
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writeline("Kernel initialized successfully!");
    terminal_writeline("Interrupts enabled. System ready.");
    terminal_writeline("Keyboard driver loaded. You can now type!");
    
    /* Display prompt */
    terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("SimpleOS> ");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    /* Main kernel loop - keep the kernel running */
    while (1) {
        /* Halt until next interrupt */
        __asm__ volatile("hlt");
    }
}

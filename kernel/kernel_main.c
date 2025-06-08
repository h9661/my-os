#include "include/kernel.h"
#include "include/syscalls/syscalls.h"
#include "include/timer/pit.h"
#include "include/process/process.h"

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

/* Display timer information */
void kernel_show_timer_info(void) {
    terminal_print_separator();
    terminal_writeline("Timer System Information:");
    
    char buffer[64];
    uint32_t ticks = timer_get_ticks();
    uint32_t seconds = timer_get_seconds();
    uint32_t ms = timer_get_milliseconds();
    
    /* Convert numbers to strings and display */
    terminal_writestring("System uptime: ");
    int_to_string(seconds, buffer);
    terminal_writestring(buffer);
    terminal_writestring(" seconds (");
    int_to_string(ms, buffer);
    terminal_writestring(buffer);
    terminal_writestring(" ms)\n");
    
    terminal_writestring("Total timer ticks: ");
    int_to_string(ticks, buffer);
    terminal_writestring(buffer);
    terminal_writestring("\n");
    
    terminal_writestring("Timer frequency: ");
    int_to_string(TIMER_FREQUENCY_100HZ, buffer);
    terminal_writestring(buffer);
    terminal_writeline(" Hz");
    
    terminal_print_separator();
}

/* Timer callback to show periodic activity */
static volatile uint32_t heartbeat_counter = 0;

void timer_heartbeat_callback(void) {
    heartbeat_counter++;
    /* Print a heartbeat every 5 seconds (500 ticks at 100Hz) */
    if (heartbeat_counter % 500 == 0) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
        terminal_writestring("[HEARTBEAT] ");
        char buffer[32];
        int_to_string(heartbeat_counter / 100, buffer);
        terminal_writestring(buffer);
        terminal_writeline("s uptime");
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    }
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
    
    /* Initialize PIT timer */
    pit_initialize();
    pit_set_frequency(TIMER_FREQUENCY_100HZ);
    terminal_writestring("PIT timer initialized at 100Hz...\n");
    
    /* Set timer callback for heartbeat */
    timer_set_callback(timer_heartbeat_callback);
    terminal_writestring("Timer heartbeat callback enabled...\n");
    
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
    
    /* Show timer information */
    kernel_show_timer_info();
    
    /* Enable interrupts */
    enable_interrupts();
    
    /* Kernel main loop or additional initialization */
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writeline("Kernel initialized successfully!");
    terminal_writeline("Interrupts enabled. System ready.");
    
    /* Start context switching test */
    process_test_context_switching();
    
    /* Main kernel loop - keep the kernel running */
    while (1) {
        /* Halt until next interrupt */
        __asm__ volatile("hlt");
    }
}

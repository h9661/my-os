#include "../../include/process/process.h"
#include "../../include/terminal/terminal.h"
#include "../../include/common/utils.h"
#include "../../include/timer/pit.h"
#include "../../include/memory/memory.h"
#include "../../include/vga/vga.h"
#include "../../include/syscalls/syscalls.h"

/* Global process table */
static process_t process_table[MAX_PROCESSES];
static bool process_slots_used[MAX_PROCESSES];

/* Global scheduler instance */
scheduler_t scheduler;

/* Next available PID */
static pid_t next_pid = 1;

/* Kernel idle process */
static process_t* idle_process = NULL;

/* Forward declarations for internal functions */
static void idle_process_entry(void);
static void process_cleanup(process_t* process);
static void scheduler_update_sleeping_processes(void);
static process_t* scheduler_get_next_process(void);
static void process_setup_stack(process_t* process, void (*entry_point)(void));

/* Process wrapper function to handle automatic cleanup when process returns */
static void process_wrapper(void (*entry_point)(void)) {
    /* Call the actual process function */
    if (entry_point) {
        entry_point();
    }
    
    /* If process returns without calling exit, automatically clean up */
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
    terminal_writestring("[KERNEL] Process ");
    
    process_t* current = get_current_process();
    if (current) {
        terminal_writestring(current->name);
        terminal_writestring(" (PID ");
        char pid_str[16];
        int_to_string(current->pid, pid_str);
        terminal_writestring(pid_str);
        terminal_writestring(")");
    }
    terminal_writeline(" returned without calling exit, auto-terminating");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    /* Automatically call sys_exit with status 0 */
    sys_exit(0);
}

/* Initialize process management system */
void process_init(void) {
    terminal_writeline("Initializing process management system...");
    
    /* Clear process table */
    memset(process_table, 0, sizeof(process_table));
    memset(process_slots_used, false, sizeof(process_slots_used));
    
    /* Initialize scheduler */
    scheduler_init();
    
    /* Create idle process */
    idle_process = process_create("idle", idle_process_entry, PROCESS_PRIORITY_LOW);
    if (idle_process) {
        idle_process->state = PROCESS_STATE_READY;
        terminal_writeline("Idle process created successfully");
    } else {
        terminal_writeline("Failed to create idle process");
    }
    
    terminal_writeline("Process management system initialized");
}

/* Initialize scheduler */
void scheduler_init(void) {
    scheduler.current_process = NULL;
    scheduler.ready_queue_head = NULL;
    scheduler.ready_queue_tail = NULL;
    scheduler.sleeping_queue = NULL;
    scheduler.num_processes = 0;
    scheduler.scheduler_ticks = 0;
    scheduler.preemption_enabled = true;
}

/* Create a new process */
process_t* process_create(const char* name, void (*entry_point)(void), process_priority_t priority) {
    /* Find free slot in process table */
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!process_slots_used[i]) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        terminal_writeline("Error: No free process slots available");
        return NULL;
    }
    
    process_t* process = &process_table[slot];
    process_slots_used[slot] = true;
    
    /* Initialize process structure */
    memset(process, 0, sizeof(process_t));
    
    process->pid = process_get_next_pid();
    process->state = PROCESS_STATE_READY;
    process->priority = priority;
    process->creation_time = timer_get_ticks();
    process->cpu_time = 0;
    process->sleep_until = 0;
    process->exit_code = 0;
    process->parent = scheduler.current_process;
    process->parent_pid = scheduler.current_process ? scheduler.current_process->pid : 0;
    
    /* Set process name */
    strncpy(process->name, name, 31);
    process->name[31] = '\0';
    
    /* Allocate stack */
    process->stack_size = 4096; /* 4KB stack */
    process->stack_base = (uint32_t)process_allocate_memory(process, process->stack_size);
    if (!process->stack_base) {
        process_slots_used[slot] = false;
        terminal_writeline("Error: Failed to allocate stack for process");
        return NULL;
    }
    
    /* Setup initial CPU state */
    process_setup_stack(process, entry_point);
    
    /* Add to scheduler */
    scheduler_add_process(process);
    
    /* Increment total process count */
    scheduler.num_processes++;
    
    return process;
}

/* Setup process stack and initial CPU state */
static void process_setup_stack(process_t* process, void (*entry_point)(void)) {
    /* Initialize registers */
    memset(&process->regs, 0, sizeof(process_regs_t));
    
    /* If entry_point is provided, wrap it with process_wrapper for automatic cleanup */
    if (entry_point) {
        /* Set up stack to call process_wrapper with entry_point as parameter */
        uint32_t* stack_ptr = (uint32_t*)(process->stack_base + process->stack_size);
        
        /* Push entry_point as parameter for process_wrapper */
        *(--stack_ptr) = (uint32_t)entry_point;
        
        /* Push return address (should never be reached) */
        *(--stack_ptr) = 0xDEADBEEF; /* Invalid return address to catch errors */
        
        /* Set initial register values to call process_wrapper */
        process->regs.eip = (uint32_t)process_wrapper;
        process->regs.esp = (uint32_t)stack_ptr;
        process->regs.ebp = process->regs.esp;
    } else {
        /* For processes without entry point (like idle), set up normally */
        process->regs.eip = 0;
        process->regs.esp = process->stack_base + process->stack_size - 4;
        process->regs.ebp = process->regs.esp;
    }
    process->regs.eflags = 0x202; /* Enable interrupts flag */
    
    /* Set segment registers to kernel data segment */
    process->regs.cs = 0x08; /* Kernel code segment */
    process->regs.ds = 0x10; /* Kernel data segment */
    process->regs.es = 0x10;
    process->regs.fs = 0x10;
    process->regs.gs = 0x10;
    process->regs.ss = 0x10; /* Kernel stack segment */
}

/* Fork current process */
process_t* process_fork(process_t* parent) {
    if (!parent) {
        return NULL;
    }
    
    /* Create child process with same name and priority */
    char child_name[32];
    strcpy(child_name, parent->name);
    strcat(child_name, "_child");
    
    process_t* child = process_create(child_name, NULL, parent->priority);
    if (!child) {
        return NULL;
    }
    
    /* Copy parent's register state */
    memcpy(&child->regs, &parent->regs, sizeof(process_regs_t));
    
    /* Child process returns 0 from fork */
    child->regs.eax = 0;
    
    /* Parent returns child PID */
    parent->regs.eax = child->pid;
    
    return child;
}

/* Terminate a process */
void process_terminate(process_t* process) {
    if (!process) {
        return;
    }
    
    /* Log process termination */
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("[KERNEL] Terminating process ");
    terminal_writestring(process->name);
    terminal_writestring(" (PID ");
    char pid_str[16];
    int_to_string(process->pid, pid_str);
    terminal_writestring(pid_str);
    terminal_writestring(") - Exit code: ");
    char exit_str[16];
    int_to_string(process->exit_code, exit_str);
    terminal_writeline(exit_str);
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    /* Set process state to terminated */
    process->state = PROCESS_STATE_TERMINATED;
    
    /* Notify parent process if it's waiting */
    if (process->parent && process->parent->state == PROCESS_STATE_BLOCKED) {
        /* Check if parent is waiting for this specific child */
        /* In a real implementation, we'd have a proper wait queue */
        process_wake_up(process->parent);
    }
    
    /* Handle orphaned child processes - reassign to init/idle process */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_slots_used[i] && process_table[i].parent == process) {
            process_table[i].parent = idle_process;
            process_table[i].parent_pid = idle_process ? idle_process->pid : 0;
        }
    }
    
    /* Remove from scheduler queues */
    scheduler_remove_process(process);
    
    /* Cleanup process resources */
    process_cleanup(process);
    
    /* Print remaining processes for debugging */
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("[DEBUG] Remaining processes: ");
    char count_str[16];
    int_to_string(scheduler.num_processes, count_str);
    terminal_writeline(count_str);
    
    /* List all remaining processes */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_slots_used[i] && &process_table[i] != process) {
            terminal_writestring("  - ");
            terminal_writestring(process_table[i].name);
            terminal_writestring(" (PID ");
            char pid_str[16];
            int_to_string(process_table[i].pid, pid_str);
            terminal_writestring(pid_str);
            terminal_writestring(", State: ");
            switch (process_table[i].state) {
                case PROCESS_STATE_READY: terminal_writestring("READY"); break;
                case PROCESS_STATE_RUNNING: terminal_writestring("RUNNING"); break;
                case PROCESS_STATE_BLOCKED: terminal_writestring("BLOCKED"); break;
                case PROCESS_STATE_SLEEPING: terminal_writestring("SLEEPING"); break;
                case PROCESS_STATE_TERMINATED: terminal_writestring("TERMINATED"); break;
                default: terminal_writestring("UNKNOWN"); break;
            }
            terminal_writeline(")");
        }
    }
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    /* If this is current process, schedule next one */
    if (process == scheduler.current_process) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        terminal_writeline("[DEBUG] Switching to next process...");
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        scheduler.current_process = NULL;
        scheduler_switch_process();
    }
}

/* Cleanup process resources */
static void process_cleanup(process_t* process) {
    if (!process) {
        return;
    }
    
    /* Free memory */
    if (process->stack_base) {
        process_free_memory(process, (void*)process->stack_base);
    }
    
    if (process->heap_base) {
        process_free_memory(process, (void*)process->heap_base);
    }
    
    /* Mark slot as free */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (&process_table[i] == process) {
            process_slots_used[i] = false;
            break;
        }
    }
    
    scheduler.num_processes--;
}

/* Kill a process with signal */
int process_kill(process_t* process, int signal) {
    if (!process) {
        return -1;
    }
    
    switch (signal) {
        case SIGNAL_TERM:
        case SIGNAL_KILL:
            process_terminate(process);
            break;
        case SIGNAL_STOP:
            process->state = PROCESS_STATE_BLOCKED;
            break;
        case SIGNAL_CONT:
            if (process->state == PROCESS_STATE_BLOCKED) {
                process->state = PROCESS_STATE_READY;
                scheduler_add_process(process);
            }
            break;
        default:
            return -1;
    }
    
    return 0;
}

/* Wait for child process */
int process_wait(process_t* parent, pid_t child_pid) {
    if (!parent) {
        return -1;
    }
    
    /* Find child process */
    process_t* child = process_find_by_pid(child_pid);
    if (!child || child->parent_pid != parent->pid) {
        return -1; /* Child not found or not our child */
    }
    
    /* If child is not terminated, block parent */
    if (child->state != PROCESS_STATE_TERMINATED) {
        parent->state = PROCESS_STATE_BLOCKED;
        /* In a real implementation, we would add this to a wait queue */
    }
    
    return child->exit_code;
}

/* Execute new program (simplified implementation) */
int process_exec(process_t* process, const char* path, char* const argv[]) {
    if (!process || !path) {
        return -1;
    }
    
    /* In a real implementation, this would load a program from storage */
    /* For now, just return success */
    terminal_writestring("exec called for path: ");
    terminal_writeline(path);
    
    return 0;
}

/* Put process to sleep */
void process_sleep(process_t* process, uint32_t milliseconds) {
    if (!process) {
        return;
    }
    
    process->state = PROCESS_STATE_SLEEPING;
    process->sleep_until = timer_get_ticks() + (milliseconds * timer_frequency / 1000);
    
    /* Remove from ready queue and add to sleeping queue */
    scheduler_remove_process(process);
    
    /* Add to sleeping queue */
    process->next = scheduler.sleeping_queue;
    if (scheduler.sleeping_queue) {
        scheduler.sleeping_queue->prev = process;
    }
    scheduler.sleeping_queue = process;
    process->prev = NULL;
}

/* Wake up a process */
void process_wake_up(process_t* process) {
    if (!process || process->state != PROCESS_STATE_SLEEPING) {
        return;
    }
    
    /* Remove from sleeping queue */
    if (process->prev) {
        process->prev->next = process->next;
    } else {
        scheduler.sleeping_queue = process->next;
    }
    if (process->next) {
        process->next->prev = process->prev;
    }
    
    /* Add back to ready queue */
    process->state = PROCESS_STATE_READY;
    scheduler_add_process(process);
}

/* Yield CPU to next process */
void process_yield(void) {
    if (scheduler.current_process) {
        scheduler.current_process->state = PROCESS_STATE_READY;
        scheduler_add_process(scheduler.current_process);
    }
    scheduler_switch_process();
}

/* Block a process */
void process_block(process_t* process) {
    if (!process) {
        return;
    }
    
    process->state = PROCESS_STATE_BLOCKED;
    scheduler_remove_process(process);
}

/* Unblock a process */
void process_unblock(process_t* process) {
    if (!process || process->state != PROCESS_STATE_BLOCKED) {
        return;
    }
    
    process->state = PROCESS_STATE_READY;
    scheduler_add_process(process);
}

/* Set process to zombie state (for parent to collect exit status) */
void process_set_zombie(process_t* process) {
    if (!process) {
        return;
    }
    
    process->state = PROCESS_STATE_ZOMBIE;
    
    /* Keep process in table for parent to wait() on it */
    /* Remove from all queues but don't cleanup yet */
    scheduler_remove_process(process);
    
    /* Log zombie transition */
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
    terminal_writestring("[KERNEL] Process ");
    terminal_writestring(process->name);
    terminal_writestring(" (PID ");
    char pid_str[16];
    int_to_string(process->pid, pid_str);
    terminal_writestring(pid_str);
    terminal_writeline(") became zombie - waiting for parent");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
}

/* Get current running process */
process_t* get_current_process(void) {
    return scheduler.current_process;
}

/* Find process by PID */
process_t* process_find_by_pid(pid_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_slots_used[i] && process_table[i].pid == pid) {
            return &process_table[i];
        }
    }
    return NULL;
}

/* Get next available PID */
pid_t process_get_next_pid(void) {
    return next_pid++;
}

/* Add process to scheduler */
void scheduler_add_process(process_t* process) {
    if (!process || process->state != PROCESS_STATE_READY) {
        return;
    }
    
    process->next = NULL;
    process->prev = scheduler.ready_queue_tail;
    
    if (scheduler.ready_queue_tail) {
        scheduler.ready_queue_tail->next = process;
    } else {
        scheduler.ready_queue_head = process;
    }
    scheduler.ready_queue_tail = process;
}

/* Remove process from scheduler queues */
void scheduler_remove_process(process_t* process) {
    if (!process) {
        return;
    }
    
    /* Remove from ready queue */
    if (process->prev) {
        process->prev->next = process->next;
    } else if (scheduler.ready_queue_head == process) {
        scheduler.ready_queue_head = process->next;
    }
    
    if (process->next) {
        process->next->prev = process->prev;
    } else if (scheduler.ready_queue_tail == process) {
        scheduler.ready_queue_tail = process->prev;
    }
    
    process->next = NULL;
    process->prev = NULL;
}

/* Get next process to run */
static process_t* scheduler_get_next_process(void) {
    /* Simple round-robin scheduling */
    process_t* next = scheduler.ready_queue_head;
    
    if (next) {
        /* Remove from head of queue */
        scheduler.ready_queue_head = next->next;
        if (scheduler.ready_queue_head) {
            scheduler.ready_queue_head->prev = NULL;
        } else {
            scheduler.ready_queue_tail = NULL;
        }
        next->next = NULL;
        next->prev = NULL;
    }
    
    /* If no process available, use idle process */
    if (!next && idle_process && idle_process->state == PROCESS_STATE_READY) {
        next = idle_process;
    }
    
    return next;
}

/* Switch to next process */
void scheduler_switch_process(void) {
    process_t* old_process = scheduler.current_process;
    process_t* new_process = scheduler_get_next_process();
    
    if (!new_process) {
        /* No process to run, stay with current or halt */
        return;
    }
    
    /* Update current process */
    scheduler.current_process = new_process;
    new_process->state = PROCESS_STATE_RUNNING;
    
    /* Perform context switch if processes are different */
    if (old_process && old_process != new_process) {
        /* Context switch implemented in assembly */
        context_switch(&old_process->regs, &new_process->regs);
    }
    else{
        /* old_process is null. so context switch to new_process */
        context_switch(NULL, &new_process->regs);
    }
}

/* Scheduler tick (called from timer interrupt) */
void scheduler_tick(void) {
    scheduler.scheduler_ticks++;
    
    /* Update sleeping processes */
    scheduler_update_sleeping_processes();
    
    /* Update current process CPU time */
    if (scheduler.current_process) {
        scheduler.current_process->cpu_time++;
    }

    /* Preemptive scheduling every 10 ticks (time quantum) */
    if (scheduler.preemption_enabled && scheduler.scheduler_ticks % 10 == 0) {
        if (scheduler.current_process && scheduler.current_process->state == PROCESS_STATE_RUNNING) {
            scheduler.current_process->state = PROCESS_STATE_READY;
            scheduler_add_process(scheduler.current_process);
            scheduler_switch_process();
        }
    }
}

/* Update sleeping processes */
static void scheduler_update_sleeping_processes(void) {
    uint32_t current_time = timer_get_ticks();
    process_t* process = scheduler.sleeping_queue;
    
    while (process) {
        process_t* next = process->next;
        
        if (current_time >= process->sleep_until) {
            process_wake_up(process);
        }
        
        process = next;
    }
}

/* Enable/disable preemptive scheduling */
void scheduler_set_preemption(bool enabled) {
    scheduler.preemption_enabled = enabled;
}

/* Idle process entry point */
static void idle_process_entry(void) {
    while (1) {
        /* Halt CPU until next interrupt */
        __asm__ volatile("hlt");
    }
}

/* Print process information */
void process_print_info(process_t* process) {
    if (!process) {
        terminal_writeline("Process: NULL");
        return;
    }
    
    char buffer[64];
    
    terminal_writestring("Process: ");
    terminal_writeline(process->name);
    
    terminal_writestring("  PID: ");
    int_to_string(process->pid, buffer);
    terminal_writeline(buffer);
    
    terminal_writestring("  Parent PID: ");
    int_to_string(process->parent_pid, buffer);
    terminal_writeline(buffer);
    
    terminal_writestring("  State: ");
    switch (process->state) {
        case PROCESS_STATE_RUNNING:
            terminal_writeline("Running");
            break;
        case PROCESS_STATE_READY:
            terminal_writeline("Ready");
            break;
        case PROCESS_STATE_BLOCKED:
            terminal_writeline("Blocked");
            break;
        case PROCESS_STATE_SLEEPING:
            terminal_writeline("Sleeping");
            break;
        case PROCESS_STATE_TERMINATED:
            terminal_writeline("Terminated");
            break;
        case PROCESS_STATE_ZOMBIE:
            terminal_writeline("Zombie");
            break;
        default:
            terminal_writeline("Unknown");
            break;
    }
    
    terminal_writestring("  CPU Time: ");
    int_to_string(process->cpu_time, buffer);
    terminal_writeline(buffer);
}

/* Print all processes */
void process_print_all(void) {
    terminal_writeline("=== Process List ===");
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_slots_used[i]) {
            process_print_info(&process_table[i]);
            terminal_writeline("");
        }
    }
}

/* Get process count */
uint32_t process_get_count(void) {
    return scheduler.num_processes;
}

/* Basic memory allocation for processes (simplified) */
void* process_allocate_memory(process_t* process, size_t size) {
    /* In a real implementation, this would use a proper heap allocator */
    /* For now, use a simple static allocation */
    static uint8_t process_memory[MAX_PROCESSES * 8192]; /* 8KB per process */
    static uint32_t memory_offset = 0;
    
    if (memory_offset + size > sizeof(process_memory)) {
        return NULL;
    }
    
    void* ptr = &process_memory[memory_offset];
    memory_offset += size;
    
    return ptr;
}

/* Free process memory (simplified) */
void process_free_memory(process_t* process, void* ptr) {
    /* In a real implementation, this would properly free memory */
    /* For now, this is a no-op */
}

/* Get process CPU time */
uint32_t process_get_cpu_time(process_t* process) {
    return process ? process->cpu_time : 0;
}

/* Get process uptime */
uint32_t process_get_uptime(process_t* process) {
    if (!process) {
        return 0;
    }
    
    uint32_t current_time = timer_get_ticks();
    return current_time - process->creation_time;
}

/* Test process A - prints messages periodically */
void test_process_a(void) {
    uint32_t counter = 0;
    char buffer[64];
    
    while (1) {
        counter++;
        
        /* Print message every 1000 iterations */
        if (counter % 1000 == 0) {
            terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK));
            terminal_writestring("[Process A] Counter: ");
            int_to_string(counter / 1000, buffer);
            terminal_writestring(buffer);
            terminal_writeline("");
            terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        }

        /* Test automatic cleanup when process returns */
        if(counter / 1000 == 10)
        {
            terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK));
            terminal_writestring("[Process A] Returning after 10 iterations (testing auto-cleanup).");
            terminal_writeline("");
            terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
            return; /* This will trigger process_wrapper auto-cleanup */
        } 
        
        /* Yield to other processes every 100 iterations */
        if (counter % 100 == 0) {
            process_yield();
        }
    }
}

/* Test process B - prints different messages periodically */
void test_process_b(void) {
    uint32_t counter = 0;
    char buffer[64];
    
    while (1) {
        counter++;
        
        /* Print message every 1500 iterations */
        if (counter % 1500 == 0) {
            terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
            terminal_writestring("[Process B] Iteration: ");
            int_to_string(counter / 1500, buffer);
            terminal_writestring(buffer);
            terminal_writeline("");
            terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        }
        
        /* Yield to other processes every 150 iterations */
        if (counter % 150 == 0) {
            process_yield();
        }
    }
}

/* Test context switching by creating two test processes */
void process_test_context_switching(void) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
    terminal_writeline("Starting context switching test...");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    /* Create test process A */
    process_t* proc_a = process_create("test_proc_a", test_process_a, PROCESS_PRIORITY_NORMAL);
    if (proc_a) {
        terminal_writeline("Test process A created successfully");
        char buffer[64];
        terminal_writestring("Process A PID: ");
        int_to_string(proc_a->pid, buffer);
        terminal_writeline(buffer);
    } else {
        terminal_writeline("Failed to create test process A");
        return;
    }
    
    /* Create test process B */
    process_t* proc_b = process_create("test_proc_b", test_process_b, PROCESS_PRIORITY_NORMAL);
    if (proc_b) {
        terminal_writeline("Test process B created successfully");
        char buffer[64];
        terminal_writestring("Process B PID: ");
        int_to_string(proc_b->pid, buffer);
        terminal_writeline(buffer);
    } else {
        terminal_writeline("Failed to create test process B");
        return;
    }
    
    /* Print current scheduler state */
    terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("Total processes in scheduler: ");
    char count_str[16];
    int_to_string(scheduler.num_processes, count_str);
    terminal_writeline(count_str);
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    
    /* Enable preemptive scheduling */
    scheduler_set_preemption(true);
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
    terminal_writeline("Context switching test processes started!");
    terminal_writeline("You should see alternating messages from Process A and Process B");
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));

    scheduler_switch_process();
}

#include "../../include/process/process.h"
#include "../../include/vga/vga.h"
#include "../../include/terminal/terminal.h"
#include "../../include/common/utils.h"

/* Global scheduler instance */
static scheduler_t scheduler;

/* Memory allocation for process stacks (simple allocation) */
static uint32_t stack_allocator_base = 0x200000;  /* Start at 2MB */
static uint32_t stack_allocator_current = 0x200000;

/* Initialize process management system */
void process_init(void) {
    scheduler_init();
    terminal_writeline("Process management system initialized");
}

/* Initialize scheduler */
void scheduler_init(void) {
    scheduler.current_process = NULL;
    scheduler.ready_queue.head = NULL;
    scheduler.ready_queue.tail = NULL;
    scheduler.ready_queue.count = 0;
    scheduler.blocked_queue.head = NULL;
    scheduler.blocked_queue.tail = NULL;
    scheduler.blocked_queue.count = 0;
    scheduler.next_pid = 1;
    scheduler.process_count = 0;
}

/* Allocate stack for a process */
uint32_t allocate_process_stack(uint32_t size) {
    uint32_t stack_base = stack_allocator_current;
    stack_allocator_current += size;
    return stack_base;
}

/* Free process stack (simple implementation - just mark as unused) */
void free_process_stack(uint32_t stack_base, uint32_t size) {
    /* Simple implementation - in a real OS this would be more complex */
    (void)stack_base;
    (void)size;
}

/* Create a new process */
process_t* process_create(const char* name, void (*entry_point)(void), process_priority_t priority) {
    /* Allocate memory for process structure */
    static process_t process_pool[32];  /* Simple static allocation */
    static uint32_t process_pool_index = 0;
    
    if (process_pool_index >= 32) {
        terminal_writeline("Error: Process pool exhausted");
        return NULL;
    }
    
    process_t* process = &process_pool[process_pool_index++];
    
    /* Initialize process structure */
    process->pid = scheduler.next_pid++;
    strncpy(process->name, name, 31);
    process->name[31] = '\0';
    process->state = PROCESS_STATE_READY;
    process->priority = priority;
    
    /* Allocate stack */
    process->stack_size = 4096;  /* 4KB stack */
    process->stack_base = allocate_process_stack(process->stack_size);
    process->stack_pointer = process->stack_base + process->stack_size - 4;
    
    /* Initialize CPU context */
    memset(&process->context, 0, sizeof(cpu_context_t));
    process->context.eip = (uint32_t)entry_point;
    process->context.esp = process->stack_pointer;
    process->context.ebp = process->stack_pointer;
    process->context.cs = 0x08;   /* Kernel code segment */
    process->context.ds = 0x10;   /* Kernel data segment */
    process->context.es = 0x10;
    process->context.fs = 0x10;
    process->context.gs = 0x10;
    process->context.ss = 0x10;   /* Kernel stack segment */
    process->context.eflags = 0x202;  /* Enable interrupts */
    
    /* Initialize time slicing */
    process->time_slice = 10;     /* 10 timer ticks */
    process->time_used = 0;
    
    /* Initialize queue pointers */
    process->next = NULL;
    process->prev = NULL;
    
    /* Add to scheduler */
    scheduler_add_process(process);
    scheduler.process_count++;
    
    /* Log process creation */
    char msg[64];
    strcpy(msg, "Created process: ");
    strcat(msg, name);
    strcat(msg, " (PID: ");
    char pid_str[16];
    int_to_string(process->pid, pid_str);
    strcat(msg, pid_str);
    strcat(msg, ")");
    terminal_writeline(msg);
    
    return process;
}

/* Add process to ready queue */
void scheduler_add_process(process_t* process) {
    if (scheduler.ready_queue.tail == NULL) {
        /* Empty queue */
        scheduler.ready_queue.head = process;
        scheduler.ready_queue.tail = process;
        process->next = NULL;
        process->prev = NULL;
    } else {
        /* Add to tail */
        scheduler.ready_queue.tail->next = process;
        process->prev = scheduler.ready_queue.tail;
        process->next = NULL;
        scheduler.ready_queue.tail = process;
    }
    scheduler.ready_queue.count++;
}

/* Remove process from ready queue */
void scheduler_remove_process(process_t* process) {
    if (process->prev) {
        process->prev->next = process->next;
    } else {
        scheduler.ready_queue.head = process->next;
    }
    
    if (process->next) {
        process->next->prev = process->prev;
    } else {
        scheduler.ready_queue.tail = process->prev;
    }
    
    process->next = NULL;
    process->prev = NULL;
    scheduler.ready_queue.count--;
}

/* Get next process to run (Round Robin scheduling) */
process_t* scheduler_get_next_process(void) {
    if (scheduler.ready_queue.head == NULL) {
        return NULL;  /* No processes to run */
    }
    
    /* Simple round-robin: get head and move it to tail */
    process_t* next_process = scheduler.ready_queue.head;
    
    /* Remove from head */
    scheduler_remove_process(next_process);
    
    /* Add back to tail if not terminated */
    if (next_process->state != PROCESS_STATE_TERMINATED) {
        scheduler_add_process(next_process);
    }
    
    return next_process;
}

/* Switch to next process */
void scheduler_switch_process(void) {
    process_t* old_process = scheduler.current_process;
    process_t* new_process = scheduler_get_next_process();
    
    if (new_process == NULL || new_process == old_process) {
        return;  /* No switching needed */
    }
    
    /* Update process states */
    if (old_process && old_process->state == PROCESS_STATE_RUNNING) {
        old_process->state = PROCESS_STATE_READY;
    }
    
    new_process->state = PROCESS_STATE_RUNNING;
    scheduler.current_process = new_process;
    
    /* Reset time slice */
    new_process->time_used = 0;
    
    /* Perform context switch */
    if (old_process) {
        context_switch(old_process, new_process);
    } else {
        /* First process - just restore context */
        restore_context(&new_process->context);
    }
}

/* Handle timer tick for scheduling */
void scheduler_tick(void) {
    if (scheduler.current_process == NULL) {
        /* No current process, try to start one */
        scheduler_switch_process();
        return;
    }
    
    /* Update time used */
    scheduler.current_process->time_used++;
    
    /* Check if time slice expired */
    if (scheduler.current_process->time_used >= scheduler.current_process->time_slice) {
        /* Time slice expired, switch to next process */
        scheduler_switch_process();
    }
}

/* Set process state */
void process_set_state(process_t* process, process_state_t state) {
    process->state = state;
}

/* Convert process state to string */
const char* process_state_to_string(process_state_t state) {
    switch (state) {
        case PROCESS_STATE_READY:
            return "READY";
        case PROCESS_STATE_RUNNING:
            return "RUNNING";
        case PROCESS_STATE_BLOCKED:
            return "BLOCKED";
        case PROCESS_STATE_TERMINATED:
            return "TERMINATED";
        default:
            return "UNKNOWN";
    }
}

/* Block a process */
void process_block(process_t* process) {
    if (process->state == PROCESS_STATE_RUNNING || process->state == PROCESS_STATE_READY) {
        process->state = PROCESS_STATE_BLOCKED;
        
        /* Remove from ready queue if present */
        if (process->state == PROCESS_STATE_READY) {
            scheduler_remove_process(process);
        }
        
        /* Add to blocked queue */
        /* Simple implementation - add to blocked queue */
        /* In real OS, this would be more sophisticated */
    }
}

/* Unblock a process */
void process_unblock(process_t* process) {
    if (process->state == PROCESS_STATE_BLOCKED) {
        process->state = PROCESS_STATE_READY;
        scheduler_add_process(process);
    }
}

/* Process yield (voluntarily give up CPU) */
void process_yield(void) {
    if (scheduler.current_process) {
        /* Reset time slice to force switch */
        scheduler.current_process->time_used = scheduler.current_process->time_slice;
        scheduler_switch_process();
    }
}

/* Find process by PID */
process_t* process_find_by_pid(uint32_t pid) {
    /* Search in ready queue */
    process_t* current = scheduler.ready_queue.head;
    while (current) {
        if (current->pid == pid) {
            return current;
        }
        current = current->next;
    }
    
    /* Check current running process */
    if (scheduler.current_process && scheduler.current_process->pid == pid) {
        return scheduler.current_process;
    }
    
    /* Search in blocked queue */
    current = scheduler.blocked_queue.head;
    while (current) {
        if (current->pid == pid) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

/* Get total process count */
uint32_t process_get_count(void) {
    return scheduler.process_count;
}

/* List all processes */
void process_list_all(void) {
    terminal_writeline("=== Process List ===");
    
    /* Show current process */
    if (scheduler.current_process) {
        char line[128];
        strcpy(line, "CURRENT: PID ");
        char pid_str[16];
        int_to_string(scheduler.current_process->pid, pid_str);
        strcat(line, pid_str);
        strcat(line, " - ");
        strcat(line, scheduler.current_process->name);
        strcat(line, " (");
        strcat(line, process_state_to_string(scheduler.current_process->state));
        strcat(line, ")");
        terminal_writeline(line);
    }
    
    /* Show ready processes */
    terminal_writeline("READY QUEUE:");
    process_t* current = scheduler.ready_queue.head;
    while (current) {
        char line[128];
        strcpy(line, "  PID ");
        char pid_str[16];
        int_to_string(current->pid, pid_str);
        strcat(line, pid_str);
        strcat(line, " - ");
        strcat(line, current->name);
        strcat(line, " (");
        strcat(line, process_state_to_string(current->state));
        strcat(line, ")");
        terminal_writeline(line);
        current = current->next;
    }
    
    char count_line[64];
    strcpy(count_line, "Total processes: ");
    char count_str[16];
    int_to_string(scheduler.process_count, count_str);
    strcat(count_line, count_str);
    terminal_writeline(count_line);
}

/* Get current running process */
process_t* get_current_process(void) {
    return scheduler.current_process;
}

/* Terminate a process */
void process_terminate(process_t* process) {
    if (!process) return;
    
    char msg[64];
    strcpy(msg, "Terminating process: ");
    strcat(msg, process->name);
    strcat(msg, " (PID: ");
    char pid_str[16];
    int_to_string(process->pid, pid_str);
    strcat(msg, pid_str);
    strcat(msg, ")");
    terminal_writeline(msg);
    
    /* Set state to terminated */
    process->state = PROCESS_STATE_TERMINATED;
    
    /* Remove from queues */
    scheduler_remove_process(process);
    
    /* Free stack memory */
    free_process_stack(process->stack_base, process->stack_size);
    
    /* Update process count */
    scheduler.process_count--;
    
    /* If this is the current process, switch to next */
    if (scheduler.current_process == process) {
        scheduler.current_process = NULL;
        scheduler_switch_process();
    }
}

/* Fork a process (create child process) */
process_t* process_fork(process_t* parent) {
    if (!parent) return NULL;
    
    /* Create new process with same entry point */
    char child_name[32];
    strcpy(child_name, parent->name);
    strcat(child_name, "_child");
    
    /* For simplicity, we'll create a copy with the same entry point */
    /* In a real OS, this would copy the entire memory space */
    process_t* child = process_create(child_name, (void*)parent->context.eip, parent->priority);
    
    if (child) {
        /* Copy parent's context to child */
        memcpy(&child->context, &parent->context, sizeof(cpu_context_t));
        
        /* Child gets different stack */
        child->context.esp = child->stack_pointer;
        child->context.ebp = child->stack_pointer;
        
        /* In child process, fork returns 0 */
        child->context.eax = 0;
        
        char msg[64];
        strcpy(msg, "Forked process ");
        char parent_pid[16], child_pid[16];
        int_to_string(parent->pid, parent_pid);
        int_to_string(child->pid, child_pid);
        strcat(msg, parent_pid);
        strcat(msg, " -> ");
        strcat(msg, child_pid);
        terminal_writeline(msg);
    }
    
    return child;
}

/* Put process to sleep for specified time */
void process_sleep(process_t* process, uint32_t milliseconds) {
    if (!process) return;
    
    /* Convert milliseconds to timer ticks (assuming 1000Hz timer) */
    uint32_t ticks = milliseconds;
    
    char msg[64];
    strcpy(msg, "Process ");
    char pid_str[16];
    int_to_string(process->pid, pid_str);
    strcat(msg, pid_str);
    strcat(msg, " sleeping for ");
    char time_str[16];
    int_to_string(milliseconds, time_str);
    strcat(msg, time_str);
    strcat(msg, "ms");
    terminal_writeline(msg);
    
    /* Block the process */
    process_block(process);
    
    /* TODO: Set up timer to unblock after specified time */
    /* For now, just block indefinitely */
}

/* Kill a process with signal */
int process_kill(process_t* process, int signal) {
    if (!process) return -1;
    
    char msg[64];
    strcpy(msg, "Sending signal ");
    char sig_str[16];
    int_to_string(signal, sig_str);
    strcat(msg, sig_str);
    strcat(msg, " to process ");
    char pid_str[16];
    int_to_string(process->pid, pid_str);
    strcat(msg, pid_str);
    terminal_writeline(msg);
    
    switch (signal) {
        case SIGTERM:
        case SIGKILL:
            process_terminate(process);
            return 0;
            
        case SIGSTOP:
            process_block(process);
            return 0;
            
        case SIGCONT:
            process_unblock(process);
            return 0;
            
        default:
            terminal_writeline("Unknown signal");
            return -1;
    }
}

/* Wait for child process */
int process_wait(process_t* parent, uint32_t child_pid) {
    if (!parent) return -1;
    
    char msg[64];
    strcpy(msg, "Process ");
    char parent_pid[16];
    int_to_string(parent->pid, parent_pid);
    strcat(msg, parent_pid);
    strcat(msg, " waiting for child ");
    char child_pid_str[16];
    int_to_string(child_pid, child_pid_str);
    strcat(msg, child_pid_str);
    terminal_writeline(msg);
    
    /* Find child process */
    process_t* child = process_find_by_pid(child_pid);
    if (!child) {
        return -1;  /* Child not found */
    }
    
    /* Block parent until child terminates */
    if (child->state != PROCESS_STATE_TERMINATED) {
        process_block(parent);
    }
    
    return 0;
}

/* Execute new program (replace current process) */
int process_exec(process_t* process, const char* path, char* const argv[]) {
    if (!process) return -1;
    
    char msg[64];
    strcpy(msg, "Executing ");
    strcat(msg, path);
    strcat(msg, " in process ");
    char pid_str[16];
    int_to_string(process->pid, pid_str);
    strcat(msg, pid_str);
    terminal_writeline(msg);
    
    /* For simplicity, just change the process name */
    /* In a real OS, this would load a new program */
    strncpy(process->name, path, 31);
    process->name[31] = '\0';
    
    /* Reset process state */
    process->time_used = 0;
    
    return 0;
}

/* Set process priority */
void process_set_priority(process_t* process, process_priority_t priority) {
    if (!process) return;
    
    process->priority = priority;
    
    /* Adjust time slice based on priority */
    switch (priority) {
        case PROCESS_PRIORITY_HIGH:
            process->time_slice = 20;  /* Longer time slice */
            break;
        case PROCESS_PRIORITY_NORMAL:
            process->time_slice = 10;  /* Normal time slice */
            break;
        case PROCESS_PRIORITY_LOW:
            process->time_slice = 5;   /* Shorter time slice */
            break;
    }
    
    char msg[64];
    strcpy(msg, "Set priority of process ");
    char pid_str[16];
    int_to_string(process->pid, pid_str);
    strcat(msg, pid_str);
    strcat(msg, " to ");
    char prio_str[16];
    int_to_string((int)priority, prio_str);
    strcat(msg, prio_str);
    terminal_writeline(msg);
}

/* Enhanced scheduler with priority support */
process_t* scheduler_get_next_process_priority(void) {
    process_t* highest_priority = NULL;
    process_t* current = scheduler.ready_queue.head;
    
    /* Find highest priority process */
    while (current) {
        if (!highest_priority || current->priority > highest_priority->priority) {
            highest_priority = current;
        }
        current = current->next;
    }
    
    if (highest_priority) {
        scheduler_remove_process(highest_priority);
        /* Add back to queue if not terminated */
        if (highest_priority->state != PROCESS_STATE_TERMINATED) {
            scheduler_add_process(highest_priority);
        }
    }
    
    return highest_priority;
}

/* Process debugging and monitoring */
void process_debug_info(process_t* process) {
    if (!process) return;
    
    terminal_writeline("=== Process Debug Info ===");
    
    char line[128];
    strcpy(line, "PID: ");
    char pid_str[16];
    int_to_string(process->pid, pid_str);
    strcat(line, pid_str);
    terminal_writeline(line);
    
    strcpy(line, "Name: ");
    strcat(line, process->name);
    terminal_writeline(line);
    
    strcpy(line, "State: ");
    strcat(line, process_state_to_string(process->state));
    terminal_writeline(line);
    
    strcpy(line, "Priority: ");
    char prio_str[16];
    int_to_string((int)process->priority, prio_str);
    strcat(line, prio_str);
    terminal_writeline(line);
    
    strcpy(line, "Time slice: ");
    char slice_str[16];
    int_to_string(process->time_slice, slice_str);
    strcat(line, slice_str);
    terminal_writeline(line);
    
    strcpy(line, "Time used: ");
    char used_str[16];
    int_to_string(process->time_used, used_str);
    strcat(line, used_str);
    terminal_writeline(line);
}

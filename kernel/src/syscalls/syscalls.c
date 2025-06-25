#include "../../include/syscalls/syscalls.h"
#include "../../include/process/process.h"
#include "../../include/vga/vga.h"
#include "../../include/terminal/terminal.h"
#include "../../include/common/utils.h"
#include "../../include/interrupts/interrupts.h"

/* Initialize system calls */
void syscalls_init(void) {
    setup_syscall_interrupt();
    terminal_writeline("System calls initialized");
}

/* Main system call handler */
uint32_t syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    switch (syscall_num) {
        case SYS_EXIT:
            sys_exit((int)arg1);
            return 0;
            
        case SYS_FORK:
            return sys_fork();
            
        case SYS_GETPID:
            return sys_getpid();
            
        case SYS_SLEEP:
            sys_sleep(arg1);
            return 0;
            
        case SYS_YIELD:
            sys_yield();
            return 0;
            
        case SYS_KILL:
            return (uint32_t)sys_kill(arg1, (int)arg2);
            
        case SYS_WAITPID:
            return (uint32_t)sys_waitpid(arg1);
            
        case SYS_EXEC:
            return (uint32_t)sys_exec((const char*)arg1, (char* const*)arg2);
            
        default:
            terminal_writeline("Error: Invalid system call");
            return -1;
    }
}

/* Exit current process */
void sys_exit(int exit_code) {
    process_t* current = get_current_process();
    if (current) {
        char msg[64];
        strcpy(msg, "Process ");
        char pid_str[16];
        int_to_string(current->pid, pid_str);
        strcat(msg, pid_str);
        strcat(msg, " exited with code ");
        char code_str[16];
        int_to_string(exit_code, code_str);
        strcat(msg, code_str);
        terminal_writeline(msg);
        
        /* Set exit code before terminating */
        current->exit_code = exit_code;
        
        /* Terminate the process (this will call scheduler_switch_process internally) */
        process_terminate(current);
        scheduler_switch_process();
    }
}

/* Fork current process */
uint32_t sys_fork(void) {
    process_t* parent = get_current_process();
    if (!parent) {
        return -1;
    }
    
    process_t* child = process_fork(parent);
    if (!child) {
        return -1;  /* Fork failed */
    }
    
    /* Return child PID to parent, 0 to child */
    return child->pid;
}

/* Get current process ID */
uint32_t sys_getpid(void) {
    process_t* current = get_current_process();
    return current ? current->pid : 0;
}

/* Sleep for specified milliseconds */
void sys_sleep(uint32_t milliseconds) {
    process_t* current = get_current_process();
    if (current) {
        process_sleep(current, milliseconds);
    }
}

/* Yield CPU to next process */
void sys_yield(void) {
    process_yield();
}

/* Kill a process with signal */
int sys_kill(uint32_t pid, int signal) {
    process_t* target = process_find_by_pid(pid);
    if (!target) {
        return -1;  /* Process not found */
    }
    
    return process_kill(target, signal);
}

/* Wait for child process to terminate */
int sys_waitpid(uint32_t pid) {
    process_t* current = get_current_process();
    if (!current) {
        return -1;
    }
    
    return process_wait(current, pid);
}

/* Execute new program */
int sys_exec(const char* path, char* const argv[]) {
    process_t* current = get_current_process();
    if (!current) {
        return -1;
    }
    
    return process_exec(current, path, argv);
}

/* Setup system call interrupt (INT 0x80) */
void setup_syscall_interrupt(void) {
    /* Register INT 0x80 handler in IDT */
    extern void syscall_interrupt_handler(void);
    
    /* Set IDT gate for system call interrupt */
    idt_set_gate(0x80, (uint32_t)syscall_interrupt_handler, 0x08, IDT_TYPE_INTERRUPT_GATE);
    
    terminal_writeline("System call interrupt (INT 0x80) registered");
}

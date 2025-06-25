#ifndef PROCESS_H
#define PROCESS_H

#include "../common/types.h"

/* Process states */
typedef enum {
    PROCESS_STATE_RUNNING = 0,
    PROCESS_STATE_READY,
    PROCESS_STATE_BLOCKED,
    PROCESS_STATE_SLEEPING,
    PROCESS_STATE_TERMINATED,
    PROCESS_STATE_ZOMBIE
} process_state_t;

/* Process priority levels */
typedef enum {
    PROCESS_PRIORITY_SYSTEM = 0,    /* Highest priority for system processes */
    PROCESS_PRIORITY_HIGH = 1,      /* High priority */
    PROCESS_PRIORITY_NORMAL = 2,    /* Normal priority */
    PROCESS_PRIORITY_LOW = 3        /* Low priority */
} process_priority_t;

/* Maximum number of processes */
#define MAX_PROCESSES 32

/* Process ID type */
typedef uint32_t pid_t;

/* Process register state for context switching */
typedef struct {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, esp, ebp;
    uint32_t eip, eflags;
    uint16_t cs, ds, es, fs, gs, ss;
} __attribute__((packed)) process_regs_t;

/* Process control block (PCB) */
typedef struct process {
    pid_t pid;                      /* Process ID */
    pid_t parent_pid;               /* Parent process ID */
    process_state_t state;          /* Current process state */
    process_priority_t priority;    /* Process priority */
    
    /* Memory management */
    uint32_t stack_base;            /* Stack base address */
    uint32_t stack_size;            /* Stack size */
    uint32_t heap_base;             /* Heap base address */
    uint32_t heap_size;             /* Heap size */
    
    /* CPU state */
    process_regs_t regs;            /* CPU registers */
    uint32_t kernel_stack;          /* Kernel stack pointer */
    
    /* Timing */
    uint32_t creation_time;         /* Process creation time */
    uint32_t cpu_time;              /* Total CPU time used */
    uint32_t sleep_until;           /* Wake up time (for sleeping processes) */
    
    /* Process management */
    int exit_code;                  /* Exit code when terminated */
    struct process* parent;         /* Pointer to parent process */
    struct process* next;           /* Next process in list */
    struct process* prev;           /* Previous process in list */
    
    /* Process name */
    char name[32];                  /* Process name */
} process_t;

/* Scheduler state */
typedef struct {
    process_t* current_process;     /* Currently running process */
    process_t* ready_queue_head;    /* Head of ready queue */
    process_t* ready_queue_tail;    /* Tail of ready queue */
    process_t* sleeping_queue;      /* Sleeping processes queue */
    uint32_t num_processes;         /* Total number of processes */
    uint32_t scheduler_ticks;       /* Scheduler tick counter */
    bool preemption_enabled;        /* Preemptive scheduling enabled */
} scheduler_t;

/* Global scheduler instance - defined in process.c */
extern scheduler_t scheduler;

/* Process management functions */
void process_init(void);
process_t* process_create(const char* name, void (*entry_point)(void), process_priority_t priority);
process_t* process_fork(process_t* parent);
void process_terminate(process_t* process);
void process_set_zombie(process_t* process);
int process_kill(process_t* process, int signal);
int process_wait(process_t* parent, pid_t child_pid);
int process_exec(process_t* process, const char* path, char* const argv[]);

/* Process state management */
void process_sleep(process_t* process, uint32_t milliseconds);
void process_wake_up(process_t* process);
void process_yield(void);
void process_block(process_t* process);
void process_unblock(process_t* process);

/* Process lookup */
process_t* get_current_process(void);
process_t* process_find_by_pid(pid_t pid);
pid_t process_get_next_pid(void);

/* Scheduler functions */
void scheduler_init(void);
void scheduler_add_process(process_t* process);
void scheduler_remove_process(process_t* process);
void scheduler_switch_process(void);
void scheduler_tick(void);
void scheduler_set_preemption(bool enabled);

/* Context switching (implemented in assembly) */
extern void context_switch(process_regs_t* old_regs, process_regs_t* new_regs);

/* Process debugging and information */
void process_print_info(process_t* process);
void process_print_all(void);
uint32_t process_get_count(void);

/* Signal handling */
#define SIGNAL_TERM     1   /* Terminate */
#define SIGNAL_KILL     2   /* Kill (cannot be caught) */
#define SIGNAL_STOP     3   /* Stop */
#define SIGNAL_CONT     4   /* Continue */

/* Memory allocation for processes */
void* process_allocate_memory(process_t* process, size_t size);
void process_free_memory(process_t* process, void* ptr);

/* Process timing functions */
uint32_t process_get_cpu_time(process_t* process);
uint32_t process_get_uptime(process_t* process);

/* Context switching test functions */
void process_test_context_switching(void);
void test_process_a(void);
void test_process_b(void);

#endif /* PROCESS_H */

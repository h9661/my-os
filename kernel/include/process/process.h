#ifndef PROCESS_H
#define PROCESS_H

#include "../common/types.h"

/* Process states */
typedef enum {
    PROCESS_STATE_READY = 0,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_BLOCKED,
    PROCESS_STATE_TERMINATED
} process_state_t;

/* Process priority levels */
typedef enum {
    PROCESS_PRIORITY_LOW = 0,
    PROCESS_PRIORITY_NORMAL,
    PROCESS_PRIORITY_HIGH
} process_priority_t;

/* CPU context structure for context switching */
typedef struct {
    uint32_t eax, ebx, ecx, edx;    /* General purpose registers */
    uint32_t esi, edi;              /* Index registers */
    uint32_t esp, ebp;              /* Stack pointers */
    uint32_t eip;                   /* Instruction pointer */
    uint32_t eflags;                /* Flags register */
    uint32_t cs, ds, es, fs, gs, ss; /* Segment registers */
} cpu_context_t;

/* Process Control Block (PCB) */
typedef struct process {
    uint32_t pid;                   /* Process ID */
    char name[32];                  /* Process name */
    process_state_t state;          /* Current state */
    process_priority_t priority;    /* Process priority */
    
    cpu_context_t context;          /* CPU context for context switching */
    
    uint32_t stack_base;            /* Stack base address */
    uint32_t stack_size;            /* Stack size */
    uint32_t stack_pointer;         /* Current stack pointer */
    
    uint32_t time_slice;            /* Time slice for scheduling */
    uint32_t time_used;             /* Time used in current slice */
    
    struct process *next;           /* Next process in queue */
    struct process *prev;           /* Previous process in queue */
} process_t;

/* Process queue structure */
typedef struct {
    process_t *head;
    process_t *tail;
    uint32_t count;
} process_queue_t;

/* Scheduler structure */
typedef struct {
    process_t *current_process;     /* Currently running process */
    process_queue_t ready_queue;    /* Ready processes queue */
    process_queue_t blocked_queue;  /* Blocked processes queue */
    uint32_t next_pid;             /* Next available PID */
    uint32_t process_count;        /* Total number of processes */
} scheduler_t;

/* Process management functions */
void process_init(void);
process_t* process_create(const char* name, void (*entry_point)(void), process_priority_t priority);
void process_destroy(process_t* process);
void process_yield(void);
void process_block(process_t* process);
void process_unblock(process_t* process);
void process_set_priority(process_t* process, process_priority_t priority);

/* Process control functions */
process_t* get_current_process(void);
void process_terminate(process_t* process);
process_t* process_fork(process_t* parent);
void process_sleep(process_t* process, uint32_t milliseconds);
int process_kill(process_t* process, int signal);
int process_wait(process_t* parent, uint32_t child_pid);
int process_exec(process_t* process, const char* path, char* const argv[]);

/* Signal handling */
#define SIGTERM 15
#define SIGKILL 9
#define SIGSTOP 19
#define SIGCONT 18

typedef struct {
    int signal;
    uint32_t timestamp;
} signal_t;

/* Process synchronization */
void process_mutex_init(void);
int process_mutex_lock(uint32_t mutex_id);
int process_mutex_unlock(uint32_t mutex_id);

/* Scheduler functions */
void scheduler_init(void);
void scheduler_add_process(process_t* process);
void scheduler_remove_process(process_t* process);
process_t* scheduler_get_next_process(void);
void scheduler_switch_process(void);
void scheduler_tick(void);

/* Context switching functions */
void context_switch(process_t* old_process, process_t* new_process);
void save_context(cpu_context_t* context);
void restore_context(cpu_context_t* context);

/* Process information functions */
void process_list_all(void);
process_t* process_find_by_pid(uint32_t pid);
uint32_t process_get_count(void);
void process_debug_info(process_t* process);

/* Process states management */
void process_set_state(process_t* process, process_state_t state);
const char* process_state_to_string(process_state_t state);

/* Stack management for processes */
uint32_t allocate_process_stack(uint32_t size);
void free_process_stack(uint32_t stack_base, uint32_t size);

#endif /* PROCESS_H */

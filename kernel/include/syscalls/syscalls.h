#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "../common/types.h"
#include "../process/process.h"

/* System call numbers */
#define SYS_EXIT        1
#define SYS_FORK        2
#define SYS_GETPID      3
#define SYS_SLEEP       4
#define SYS_YIELD       5
#define SYS_KILL        6
#define SYS_WAITPID     7
#define SYS_EXEC        8

/* System call interface */
void syscalls_init(void);
uint32_t syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3);

/* Individual system calls */
void sys_exit(int exit_code);
uint32_t sys_fork(void);
uint32_t sys_getpid(void);
void sys_sleep(uint32_t milliseconds);
void sys_yield(void);
int sys_kill(uint32_t pid, int signal);
int sys_waitpid(uint32_t pid);
int sys_exec(const char* path, char* const argv[]);

/* Helper functions */
void setup_syscall_interrupt(void);

#endif /* SYSCALLS_H */

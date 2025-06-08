[BITS 32]

; System call interrupt handler
global syscall_interrupt_handler
extern syscall_handler

; System call interrupt handler (INT 0x80)
syscall_interrupt_handler:
    ; Disable interrupts
    cli
    
    ; Save all registers
    pusha
    
    ; Save segment registers
    push ds
    push es
    push fs
    push gs
    
    ; Load kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Push arguments for syscall_handler
    ; System call convention: EAX = syscall number, EBX = arg1, ECX = arg2, EDX = arg3
    push edx    ; arg3
    push ecx    ; arg2
    push ebx    ; arg1
    push eax    ; syscall_num
    
    ; Call C handler
    call syscall_handler
    
    ; Clean up stack (4 arguments)
    add esp, 16
    
    ; Restore segment registers
    pop gs
    pop fs
    pop es
    pop ds
    
    ; Restore all registers
    popa
    
    ; Enable interrupts and return
    sti
    iret

[BITS 32]

; External C functions
extern exception_handler_common
extern irq_handler_common

; Exception handlers
global exception_handler_0
global exception_handler_1
global exception_handler_2
global exception_handler_3
global exception_handler_4
global exception_handler_5
global exception_handler_6
global exception_handler_7
global exception_handler_8
global exception_handler_9
global exception_handler_10
global exception_handler_11
global exception_handler_12
global exception_handler_13
global exception_handler_14
global exception_handler_15
global exception_handler_16
global exception_handler_17
global exception_handler_18
global exception_handler_19

; Hardware interrupt handlers
global irq_handler_timer
global irq_handler_keyboard

; Exception handler macro (no error code)
%macro EXCEPTION_NOERRCODE 1
exception_handler_%1:
    cli
    push 0          ; Push dummy error code
    push %1         ; Push exception number
    jmp exception_common_stub
%endmacro

; Exception handler macro (with error code)
%macro EXCEPTION_ERRCODE 1
exception_handler_%1:
    cli
    push %1         ; Push exception number
    jmp exception_common_stub
%endmacro

; IRQ handler macro
%macro IRQ_HANDLER 2
irq_handler_%2:
    cli
    push 0          ; Push dummy error code
    push %1         ; Push IRQ number
    jmp irq_common_stub
%endmacro

; Exception handlers
EXCEPTION_NOERRCODE 0   ; Divide by zero
EXCEPTION_NOERRCODE 1   ; Debug
EXCEPTION_NOERRCODE 2   ; NMI
EXCEPTION_NOERRCODE 3   ; Breakpoint
EXCEPTION_NOERRCODE 4   ; Overflow
EXCEPTION_NOERRCODE 5   ; Bound range exceeded
EXCEPTION_NOERRCODE 6   ; Invalid opcode
EXCEPTION_NOERRCODE 7   ; Device not available
EXCEPTION_ERRCODE   8   ; Double fault
EXCEPTION_NOERRCODE 9   ; Coprocessor segment overrun (reserved)
EXCEPTION_ERRCODE   10  ; Invalid TSS
EXCEPTION_ERRCODE   11  ; Segment not present
EXCEPTION_ERRCODE   12  ; Stack fault
EXCEPTION_ERRCODE   13  ; General protection fault
EXCEPTION_ERRCODE   14  ; Page fault
EXCEPTION_NOERRCODE 15  ; Reserved
EXCEPTION_NOERRCODE 16  ; x87 FPU error
EXCEPTION_ERRCODE   17  ; Alignment check
EXCEPTION_NOERRCODE 18  ; Machine check
EXCEPTION_NOERRCODE 19  ; SIMD FP exception

; Hardware interrupt handlers
IRQ_HANDLER 32, timer
IRQ_HANDLER 33, keyboard

; Common exception handler stub
exception_common_stub:
    pusha           ; Push all general purpose registers
    
    mov ax, ds      ; Push data segment
    push eax
    
    mov ax, 0x10    ; Load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    call exception_handler_common
    
    pop eax         ; Restore data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa            ; Restore registers
    add esp, 8      ; Remove error code and interrupt number
    sti
    iret            ; Return from interrupt

; Common IRQ handler stub
irq_common_stub:
    pusha           ; Push all general purpose registers
    
    mov ax, ds      ; Push data segment
    push eax
    
    mov ax, 0x10    ; Load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    call irq_handler_common
    
    pop eax         ; Restore data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa            ; Restore registers
    add esp, 8      ; Remove error code and interrupt number
    sti
    iret            ; Return from interrupt
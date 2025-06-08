[BITS 32]

; External C handler function declarations
extern c_irq_handler_timer
extern c_irq_handler_keyboard
extern c_exception_handler

; IRQ handler global declarations
global irq_handler_timer
global irq_handler_keyboard
global exception_handler_common

; Exception handlers (0-31)
global exception_handler_0   ; Division by zero
global exception_handler_1   ; Debug
global exception_handler_2   ; Non-maskable interrupt
global exception_handler_3   ; Breakpoint
global exception_handler_4   ; Overflow
global exception_handler_5   ; Bound range exceeded
global exception_handler_6   ; Invalid opcode
global exception_handler_7   ; Device not available
global exception_handler_8   ; Double fault
global exception_handler_9   ; Coprocessor segment overrun
global exception_handler_10  ; Invalid TSS
global exception_handler_11  ; Segment not present
global exception_handler_12  ; Stack-segment fault
global exception_handler_13  ; General protection fault
global exception_handler_14  ; Page fault
global exception_handler_15  ; Reserved
global exception_handler_16  ; x87 FPU floating-point error
global exception_handler_17  ; Alignment check
global exception_handler_18  ; Machine check
global exception_handler_19  ; SIMD floating-point exception

; Common interrupt handler macro for code reuse
%macro IRQ_HANDLER_COMMON 1
    ; NOTE: CLI is not needed - interrupts are automatically disabled
    ; when entering an interrupt handler via interrupt gate
    pusha              ; Save all general-purpose registers
    
    ; Save segment registers
    mov ax, ds
    push eax
    mov ax, es  
    push eax
    mov ax, fs
    push eax
    mov ax, gs
    push eax
    
    ; Load kernel data segment
    mov ax, 0x10       ; Kernel data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Push IRQ number as parameter
    push %1
    call %1            ; Call the specific C handler
    add esp, 4         ; Clean up stack (remove parameter)
    
    ; Restore segment registers
    pop eax
    mov gs, ax
    pop eax
    mov fs, ax
    pop eax
    mov es, ax
    pop eax
    mov ds, ax
    
    popa               ; Restore general-purpose registers
    iret               ; Return from interrupt (automatically enables interrupts)
%endmacro

; Exception handler macro for CPU exceptions
%macro EXCEPTION_HANDLER 2
exception_handler_%1:
    ; Some exceptions push error code, others don't
    %if %2 == 0
        push 0         ; Push dummy error code for exceptions that don't have one
    %endif
    push %1            ; Push exception number
    jmp exception_handler_common
%endmacro

; Timer IRQ handler (IRQ0 -> INT 32)
irq_handler_timer:
    IRQ_HANDLER_COMMON c_irq_handler_timer

; Keyboard IRQ handler (IRQ1 -> INT 33)  
irq_handler_keyboard:
    IRQ_HANDLER_COMMON c_irq_handler_keyboard

; Exception handlers
EXCEPTION_HANDLER 0, 0   ; Division by zero
EXCEPTION_HANDLER 1, 0   ; Debug
EXCEPTION_HANDLER 2, 0   ; Non-maskable interrupt
EXCEPTION_HANDLER 3, 0   ; Breakpoint
EXCEPTION_HANDLER 4, 0   ; Overflow
EXCEPTION_HANDLER 5, 0   ; Bound range exceeded
EXCEPTION_HANDLER 6, 0   ; Invalid opcode
EXCEPTION_HANDLER 7, 0   ; Device not available
EXCEPTION_HANDLER 8, 1   ; Double fault (has error code)
EXCEPTION_HANDLER 9, 0   ; Coprocessor segment overrun
EXCEPTION_HANDLER 10, 1  ; Invalid TSS (has error code)
EXCEPTION_HANDLER 11, 1  ; Segment not present (has error code)
EXCEPTION_HANDLER 12, 1  ; Stack-segment fault (has error code)
EXCEPTION_HANDLER 13, 1  ; General protection fault (has error code)
EXCEPTION_HANDLER 14, 1  ; Page fault (has error code)
EXCEPTION_HANDLER 15, 0  ; Reserved
EXCEPTION_HANDLER 16, 0  ; x87 FPU floating-point error
EXCEPTION_HANDLER 17, 1  ; Alignment check (has error code)
EXCEPTION_HANDLER 18, 0  ; Machine check
EXCEPTION_HANDLER 19, 0  ; SIMD floating-point exception

; Common exception handler
exception_handler_common:
    pusha              ; Save all general-purpose registers
    
    ; Save segment registers
    mov ax, ds
    push eax
    mov ax, es
    push eax
    mov ax, fs
    push eax
    mov ax, gs
    push eax
    
    ; Load kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Call C exception handler
    ; Stack layout: [gs][fs][es][ds][popa_registers][exception_num][error_code][eip][cs][eflags]
    ; Pass pointer to the exception context as parameter
    mov eax, esp       ; ESP points to the context structure
    push eax           ; Pass context pointer as parameter
    call c_exception_handler
    add esp, 4         ; Clean up parameter
    
    ; Restore segment registers
    pop eax
    mov gs, ax
    pop eax
    mov fs, ax
    pop eax
    mov es, ax
    pop eax
    mov ds, ax
    
    popa               ; Restore general-purpose registers
    add esp, 8         ; Remove exception number and error code from stack
    iret               ; Return from interrupt
[BITS 32]

; Context switching assembly functions
global save_context
global restore_context
global context_switch

; Save current CPU context to memory
; void save_context(cpu_context_t* context)
save_context:
    push ebp
    mov ebp, esp
    
    ; Get context pointer from argument
    mov edx, [ebp + 8]
    
    ; Save general purpose registers
    mov [edx + 0], eax     ; context->eax
    mov [edx + 4], ebx     ; context->ebx
    mov [edx + 8], ecx     ; context->ecx
    mov [edx + 12], edx    ; context->edx (will be overwritten, save original)
    mov eax, edx           ; save context pointer
    mov edx, [ebp + 8]     ; restore context pointer
    mov [eax + 12], edx    ; save original edx
    
    mov [edx + 16], esi    ; context->esi
    mov [edx + 20], edi    ; context->edi
    
    ; Save stack pointers
    mov [edx + 24], esp    ; context->esp
    mov [edx + 28], ebp    ; context->ebp
    
    ; Save instruction pointer (return address)
    mov eax, [ebp + 4]     ; get return address from stack
    mov [edx + 32], eax    ; context->eip
    
    ; Save flags
    pushf
    pop eax
    mov [edx + 36], eax    ; context->eflags
    
    ; Save segment registers
    mov ax, cs
    mov [edx + 40], ax     ; context->cs
    mov ax, ds
    mov [edx + 44], ax     ; context->ds
    mov ax, es
    mov [edx + 48], ax     ; context->es
    mov ax, fs
    mov [edx + 52], ax     ; context->fs
    mov ax, gs
    mov [edx + 56], ax     ; context->gs
    mov ax, ss
    mov [edx + 60], ax     ; context->ss
    
    mov esp, ebp
    pop ebp
    ret

; Restore CPU context from memory
; void restore_context(cpu_context_t* context)
restore_context:
    ; Get context pointer from stack (we can't use standard calling convention
    ; since we're changing all registers)
    mov edx, [esp + 4]
    
    ; Restore segment registers
    mov ax, [edx + 44]     ; context->ds
    mov ds, ax
    mov ax, [edx + 48]     ; context->es
    mov es, ax
    mov ax, [edx + 52]     ; context->fs
    mov fs, ax
    mov ax, [edx + 56]     ; context->gs
    mov gs, ax
    mov ax, [edx + 60]     ; context->ss
    mov ss, ax
    
    ; Restore stack pointers
    mov esp, [edx + 24]    ; context->esp
    mov ebp, [edx + 28]    ; context->ebp
    
    ; Restore flags
    mov eax, [edx + 36]    ; context->eflags
    push eax
    popf
    
    ; Push instruction pointer for iret
    push dword [edx + 40]  ; context->cs
    push dword [edx + 32]  ; context->eip
    
    ; Restore general purpose registers
    mov eax, [edx + 0]     ; context->eax
    mov ebx, [edx + 4]     ; context->ebx
    mov ecx, [edx + 8]     ; context->ecx
    mov esi, [edx + 16]    ; context->esi
    mov edi, [edx + 20]    ; context->edi
    ; Note: EDX is restored last since we're using it
    
    ; Save and restore EDX
    push dword [edx + 12]  ; context->edx
    pop edx
    
    ; Far return to restore CS:EIP
    retf

; Perform context switch between two processes
; void context_switch(process_t* old_process, process_t* new_process)
context_switch:
    push ebp
    mov ebp, esp
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    
    ; Get process pointers
    mov eax, [ebp + 8]     ; old_process
    mov ebx, [ebp + 12]    ; new_process
    
    ; Save current context to old process
    ; Offset to context field in process structure (assuming it's at offset 16)
    add eax, 16            ; &old_process->context
    push eax
    call save_context
    add esp, 4
    
    ; Restore context from new process
    ; Offset to context field in process structure
    add ebx, 16            ; &new_process->context
    push ebx
    call restore_context
    add esp, 4
    
    ; This point should not be reached in normal execution
    ; as restore_context should jump to the new process
    
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    mov esp, ebp
    pop ebp
    ret

; Simple process entry wrapper
; This is used to wrap process entry points
global process_entry_wrapper
process_entry_wrapper:
    ; Enable interrupts for the process
    sti
    
    ; Call the actual process function
    ; The function address should be set up by the scheduler
    call eax
    
    ; If process returns, terminate it
    ; In a real OS, this would call process_exit() or similar
.loop:
    hlt
    jmp .loop

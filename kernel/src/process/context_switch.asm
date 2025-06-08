[BITS 32]

; Context switching function
; void context_switch(process_regs_t* old_regs, process_regs_t* new_regs)
global context_switch

context_switch:
    ; Get parameters
    mov eax, [esp + 4]      ; old_regs pointer
    mov ebx, [esp + 8]      ; new_regs pointer
    
    ; Save current process state to old_regs
    test eax, eax
    jz load_new_context     ; Skip saving if old_regs is NULL
    
    ; Save general purpose registers
    mov [eax + 0], eax      ; Save EAX (will be overwritten, but that's ok)
    mov [eax + 4], ebx      ; Save EBX
    mov [eax + 8], ecx      ; Save ECX  
    mov [eax + 12], edx     ; Save EDX
    mov [eax + 16], esi     ; Save ESI
    mov [eax + 20], edi     ; Save EDI
    mov [eax + 24], esp     ; Save ESP
    mov [eax + 28], ebp     ; Save EBP
    
    ; Save EIP (return address)
    mov ecx, [esp]          ; Get return address from stack
    mov [eax + 32], ecx     ; Save EIP
    
    ; Save EFLAGS
    pushfd                  ; Push EFLAGS onto stack
    pop ecx                 ; Pop into ECX
    mov [eax + 36], ecx     ; Save EFLAGS
    
    ; Save segment registers
    mov cx, cs
    mov [eax + 40], cx      ; Save CS
    mov cx, ds  
    mov [eax + 42], cx      ; Save DS
    mov cx, es
    mov [eax + 44], cx      ; Save ES
    mov cx, fs
    mov [eax + 46], cx      ; Save FS
    mov cx, gs
    mov [eax + 48], cx      ; Save GS
    mov cx, ss
    mov [eax + 50], cx      ; Save SS

load_new_context:
    ; Load new process state from new_regs
    test ebx, ebx
    jz context_switch_done  ; Skip loading if new_regs is NULL
    
    ; Load segment registers first
    mov cx, [ebx + 42]      ; Load DS
    mov ds, cx
    mov cx, [ebx + 44]      ; Load ES
    mov es, cx
    mov cx, [ebx + 46]      ; Load FS
    mov fs, cx
    mov cx, [ebx + 48]      ; Load GS
    mov gs, cx
    mov cx, [ebx + 50]      ; Load SS
    mov ss, cx
    
    ; Load stack pointer
    mov esp, [ebx + 24]     ; Load ESP
    mov ebp, [ebx + 28]     ; Load EBP
    
    ; Load EFLAGS
    mov ecx, [ebx + 36]     ; Load EFLAGS
    push ecx                ; Push onto stack
    popfd                   ; Pop into EFLAGS register
    
    ; Load general purpose registers
    mov eax, [ebx + 0]      ; Load EAX
    mov ecx, [ebx + 8]      ; Load ECX
    mov edx, [ebx + 12]     ; Load EDX
    mov esi, [ebx + 16]     ; Load ESI
    mov edi, [ebx + 20]     ; Load EDI
    
    ; Load EIP and CS by setting up return
    push dword [ebx + 40]   ; Push CS
    push dword [ebx + 32]   ; Push EIP
    
    ; Load EBX last
    mov ebx, [ebx + 4]      ; Load EBX
    
    ; Far return to new process
    retf

context_switch_done:
    ret

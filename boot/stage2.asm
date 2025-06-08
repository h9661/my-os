; ============================================================================
; Stage 2 Bootloader - Second stage loaded by Stage 1
; ============================================================================
; Role: 1) Load kernel from disk 2) Set up GDT 3) Switch to protected mode
; 4) Jump to kernel
; This has more space (multiple sectors) for complex operations

[ORG 0x7E00]           ; Stage 2 loads at this address (right after Stage 1)

; === Constants ===
KERNEL_OFFSET equ 0x10000       ; Load kernel at 64KB (safe location)
KERNEL_SECTORS equ 97           ; Max sectors for kernel (97 * 512 = 49408 bytes)
KERNEL_START_SECTOR equ 11      ; Kernel starts at sector 11

; GDT segment selectors
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; === 16-bit Real Mode ===
[BITS 16]

; === Stage 2 Entry Point ===
stage2_start:
    ; === Critical: Set up segments and stack first ===
    cli                   ; Disable interrupts during setup
    xor ax, ax           ; Clear AX
    mov ds, ax           ; Data segment = 0
    mov es, ax           ; Extra segment = 0  
    mov ss, ax           ; Stack segment = 0
    
    ; Setup stack BEFORE enabling interrupts
    mov bp, 0x9000       ; Stack base at 36KB  
    mov sp, bp           ; Stack pointer = base
    sti                  ; Re-enable interrupts AFTER stack setup
    
    ; Save boot drive number (passed from Stage 1 in DL)
    mov [boot_drive], dl
    
    ; === Immediate Debug: Show we reached Stage 2 ===
    ; Write directly to VGA memory first to ensure we can see output
    mov ax, 0xB800          ; VGA text mode segment
    mov es, ax
    mov word [es:0], 0x0F32 ; "2" in white on black
    mov word [es:2], 0x0F53 ; "S" in white on black
    
    ; Reset ES to 0
    xor ax, ax
    mov es, ax
    
    ; === Display Stage 2 Banner ===
    mov si, msg_stage2_start
    call print_string
    
    ; === Enable A20 Line First ===
    call enable_a20
    
    ; === Load Kernel from Disk (in 16-bit mode) ===
    call load_kernel_16bit
    
    ; === Switch to Protected Mode ===
    call switch_to_protected_mode
    
    ; Should never reach here
    jmp $

; === Enable A20 Line ===
; Enables access to memory above 1MB
enable_a20:
    mov si, msg_enabling_a20
    call print_string
    
    ; Method 1: Fast A20 (most reliable)
    in al, 0x92
    or al, 2
    out 0x92, al
    
    ret

; === Load Kernel in 16-bit Mode (using BIOS) ===
load_kernel_16bit:
    mov si, msg_loading_kernel
    call print_string
    
    ; Setup parameters for kernel loading
    ; Set ES:BX to point to KERNEL_OFFSET (0x10000)
    mov ax, KERNEL_OFFSET >> 4  ; Segment = 0x1000
    mov es, ax
    mov bx, 0                   ; Offset = 0 (ES:BX = 0x1000:0x0000 = 0x10000)
    mov dh, KERNEL_SECTORS      ; Number of sectors (85)
    mov dl, [boot_drive]        ; Boot drive number
    mov ch, 0                   ; Cylinder 0
    mov cl, KERNEL_START_SECTOR ; Starting sector (11)

    ; Load kernel using BIOS disk services
    call disk_load_extended
    
    ; Reset ES to 0
    xor ax, ax
    mov es, ax
    
    mov si, msg_kernel_loaded
    call print_string
    ret

; === Extended Disk Load Function for Large Reads ===
disk_load_extended:
    pusha
    
    ; For large reads, we need to load in chunks due to BIOS limitations
    mov al, dh              ; Number of sectors to read
    mov ah, 0x02            ; BIOS read sectors function
    mov dh, 0               ; Head number = 0
    ; CH, CL, BX, DL already set by caller
    
    int 0x13                ; BIOS disk interrupt
    jc .disk_error          ; Jump if carry flag set (error)
    
    popa
    ret

.disk_error:
    mov si, msg_disk_error
    call print_string
    cli
    hlt

; Store boot drive number
boot_drive: db 0

; === Switch to Protected Mode ===
switch_to_protected_mode:
    mov si, msg_switching_pm
    call print_string
    
    cli                           ; Disable interrupts
    
    lgdt [gdt_descriptor]         ; Load GDT
    
    ; Set PE bit in CR0
    mov eax, cr0
    or al, 1
    mov cr0, eax
    
    ; Far jump to flush pipeline and load CS
    jmp CODE_SEG:init_pm

; === 32-bit Protected Mode ===
[BITS 32]

init_pm:
    ; Set up segment registers
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Set up 32-bit stack
    mov ebp, 0x90000              ; 576KB stack base
    mov esp, ebp
    
    ; Display protected mode message
    mov dword [0xB8000], 0x2F4B2F4F   ; "OK" in green
    mov dword [0xB8004], 0x2F4D2F50   ; "PM" in green (Protected Mode)
    
    ; Jump to kernel (kernel was already loaded in 16-bit mode)
    jmp KERNEL_OFFSET

; Print string function
print_string:
    pusha
.loop:
    lodsb
    cmp al, 0
    je .done
    mov ah, 0x0E
    mov bh, 0
    mov bl, 0x07
    int 0x10
    jmp .loop
.done:
    popa
    ret

; === GDT (Global Descriptor Table) ===
gdt_start:
    ; Null descriptor (required)
    dd 0x0
    dd 0x0

gdt_code:
    ; Code segment descriptor
    ; Base=0, Limit=0xFFFFF, Access=10011010b, Flags=1100b
    dw 0xFFFF       ; Limit (0-15)
    dw 0x0          ; Base (0-15)
    db 0x0          ; Base (16-23)
    db 10011010b    ; Access byte (present, DPL=0, code, exec/read)
    db 11001111b    ; Flags (4-bit) + Limit (16-19)
    db 0x0          ; Base (24-31)

gdt_data:
    ; Data segment descriptor
    ; Base=0, Limit=0xFFFFF, Access=10010010b, Flags=1100b
    dw 0xFFFF       ; Limit (0-15)
    dw 0x0          ; Base (0-15)
    db 0x0          ; Base (16-23)
    db 10010010b    ; Access byte (present, DPL=0, data, read/write)
    db 11001111b    ; Flags (4-bit) + Limit (16-19)
    db 0x0          ; Base (24-31)

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1    ; GDT size
    dd gdt_start                  ; GDT base address

; Messages
msg_stage2_start:   db "Stage 2 Bootloader Started", 0x0D, 0x0A, 0
msg_debug_stage2:   db "DEBUG: Stage 2 initialized, stack setup complete", 0x0D, 0x0A, 0
msg_loading_kernel: db "Loading kernel...", 0x0D, 0x0A, 0
msg_kernel_loaded:  db "Kernel loaded successfully!", 0x0D, 0x0A, 0
msg_debug_kernel_done: db "DEBUG: Kernel loading phase completed", 0x0D, 0x0A, 0
msg_enabling_a20:   db "Enabling A20 line...", 0x0D, 0x0A, 0
msg_switching_pm:   db "Switching to Protected Mode...", 0x0D, 0x0A, 0
msg_disk_error:     db "Disk read error in Stage 2!", 0x0D, 0x0A, 0

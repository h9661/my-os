; Bootloader - 16-bit Real Mode to 32-bit Protected Mode
; Loads kernel from disk and switches to protected mode

[ORG 0x7C00]           ; BIOS loads boot sector here

; Constants
KERNEL_OFFSET equ 0x1000    ; Load kernel at 4KB
KERNEL_SECTORS equ 50       ; Load 30 sectors (15KB)

; GDT segment selectors
CODE_SEG equ gdt_code - gdt_start    ; Code segment selector (0x08)
DATA_SEG equ gdt_data - gdt_start    ; Data segment selector (0x10)

; 16-bit Real Mode
[BITS 16]

; Stage 1: Initialize environment
start:
    ; Clear segment registers for safe memory access
    xor ax, ax         ; AX = 0
    mov ds, ax         ; Data segment = 0
    mov es, ax         ; Extra segment = 0
    
    ; Setup stack at safe location (36KB)
    mov bp, 0x9000     ; Stack base pointer
    mov sp, bp         ; Stack pointer
    
    ; Save boot drive (BIOS passes drive number in DL)
    mov [BOOT_DRIVE], dl
    
    ; Print startup message
    mov si, MSG_BOOT_START
    call print_string
    
    ; Continue to stage 2

; Stage 2: Load kernel from disk
load_kernel:
    mov si, MSG_LOADING_KERNEL
    call print_string
    
    ; Load kernel sectors to memory
    mov bx, KERNEL_OFFSET     ; Target memory address
    mov dh, KERNEL_SECTORS    ; Number of sectors
    mov dl, [BOOT_DRIVE]      ; Boot drive number
    call disk_load            ; Call disk load function
    
    mov si, MSG_KERNEL_LOADED
    call print_string
    
    ; Continue to stage 3

; Stage 3: Enable A20 line for >1MB memory access
enable_a20:
    ; Fast A20 method
    in al, 0x92           ; Read system control port A
    or al, 2              ; Set A20 bit
    out 0x92, al          ; Write back to port

; Stage 4: Switch to 32-bit Protected Mode
switch_to_pm:
    cli                   ; Disable interrupts during mode switch
    
    ; Load GDT
    lgdt [gdt_descriptor]
    
    ; Set PE (Protection Enable) bit in CR0
    mov eax, cr0          ; Read CR0 register
    or al, 1              ; Set PE bit (bit 0)
    mov cr0, eax          ; Write back to CR0
    
    ; Far jump to flush pipeline and update code segment
    jmp CODE_SEG:init_pm  ; Jump to 32-bit protected mode code

; 32-bit Protected Mode
[BITS 32]

init_pm:
    ; Set all segment registers to data segment
    mov ax, DATA_SEG      ; Load data segment selector
    mov ds, ax            ; Data segment
    mov ss, ax            ; Stack segment  
    mov es, ax            ; Extra segment
    mov fs, ax            ; Additional segment
    mov gs, ax            ; Additional segment
    
    ; Setup 32-bit stack at 576KB
    mov ebp, 0x90000      ; 32-bit stack base
    mov esp, ebp          ; 32-bit stack pointer
    
    ; Write "OK" to VGA memory to show we're in protected mode
    mov dword [0xB8000], 0x4F4B4F4F    ; "OK" (red background)
    
    ; Jump to kernel
    jmp KERNEL_OFFSET

; Functions

; Print string in 16-bit mode (uses BIOS)
print_string:
    pusha                 ; Save all registers
.loop:
    lodsb                 ; Load byte from SI to AL, increment SI
    cmp al, 0             ; Check for null terminator
    je .done              ; If null, we're done
    mov ah, 0x0e          ; BIOS teletype service
    mov bh, 0             ; Page number
    mov bl, 0x07          ; Character attribute (white on black)
    int 0x10              ; BIOS video interrupt
    jmp .loop
.done:
    popa                  ; Restore all registers
    ret

; Include external files
%include "boot/disk.asm"     ; Disk I/O functions
%include "boot/gdt.asm"      ; GDT setup

; Messages - shortened to save space
MSG_BOOT_START      db "Boot...", 0x0D, 0x0A, 0
MSG_LOADING_KERNEL  db "Load kernel...", 0x0D, 0x0A, 0
MSG_KERNEL_LOADED   db "OK", 0x0D, 0x0A, 0

; Variables
BOOT_DRIVE db 0           ; Store boot drive number

; Boot signature - must be 512 bytes with 0x55AA at the end
times 510-($-$$) db 0     ; Fill to 510 bytes with zeros
dw 0xAA55                 ; Boot signature (BIOS recognizes valid boot sector)

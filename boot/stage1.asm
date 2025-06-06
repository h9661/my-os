; ============================================================================
; Stage 1 Bootloader - First stage loaded by BIOS (512 bytes)
; ============================================================================
; Role: 1) Initialize basic environment 2) Load Stage 2 bootloader 3) Jump to Stage 2
; This is the minimal bootloader that BIOS loads from the MBR (Master Boot Record)

[ORG 0x7C00]           ; BIOS loads boot sector at this address

; === Constants ===
STAGE2_OFFSET equ 0x7E00        ; Load Stage 2 right after Stage 1 (512 bytes later)
STAGE2_SECTORS equ 1            ; Stage 2 size in sectors (4KB maximum)
STAGE2_START_SECTOR equ 3       ; Stage 2 starts at sector 2 (matches create_hdd.sh)

; === 16-bit Real Mode ===
[BITS 16]

; === Stage 1 Entry Point ===
stage1_start:
    ; === Initialize Environment ===
    ; Clear segment registers for consistent memory access
    cli                    ; Disable interrupts during setup
    xor ax, ax            ; AX = 0
    mov ds, ax            ; Data segment = 0 (0x0000-0xFFFF)
    mov es, ax            ; Extra segment = 0
    mov ss, ax            ; Stack segment = 0
    
    ; Setup stack below our bootloader (grows downward)
    mov sp, 0x7C00        ; Stack pointer just below bootloader
    sti                   ; Re-enable interrupts
    
    ; Save boot drive number (BIOS passes in DL)
    mov [boot_drive], dl
    
    ; === Display Stage 1 Message ===
    mov si, msg_stage1
    call print_string
    
    ; === Load Stage 2 Bootloader ===
    mov si, msg_loading_stage2
    call print_string
    
    ; Setup parameters for disk read
    mov bx, STAGE2_OFFSET      ; ES:BX = target memory address
    mov dh, STAGE2_SECTORS     ; Number of sectors to read
    mov dl, [boot_drive]       ; Drive number
    mov ch, 0                  ; Cylinder 0
    mov cl, STAGE2_START_SECTOR ; Starting sector
    
    call disk_load
    
    ; === Jump to Stage 2 ===
    mov si, msg_stage2_loaded
    call print_string
    
    ; Pass boot drive to Stage 2 in DL register
    mov dl, [boot_drive]
    jmp STAGE2_OFFSET         ; Jump to Stage 2 bootloader

; === Disk Load Function ===
; Load sectors from disk to memory
; Input: BX=target address, DH=sectors to read, DL=drive, CH=cylinder, CL=sector
disk_load:
    pusha                     ; Save all registers
    
    mov ah, 0x02             ; BIOS read sectors function
    mov al, dh               ; Number of sectors to read
    mov dh, 0                ; Head number = 0
    ; CH, CL, BX, DL already set by caller
    
    int 0x13                 ; BIOS disk interrupt
    jc disk_error            ; Jump if carry flag set (error)
    
    popa                     ; Restore all registers
    ret

disk_error:
    mov si, msg_disk_error
    call print_string
    ; Halt system on disk error
    cli
    hlt

; === Print String Function ===
; Print null-terminated string using BIOS
; Input: SI = pointer to string
print_string:
    pusha                    ; Save all registers
.loop:
    lodsb                    ; Load byte from [SI] to AL, increment SI
    cmp al, 0               ; Check for null terminator
    je .done                ; If null, we're done
    
    mov ah, 0x0E            ; BIOS teletype output function
    mov bh, 0               ; Page number
    mov bl, 0x07            ; Text attribute (white on black)
    int 0x10                ; BIOS video interrupt
    jmp .loop
.done:
    popa                    ; Restore all registers
    ret

; === Data Section ===
boot_drive:        db 0     ; Storage for boot drive number

; Messages (keep short to save space in 512 bytes)
msg_stage1:        db "Stage1", 0x0D, 0x0A, 0
msg_loading_stage2: db "Loading Stage2...", 0x0D, 0x0A, 0
msg_stage2_loaded: db "Stage2 loaded!", 0x0D, 0x0A, 0
msg_disk_error:    db "Disk Error!", 0x0D, 0x0A, 0

; === Boot Sector Padding and Signature ===
; Fill remaining space to make exactly 512 bytes
times 510-($-$$) db 0     ; Pad with zeros to byte 510
dw 0xAA55                 ; Boot signature (BIOS requirement)

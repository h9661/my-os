; Disk I/O Functions
; BIOS interrupt-based disk reading (16-bit Real Mode only)

; Load sectors from disk to memory
; Parameters: BX = target memory address, DH = sectors to read, DL = drive number
disk_load:
    push dx               ; Save original parameters for error checking
    
    mov ah, 0x02          ; BIOS read function
    mov al, dh            ; Number of sectors to read
    mov ch, 0             ; Cylinder (track) number = 0
    mov dh, 0             ; Head number = 0  
    mov cl, 2             ; Sector number = 2 (skip boot sector)
    ; ES:BX already set by caller (target address)
    
    int 0x13              ; BIOS disk service interrupt
    
    jc disk_error         ; Jump if carry flag set (error occurred)
    
    pop dx                ; Restore original DX
    cmp dh, al            ; Compare requested vs actual sectors read
    jne disk_error        ; Jump if not equal (error)
    
    ret                   ; Success

; Handle disk read errors
disk_error:
    mov si, DISK_ERROR_MSG
    call print_string     ; Print error message
    
    mov dh, ah            ; Copy error code to DH
    call print_hex        ; Print error code in hex
    
    jmp $                 ; Infinite loop (halt system)

; Print byte value in hexadecimal
; Parameter: DH = byte value to print
print_hex:
    pusha                 ; Save all registers
    mov cx, 4             ; Process 4 bits at a time (2 nibbles per byte)
    
.hex_loop:
    dec cx                ; Decrement counter
    mov ax, dx            ; Copy DX to AX
    shr ax, cl            ; Shift right by CX*4 bits
    shr ax, cl
    shr ax, cl  
    shr ax, cl
    and ax, 0x0F          ; Keep only lower 4 bits
    
    add al, '0'           ; Convert to ASCII digit
    cmp al, '9'           ; Check if greater than 9
    jle .print_char       ; If ≤ 9, print directly
    add al, 7             ; Convert to A-F (add 7 for ASCII adjustment)
    
.print_char:
    mov ah, 0x0e          ; BIOS teletype service
    mov bh, 0             ; Page number
    mov bl, 0x07          ; Character attribute
    int 0x10              ; BIOS video interrupt
    
    cmp cx, 0             ; Check if all nibbles processed
    jne .hex_loop         ; Continue if more to process
    
    popa                  ; Restore all registers
    ret

; Error message
DISK_ERROR_MSG db "Disk read error! Error code: ", 0

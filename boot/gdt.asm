; Global Descriptor Table (GDT) Setup
; Defines memory segments for 32-bit Protected Mode

gdt_start:

; NULL Descriptor - Required first entry
gdt_null:
    dd 0x0                ; 8 bytes of zeros
    dd 0x0

; Code Segment Descriptor - Executable memory
gdt_code:
    dw 0xFFFF             ; Segment limit (0-15)
    dw 0x0000             ; Base address (0-15)
    db 0x00               ; Base address (16-23)
    db 10011010b          ; Access byte: Present, Ring 0, Executable, Readable
    db 11001111b          ; Flags: 4KB granularity, 32-bit, Limit (16-19)
    db 0x00               ; Base address (24-31)

; Data Segment Descriptor - Read/Write memory
gdt_data:
    dw 0xFFFF             ; Segment limit (0-15)
    dw 0x0000             ; Base address (0-15)
    db 0x00               ; Base address (16-23)
    db 10010010b          ; Access byte: Present, Ring 0, Writable
    db 11001111b          ; Flags: 4KB granularity, 32-bit, Limit (16-19)
    db 0x00               ; Base address (24-31)

gdt_end:

; GDT Descriptor - Points to GDT for LGDT instruction
gdt_descriptor:
    dw gdt_end - gdt_start - 1    ; GDT size in bytes minus 1
    dd gdt_start                  ; GDT base address

; Segment selectors: CODE_SEG = 0x08, DATA_SEG = 0x10

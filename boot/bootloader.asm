[BITS 16]
[ORG 0x7C00]

mov ax, 0
mov ds, ax
mov es, ax

mov bp, 0x9000
mov sp, bp

mov [boot_drive], dl

mov si, MSG_REAL_MODE
call print_string
mov bx, 0x1000
mov dh, 50
mov dl, [boot_drive]

call disk_load

mov si, MSG_LOAD_KERNEL
call print_string

cli
xor ax, ax
mov ds, ax
mov es, ax

lgdt [gdt_descriptor]

mov eax, cr0
or al, 1
mov cr0, eax

jmp CODE_SEG:init_pm

%include "boot/gdt.asm"
%include "boot/disk.asm"

[BITS 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    mov ebp, 0x90000
    mov esp, ebp
    
    call BEGIN_PM

BEGIN_PM:
    mov ebx, MSG_PROT_MODE
    call print_string_pm
    
    call clear_screen_pm
    
    mov ebx, MSG_JUMPING_KERNEL
    call print_string_pm
    
    mov dword [0xb8000], 0x4F524F42
    
    jmp 0x1000

clear_screen_pm:
    pusha
    mov edx, 0xb8000
    mov ecx, 2000
    mov ax, 0x0700
.clear_loop:
    mov [edx], ax
    add edx, 2
    dec ecx
    jnz .clear_loop
    popa
    ret

print_string_pm:
    pusha
    mov edx, 0xb8000
.loop:
    mov al, [ebx]
    mov ah, 0x0F
    cmp al, 0
    je .done
    mov [edx], ax
    add ebx, 1
    add edx, 2
    jmp .loop
.done:
    popa
    ret

MSG_REAL_MODE db "Started in 16-bit Real Mode", 0x0D, 0x0A, 0
MSG_LOAD_KERNEL db "Loading kernel into memory", 0x0D, 0x0A, 0
MSG_PROT_MODE db "Successfully switched to 32-bit Protected Mode", 0
MSG_JUMPING_KERNEL db "Jumping to kernel at 0x1000...", 0

times 510-($-$$) db 0
dw 0xAA55
boot_drive db 0

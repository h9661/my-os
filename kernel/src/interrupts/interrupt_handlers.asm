[BITS 32]

; 외부 C 핸들러 함수 선언
extern c_irq_handler_timer
extern c_irq_handler_keyboard

; IRQ 핸들러 전역 선언
global irq_handler_timer
global irq_handler_keyboard

; 타이머 IRQ 핸들러 (IRQ0)
irq_handler_timer:
    cli                ; 인터럽트 비활성화
    pusha              ; 모든 범용 레지스터를 스택에 저장
    
    mov ax, ds         ; 데이터 세그먼트 저장
    push eax
    
    mov ax, 0x10       ; 커널 데이터 세그먼트 로드
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    call c_irq_handler_timer   ; C 핸들러 함수 호출
    
    pop eax            ; 데이터 세그먼트 복원
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa               ; 레지스터 복원
    sti                ; 인터럽트 활성화
    iret               ; 인터럽트에서 복귀

; 키보드 IRQ 핸들러 (IRQ1)
irq_handler_keyboard:
    cli                ; 인터럽트 비활성화
    pusha              ; 모든 범용 레지스터를 스택에 저장
    
    mov ax, ds         ; 데이터 세그먼트 저장
    push eax
    
    mov ax, 0x10       ; 커널 데이터 세그먼트 로드
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    call c_irq_handler_keyboard   ; C 핸들러 함수 호출
    
    pop eax            ; 데이터 세그먼트 복원
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa               ; 레지스터 복원
    sti                ; 인터럽트 활성화
    iret               ; 인터럽트에서 복귀
    cli                ; 인터럽트 비활성화
    pusha              ; 모든 범용 레지스터를 스택에 저장
    
    mov ax, ds         ; 데이터 세그먼트 저장
    push eax
    
    mov ax, 0x10       ; 커널 데이터 세그먼트 로드
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    call c_irq_handler_keyboard   ; C 핸들러 함수 호출
    
    pop eax            ; 데이터 세그먼트 복원
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa               ; 레지스터 복원
    sti                ; 인터럽트 활성화
    iret               ; 인터럽트에서 복귀
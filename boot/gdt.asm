; === GDT (Global Descriptor Table) 정의 ===
; 32비트 보호 모드에서 메모리 세그먼트를 관리하는 테이블
; 각 엔트리는 8바이트이며, 세그먼트의 기준 주소, 크기, 권한 등을 정의
; 보호 모드에서는 세그먼트 레지스터가 GDT의 인덱스(셀렉터)를 저장함

gdt_start:
    ; === NULL 디스크립터 (필수) ===
    ; GDT의 첫 번째 엔트리는 반드시 NULL이어야 함 (Intel CPU 아키텍처 요구사항)
    ; NULL 셀렉터(0x0000)로 세그먼트에 접근하면 General Protection Fault 발생
    dd 0x0         ; 하위 4바이트: 모두 0 (세그먼트 기준 주소, 크기 등 모두 무효)
    dd 0x0         ; 상위 4바이트: 모두 0 (접근 권한, 플래그 등 모두 무효)
    
gdt_code:          ; === 코드 세그먼트 디스크립터 ===
    ; 실행 가능한 코드가 저장되는 메모리 영역을 정의
    ; 이 세그먼트는 CPU 명령어들이 저장된 메모리 영역을 나타냄
    dw 0xffff      ; Limit (bits 0-15): 세그먼트 크기 하위 16비트 (최대값 0xFFFF)
                   ; 실제 크기는 Granularity 비트에 따라 결정됨 (G=1이면 4KB 단위)
    dw 0x0         ; Base (bits 0-15): 세그먼트 시작 주소 하위 16비트 (0x00000000)
                   ; 플랫 메모리 모델에서는 모든 세그먼트가 주소 0에서 시작
    db 0x0         ; Base (bits 16-23): 세그먼트 시작 주소 중간 8비트
    db 10011010b   ; Access byte: Present(1) DPL(00) S(1) Type(1010 = 실행/읽기 가능 코드)
                   ; Bit 7 (P): Present = 1 (세그먼트가 메모리에 존재함)
                   ; Bit 6-5 (DPL): Descriptor Privilege Level = 00 (커널 권한 레벨)
                   ; Bit 4 (S): Descriptor Type = 1 (코드/데이터 세그먼트, 시스템 세그먼트 아님)
                   ; Bit 3 (E): Executable = 1 (실행 가능한 코드 세그먼트)
                   ; Bit 2 (DC): Direction/Conforming = 0 (비준수 세그먼트)
                   ; Bit 1 (RW): Read/Write = 1 (읽기 가능, 코드는 항상 읽기만 가능)
                   ; Bit 0 (A): Accessed = 0 (CPU가 자동으로 설정, 초기값은 0)
    db 11001111b   ; Flags(1100) + Limit(1111): G=1(4KB 단위), D/B=1(32비트), L=0(비64비트), AVL=0 + 크기 상위 4비트
                   ; Bit 7 (G): Granularity = 1 (4KB 단위로 크기 계산, 0xFFFFF * 4KB = 4GB)
                   ; Bit 6 (D/B): Default operation size = 1 (32비트 연산 기본)
                   ; Bit 5 (L): Long mode = 0 (64비트 모드 아님, 32비트 모드)
                   ; Bit 4 (AVL): Available = 0 (운영체제가 자유롭게 사용 가능)
                   ; Bit 3-0: Limit bits 19-16 = 1111 (세그먼트 크기 상위 4비트)
    db 0x0         ; Base (bits 24-31): 세그먼트 시작 주소 상위 8비트
    
gdt_data:          ; === 데이터 세그먼트 디스크립터 ===
    ; 읽기/쓰기 가능한 데이터가 저장되는 메모리 영역을 정의
    ; 변수, 배열, 구조체 등 프로그램 데이터가 저장되는 영역
    dw 0xffff      ; Limit (bits 0-15): 세그먼트 크기 하위 16비트 (최대값 0xFFFF)
                   ; 코드 세그먼트와 동일하게 4GB 전체 메모리 접근 허용
    dw 0x0         ; Base (bits 0-15): 세그먼트 시작 주소 하위 16비트 (0x00000000)
                   ; 플랫 메모리 모델: 코드와 데이터 세그먼트가 같은 주소에서 시작
    db 0x0         ; Base (bits 16-23): 세그먼트 시작 주소 중간 8비트  
    db 10010010b   ; Access byte: Present(1) DPL(00) S(1) Type(0010 = 읽기/쓰기 가능 데이터)
                   ; Bit 7 (P): Present = 1 (세그먼트가 메모리에 존재함)
                   ; Bit 6-5 (DPL): Descriptor Privilege Level = 00 (커널 권한 레벨)
                   ; Bit 4 (S): Descriptor Type = 1 (코드/데이터 세그먼트)
                   ; Bit 3 (E): Executable = 0 (실행 불가능한 데이터 세그먼트)
                   ; Bit 2 (ED): Expansion Direction = 0 (위쪽으로 확장, 일반 데이터 세그먼트)
                   ; Bit 1 (W): Writable = 1 (쓰기 가능, 읽기는 항상 가능)
                   ; Bit 0 (A): Accessed = 0 (CPU가 자동으로 설정, 초기값은 0)
    db 11001111b   ; Flags(1100) + Limit(1111): G=1(4KB 단위), D/B=1(32비트), L=0(비64비트), AVL=0 + 크기 상위 4비트
                   ; 코드 세그먼트와 동일한 플래그 설정 (4GB, 32비트 모드)
    db 0x0         ; Base (bits 24-31): 세그먼트 시작 주소 상위 8비트
    
gdt_end:           ; GDT의 끝을 표시하는 라벨 (크기 계산에 사용)

; === GDT 디스크립터 ===
; CPU가 GDT의 위치와 크기를 알 수 있도록 하는 특별한 구조체
; LGDT 명령어가 이 구조체를 사용하여 GDT를 프로세서에 로드함
gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; GDT 크기 (바이트 수 - 1, Intel CPU 요구사항)
                                ; 실제 크기에서 1을 빼는 이유: CPU가 요구하는 형식
                                ; 예: 24바이트 GDT의 경우 23을 저장
    dd gdt_start                ; GDT 시작 주소 (32비트 물리 주소)
                                ; CPU가 GDT를 찾을 수 있는 메모리 주소

; === 세그먼트 셀렉터 상수 정의 ===
; 보호 모드에서 세그먼트 레지스터에 로드할 값들
; 세그먼트 셀렉터 = (GDT 인덱스 × 8) + RPL(Requested Privilege Level) + TI(Table Indicator)
; RPL: 요청된 권한 레벨 (0=커널, 3=사용자)
; TI: 테이블 지시자 (0=GDT, 1=LDT)
CODE_SEG equ gdt_code - gdt_start   ; 코드 세그먼트 셀렉터 (인덱스 1 × 8 + 0 + 0 = 0x08)
DATA_SEG equ gdt_data - gdt_start   ; 데이터 세그먼트 셀렉터 (인덱스 2 × 8 + 0 + 0 = 0x10)

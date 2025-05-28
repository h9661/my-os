; === 디스크 로딩 루틴 (하드 디스크용) ===
; BIOS 인터럽트 0x13을 사용하여 디스크 섹터를 메모리로 읽어오는 함수
; 입력 파라미터: BX = 목표 메모리 주소, DH = 읽을 섹터 수, DL = 드라이브 번호
; 출력: 성공 시 정상 복귀, 실패 시 에러 메시지 후 시스템 정지
disk_load:
    pusha                   ; 모든 범용 레지스터를 스택에 백업 (AX, BX, CX, DX, SI, DI, BP)
    push dx                 ; DX 레지스터 별도 백업 (나중에 섹터 수 비교용으로 사용)
    
    ; === BIOS 디스크 읽기 서비스 파라미터 설정 ===
    ; INT 0x13, AH=0x02: 디스크 섹터 읽기 서비스
    mov ah, 0x02            ; AH = 0x02 (BIOS 섹터 읽기 기능 번호)
    mov al, dh              ; AL = 읽을 섹터 수 (DH에서 복사, 1~128 범위)
    mov ch, 0               ; CH = 실린더 번호 (0번 실린더, 트랙 번호라고도 함)
    mov cl, 2               ; CL = 시작 섹터 번호 (2번, 1번은 부트섹터이므로 건너뜀)
                            ; 주의: 섹터 번호는 1부터 시작 (0이 아님)
    mov dh, 0               ; DH = 헤드 번호 (0번 헤드, 디스크의 면 번호)
    ; DL = 드라이브 번호 (이미 올바른 값이 설정되어 있음)
    ; 0x00~0x7F: 플로피 드라이브, 0x80~0xFF: 하드 드라이브
    
    ; === 로드 주소 설정 ===
    ; ES:BX = 목표 메모리 주소 (세그먼트:오프셋 방식)
    ; ES는 이미 0으로 설정되어 있으므로 물리 주소 = BX
    mov bx, 0x1000          ; BX = 0x1000 (오프셋, ES=0이므로 물리주소 0x1000)
    
    ; === BIOS 디스크 서비스 호출 ===
    int 0x13                ; BIOS 인터럽트 0x13 (디스크 서비스) 호출
                            ; 성공 시 CF=0, 실패 시 CF=1이며 AH에 에러 코드 저장
    jc .disk_error          ; CF(캐리 플래그) 설정시 에러 발생, 에러 처리로 분기
    
    ; === 읽기 결과 검증 ===
    ; BIOS는 실제로 읽은 섹터 수를 AL에 반환함
    pop dx                  ; 백업된 DX 복원 (원래 요청한 섹터 수가 DH에 있음)
    cmp dh, al              ; 요청한 섹터 수(DH)와 실제 읽은 섹터 수(AL) 비교
    jne .sectors_error      ; 다르면 섹터 수 불일치 에러로 분기
    
    popa                    ; 모든 범용 레지스터 복원 (PUSHA의 역순으로 복원)
    ret                     ; 호출자로 복귀 (성공)
    
; === 디스크 읽기 에러 처리 ===
; BIOS가 디스크 읽기 실패를 보고했을 때 실행되는 에러 핸들러
.disk_error:
    mov si, DISK_ERROR_MSG  ; SI에 디스크 에러 메시지 주소 로드
    call print_string       ; 에러 메시지를 화면에 출력
    mov dh, ah              ; AH의 BIOS 에러 코드를 DH로 복사 (print_hex 함수용)
                            ; BIOS 에러 코드 예시: 0x01=잘못된 명령, 0x04=섹터 없음, 0x08=DMA 오버런
    call print_hex          ; 16진수로 에러 코드 출력 (디버깅용)
    jmp $                   ; 무한 루프 (시스템 정지) - $는 현재 주소를 의미

; === 섹터 수 불일치 에러 처리 ===
; 요청한 섹터 수와 실제로 읽은 섹터 수가 다를 때 실행
.sectors_error:
    mov si, SECTORS_ERROR_MSG ; SI에 섹터 에러 메시지 주소 로드
    call print_string         ; 에러 메시지를 화면에 출력
    jmp $                     ; 무한 루프 (시스템 정지)

; === 에러 메시지 정의 ===
; null-terminated 문자열들 (C 스타일 문자열)
DISK_ERROR_MSG db "Disk read error! Error code: ", 0      ; 디스크 읽기 실패 메시지
SECTORS_ERROR_MSG db "Incorrect number of sectors read", 0 ; 섹터 수 불일치 메시지

; === 2자리 16진수 출력 함수 ===
; DH 레지스터의 값을 16진수 문자열로 화면에 출력하는 유틸리티 함수
; 입력: DH = 출력할 8비트 값 (0x00 ~ 0xFF 범위)
; 출력: 화면에 2자리 16진수 표시 (예: 0x4A → "4A")
print_hex:
    pusha                   ; 모든 범용 레지스터 백업 (함수 호출 규약)
    mov cx, 2               ; CX = 처리할 자릿수 (2자리: 상위 니블 + 하위 니블)
    
    ; === 16진수 변환 루프 ===
    ; 8비트 값을 4비트씩 나누어 각각을 ASCII 문자로 변환
.hex_loop:
    dec cx                  ; 자릿수 카운터 감소 (2 → 1 → 0)
    
    ; === 4비트 니블 추출 및 ASCII 변환 ===
    mov ax, dx              ; DX 값을 AX로 복사 (원본 보존)
    shr al, 4               ; AL을 4비트 오른쪽 시프트 (상위 4비트만 남김)
                            ; 예: 0x4A → 0x04 (상위 니블 추출)
    and al, 0xF             ; 하위 4비트만 마스킹 (0x0 ~ 0xF 범위 확보)
    
    ; === 숫자/문자 ASCII 변환 ===
    add al, '0'             ; '0' 문자 코드(0x30) 추가
                            ; 0~9: 0x30~0x39 ('0'~'9')
    cmp al, '9'             ; '9'(0x39)보다 큰지 확인
    jle .store_digit        ; 9 이하면 숫자이므로 바로 출력
    add al, 7               ; A-F로 변환: '9'+1+7='A'(0x41)
                            ; 10~15: 0x41~0x46 ('A'~'F')
    
.store_digit:
    ; === BIOS 텔레타입 서비스를 사용한 문자 출력 ===
    mov ah, 0x0E            ; AH = 0x0E (BIOS 텔레타입 출력 기능)
    int 0x10                ; BIOS 비디오 인터럽트 호출 - AL의 문자를 화면에 출력
    
    ; === 다음 니블 처리 준비 ===
    shl dx, 4               ; DX를 4비트 왼쪽 시프트 (다음 니블을 상위로 이동)
                            ; 예: 0x4A → 0xA0 (하위 니블이 상위로 이동)
    
    cmp cx, 0               ; 모든 자릿수를 처리했는지 확인
    jne .hex_loop           ; 아직 남아있으면 루프 계속
    
    popa                    ; 모든 범용 레지스터 복원
    ret                     ; 호출자로 복귀

; === 16비트 모드용 문자열 출력 함수 ===
; null-terminated 문자열을 BIOS 서비스를 사용하여 화면에 출력
; 입력: SI = 출력할 문자열의 주소 (null로 끝나는 문자열)
; 출력: 화면에 문자열 표시
print_string:
    pusha                   ; 모든 범용 레지스터 백업
    mov ah, 0x0E            ; AH = 0x0E (BIOS 텔레타입 출력 기능)
                            ; 이 기능은 문자를 현재 커서 위치에 출력하고 커서를 이동
.loop:
    lodsb                   ; LODSB: SI가 가리키는 바이트를 AL에 로드하고 SI 증가
                            ; DS:SI → AL, SI = SI + 1 (문자열을 순차적으로 읽음)
    cmp al, 0               ; AL이 null terminator(0)인지 확인
    je .done                ; null이면 문자열 끝이므로 함수 종료
    int 0x10                ; BIOS 비디오 인터럽트 호출 - AL의 문자를 화면에 출력
    jmp .loop               ; 다음 문자 처리를 위해 루프 계속
.done:
    popa                    ; 모든 범용 레지스터 복원
    ret                     ; 호출자로 복귀

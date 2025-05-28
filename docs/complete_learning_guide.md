# 🎓 Complete OS Development Learning Guide

## 📖 Overview
이 가이드는 x86 아키텍처에서 간단한 운영체제를 개발하는 전체 과정을 단계별로 설명합니다. 각 파일과 개념을 심층적으로 이해할 수 있도록 구성되었습니다.

## 🗂️ Project Structure
```
my-os/
├── boot/
│   ├── bootloader.asm    # 부트로더 메인 코드 (16→32bit 전환)
│   ├── disk.asm          # 디스크 읽기 및 출력 함수들
│   └── gdt.asm           # Global Descriptor Table 정의
├── kernel/
│   ├── kernel.c          # C로 작성된 커널 메인 로직
│   ├── kernel_entry.asm  # 32bit 커널 진입점
│   └── kernel.ld         # 링커 스크립트 (메모리 레이아웃)
├── docs/
│   ├── bootloader_detailed_guide.md
│   ├── bootloader_execution_flow.md
│   └── complete_learning_guide.md (이 파일)
├── build/               # 빌드 결과물들
├── Makefile            # 빌드 규칙 정의
└── create_hdd.sh       # 디스크 이미지 생성 스크립트
```

## 🚀 Learning Path

### 1️⃣ 기초 개념 이해

#### CPU 모드와 메모리 모델
- **Real Mode (16-bit)**: BIOS가 실행되는 모드, 1MB 메모리 제한
- **Protected Mode (32-bit)**: 현대적 OS의 기본 모드, 4GB 메모리 접근
- **Segmentation**: 메모리를 논리적 세그먼트로 분할하는 방식
- **GDT (Global Descriptor Table)**: 세그먼트 정보를 저장하는 테이블

#### 부팅 프로세스
1. **Power-On**: CPU가 리셋 벡터(0xFFFF0)에서 실행 시작
2. **BIOS**: 하드웨어 초기화 및 부트 디바이스 탐색
3. **MBR Loading**: 첫 번째 섹터(512바이트)를 0x7C00에 로드
4. **Bootloader**: 커널을 메모리에 로드하고 protected mode 전환
5. **Kernel**: 운영체제 커널이 시스템 제어권을 갖게 됨

### 2️⃣ 파일별 학습 순서

#### Step 1: GDT 이해 (`boot/gdt.asm`)
```asm
; 학습 포인트:
; - 세그먼트 디스크립터 구조 (8바이트)
; - Access Byte의 각 비트 의미
; - Flags의 DB/G 비트 역할
; - 코드/데이터 세그먼트 차이점
```

**핵심 개념:**
- Null 디스크립터 (첫 번째는 항상 0)
- Code Segment: 실행 가능한 코드용
- Data Segment: 데이터 읽기/쓰기용
- Flat Memory Model: 모든 세그먼트가 0-4GB 커버

#### Step 2: 디스크 I/O 이해 (`boot/disk.asm`)
```asm
; 학습 포인트:
; - BIOS INT 0x13 서비스 사용법
; - CHS (Cylinder-Head-Sector) 주소 체계
; - 에러 처리 및 재시도 로직
; - 16진수 출력 함수 구현
```

**핵심 개념:**
- AH=0x02: 디스크 읽기 서비스
- DL 레지스터: 드라이브 번호 (0x80 = 첫 번째 하드 디스크)
- CHS → LBA 변환 공식
- 캐리 플래그로 오류 검출

#### Step 3: 부트로더 메인 (`boot/bootloader.asm`)
```asm
; 학습 포인트:
; - 16비트 리얼 모드에서 시작
; - 스택 설정의 중요성
; - 커널 로딩 과정
; - A20 라인 활성화
; - Protected Mode 전환 시퀀스
```

**핵심 개념:**
- `[ORG 0x7C00]`: 부트로더가 로드되는 메모리 주소
- Stack Pointer 설정으로 안전한 함수 호출 환경 구축
- `lgdt [gdt_descriptor]`: GDT 로드
- `cr0` 레지스터 조작으로 Protected Mode 진입
- Far Jump로 CS 레지스터 갱신

#### Step 4: 커널 진입점 (`kernel/kernel_entry.asm`)
```asm
; 학습 포인트:
; - 32비트 Protected Mode 환경
; - VGA 메모리 직접 조작
; - C 함수 호출 규약
; - 시스템 종료 처리
```

**핵심 개념:**
- `[BITS 32]`: 32비트 명령어 사용
- VGA 텍스트 모드 메모리 (0xB8000)
- C calling convention (cdecl)
- HLT 명령어로 CPU 정지

#### Step 5: C 커널 (`kernel/kernel.c`)
```c
// 학습 포인트:
// - 32비트 환경에서의 C 프로그래밍
// - 하드웨어 직접 제어
// - 메모리 매핑된 I/O
// - 무한 루프로 시스템 유지
```

### 3️⃣ 빌드 프로세스 이해

#### 툴체인 구성요소
1. **NASM**: x86 어셈블리 어셈블러
2. **i686-elf-gcc**: 크로스 컴파일러 (타겟: i686 ELF)
3. **i686-elf-ld**: ELF 링커
4. **i686-elf-objcopy**: 바이너리 형식 변환 도구

#### 빌드 단계 (`Makefile`)
```makefile
# 1. 어셈블리 → 오브젝트 파일
bootloader.bin: bootloader.asm
    nasm -f bin bootloader.asm -o bootloader.bin

# 2. C 소스 → 오브젝트 파일  
kernel.o: kernel.c
    i686-elf-gcc -ffreestanding -c kernel.c -o kernel.o

# 3. 링킹 → 실행 파일
kernel.bin: kernel.o kernel_entry.o
    i686-elf-ld -T kernel.ld kernel_entry.o kernel.o -o kernel.bin

# 4. ELF → Raw Binary 변환
    i686-elf-objcopy -O binary kernel.bin kernel.bin
```

#### 디스크 이미지 생성 (`create_hdd.sh`)
```bash
# 1. 빈 디스크 이미지 생성
dd if=/dev/zero of=hard-disk.img bs=1M count=10

# 2. 부트로더를 첫 번째 섹터에 복사
dd if=bootloader.bin of=hard-disk.img conv=notrunc

# 3. 커널을 두 번째 섹터부터 복사
dd if=kernel.bin of=hard-disk.img bs=512 seek=1 conv=notrunc
```

### 4️⃣ 메모리 레이아웃 이해

#### 부팅 시 메모리 맵
```
0x00000000 |----------------| 
           | IVT (인터럽트  |  0-1KB
           | 벡터 테이블)   |
0x00000400 |----------------|
           | BIOS Data Area |  1-2KB
0x00000500 |----------------|
           | 사용 가능      |  2KB-639KB
           | 메모리        |
0x0009FC00 |----------------|
           | EBDA          |  639-640KB
0x000A0000 |----------------|
           | 비디오 메모리  |  640KB-1MB
0x000B8000 | (VGA 텍스트)   |
0x000C0000 |----------------|
           | BIOS ROM      |
0x00100000 |----------------|
           | 확장 메모리    |  1MB+
```

#### 우리 OS의 메모리 사용
```
0x00001000 |----------------| ← 커널 로드 위치
           | Kernel Code    |
           |                |
0x0007C000 |----------------|
           | 사용 가능      |
0x0007C00  |----------------| ← 부트로더 로드 위치
           | Bootloader     |
0x0007E00  |----------------|
           | 사용 가능      |
0x00090000 |----------------| ← 스택 시작점
           | Stack (grows   |   (아래쪽으로 성장)
           | downward)      |
```

### 5️⃣ 디버깅 및 테스트

#### QEMU를 사용한 테스트
```bash
# 기본 실행
qemu-system-i386 -drive format=raw,file=build/hard-disk.img

# 디버그 모드 (GDB 연결 가능)
qemu-system-i386 -drive format=raw,file=build/hard-disk.img -s -S

# 직렬 포트 출력 리다이렉션
qemu-system-i386 -drive format=raw,file=build/hard-disk.img -serial stdio
```

#### 일반적인 문제들과 해결법

1. **부팅이 안 됨**
   - 부트 시그니처 (0xAA55) 확인
   - 첫 번째 섹터가 512바이트인지 확인
   - QEMU에서 `-drive format=raw` 옵션 사용

2. **커널이 로드되지 않음**
   - 디스크 읽기 에러 처리 확인
   - CHS 주소 계산 검증
   - 커널 크기가 로드 가능한 범위 내인지 확인

3. **Protected Mode 전환 실패**
   - GDT 구조 검증
   - A20 라인 활성화 확인
   - CR0 레지스터 설정 검토

### 6️⃣ 확장 학습 주제

#### 고급 주제들
1. **인터럽트 처리**: IDT, ISR 구현
2. **메모리 관리**: 페이징, 가상 메모리
3. **파일 시스템**: FAT12/16, 디렉토리 구조
4. **프로세스 관리**: 멀티태스킹, 스케줄링
5. **디바이스 드라이버**: 키보드, 마우스, 네트워크

#### 추가 학습 자료
- Intel x86 아키텍처 매뉴얼
- OSDev.org 위키
- "Operating Systems: Design and Implementation" - Andrew Tanenbaum
- "Operating System Concepts" - Abraham Silberschatz

## 🎯 학습 목표 달성 체크리스트

- [ ] 16비트 리얼 모드와 32비트 보호 모드의 차이점 이해
- [ ] 부트 프로세스의 각 단계 설명 가능
- [ ] GDT의 구조와 역할 완전 이해
- [ ] 어셈블리 코드의 각 라인이 하는 일 설명 가능
- [ ] C 커널과 어셈블리 부트로더의 연결 방식 이해
- [ ] 메모리 레이아웃과 주소 지정 방식 파악
- [ ] 빌드 프로세스의 각 단계 이해
- [ ] 디스크 이미지 구조와 부팅 메커니즘 이해
- [ ] 기본적인 디버깅 방법 숙지
- [ ] 다음 단계 확장 계획 수립

## 🌟 마무리

이 OS 개발 프로젝트는 컴퓨터 시스템의 가장 기본적인 동작 원리를 이해하는 데 도움이 됩니다. 각 파일의 상세한 주석과 이 가이드를 통해 x86 아키텍처, 부트 프로세스, 시스템 프로그래밍의 기초를 탄탄히 다질 수 있습니다.

계속해서 더 복잡한 기능들을 추가하면서 본격적인 운영체제 개발의 세계를 탐험해보세요! 🚀

# My OS Project Guide

## 소개 (Introduction)

"My OS"는 간단한 운영체제를 처음부터 만들어보는 학습 프로젝트입니다. 이 프로젝트는 부트로더부터 시작해 커널까지 기본적인 운영체제의 구성 요소를 직접 개발하는 과정을 담고 있습니다. 운영체제의 기본 원리를 이해하고, 시스템 프로그래밍에 대한 심층적인 지식을 얻을 수 있는 좋은 기회가 될 것입니다.

## 프로젝트 구조 (Project Structure)

```
my-os/
├── boot/                  # 부트로더 관련 파일
│   ├── bootloader.asm     # 메인 부트로더 코드
│   ├── disk.asm           # 디스크 읽기 루틴
│   └── gdt.asm            # 글로벌 디스크립터 테이블(GDT) 정의
├── build/                 # 빌드 결과물 디렉토리
│   ├── bootloader.bin     # 컴파일된 부트로더
│   ├── hard-disk.img      # 최종 OS 이미지
│   ├── kernel_elf.o       # 커널 ELF 파일
│   ├── kernel_entry.o     # 커널 진입점 오브젝트 파일
│   ├── kernel.bin         # 컴파일된 커널 바이너리
│   └── kernel.o           # 커널 C 코드 오브젝트 파일
├── docs/                  # 문서 디렉토리
├── kernel/                # 커널 관련 파일
│   ├── kernel.c           # 메인 커널 C 코드
│   ├── kernel_entry.asm   # 커널 진입점 어셈블리 코드
│   └── kernel.ld          # 커널 링커 스크립트
├── create_hdd.sh          # 하드 디스크 이미지 생성 스크립트
├── Makefile               # 빌드 시스템 정의
└── README.md              # 프로젝트 소개
```

## 구성 요소 설명 (Component Explanation)

### 1. 부트로더 (Bootloader)

부트로더는 컴퓨터가 켜질 때 가장 먼저 실행되는 코드입니다. BIOS는 하드 디스크의 첫 번째 섹터(512바이트)를 메모리에 로드하고 실행합니다.

#### bootloader.asm

- **기능**: 시스템 초기화, 실제 모드(16비트)에서 보호 모드(32비트)로 전환, 커널 로딩
- **주요 단계**:
  1. 세그먼트 레지스터 및 스택 초기화
  2. 디스크에서 커널 로드
  3. GDT(Global Descriptor Table) 설정
  4. 보호 모드(32비트)로 전환
  5. 커널 진입점으로 점프(0x1000)

#### gdt.asm

- **역할**: 메모리 세그먼트를 정의하는 GDT 설정
- **구성**:
  - 널(Null) 디스크립터
  - 코드 세그먼트 디스크립터
  - 데이터 세그먼트 디스크립터

#### disk.asm

- **기능**: BIOS 인터럽트를 사용한 디스크 읽기 루틴
- **작동 방식**: BIOS 인터럽트 0x13을 사용하여 디스크 섹터를 메모리로 로드

### 2. 커널 (Kernel)

커널은 운영체제의 핵심 부분으로, 하드웨어와 소프트웨어 간의 통신을 관리합니다.

#### kernel_entry.asm

- **역할**: C로 작성된 커널 메인 함수로 진입하기 위한 어셈블리 코드
- **주요 기능**:
  - 커널용 스택 설정
  - `kernel_main` C 함수 호출
  - 커널이 종료되면 무한 루프 실행

#### kernel.c

- **기능**: 실제 커널 코드 (C언어로 작성)
- **주요 기능**:
  - VGA 텍스트 모드 터미널 인터페이스 초기화
  - 화면에 메시지 출력
  - CPU 정보 및 메모리 크기 표시

#### kernel.ld

- **역할**: 커널 링커 스크립트
- **기능**: 커널 코드를 메모리 주소 0x1000에 로드하도록 지정하고, 여러 섹션(.text, .data, .bss 등)을 적절히 배치

### 3. 빌드 시스템 (Build System)

#### Makefile

- **역할**: 프로젝트 빌드 자동화
- **주요 타겟**:
  - `all`: 전체 OS 이미지 생성
  - `bootloader`: 부트로더만 빌드
  - `kernel`: 커널만 빌드
  - `clean`: 빌드 결과물 정리
  - `run`: QEMU에서 OS 실행
  - `debug`: QEMU에서 디버깅 모드로 실행

#### create_hdd.sh

- **역할**: 최종 하드 디스크 이미지 생성
- **작동 방식**:
  1. 10MB 크기의 빈 디스크 이미지 생성
  2. 부트로더를 첫 번째 섹터(MBR)에 복사
  3. 커널을 두 번째 섹터부터 복사

## 빌드 및 실행 방법 (Build and Run Instructions)

### 필수 도구 (Prerequisites)

- **크로스 컴파일러**: i686-elf-gcc, i686-elf-ld
- **어셈블러**: NASM
- **에뮬레이터**: QEMU

### 빌드 명령 (Build Commands)

1. **전체 OS 빌드**:
   ```bash
   make all
   ```

2. **부트로더만 빌드**:
   ```bash
   make bootloader
   ```

3. **커널만 빌드**:
   ```bash
   make kernel
   ```

4. **하드 디스크 이미지 생성**:
   ```bash
   ./create_hdd.sh
   ```

### 실행 명령 (Run Commands)

1. **QEMU에서 실행**:
   ```bash
   make run
   ```

2. **디버깅 모드로 실행**:
   ```bash
   make debug
   ```

## 학습 포인트 (Learning Points)

### 1. 실제 모드와 보호 모드 (Real Mode and Protected Mode)

- **실제 모드(16비트)**: 부팅 시 초기 모드, 메모리 접근 제한 없음, 세그먼트:오프셋 주소 지정 방식
- **보호 모드(32비트)**: 메모리 보호 기능, 가상 메모리 지원, 더 많은 메모리 접근 가능

### 2. 부트 프로세스 (Boot Process)

1. **BIOS 단계**: 전원 켜짐 → POST(Power-On Self Test) → 부트 장치 찾기
2. **부트로더 단계**: MBR 로드 → 실제 모드에서 초기화 → 보호 모드로 전환 → 커널 로드
3. **커널 초기화**: 커널 진입 → 하드웨어 초기화 → 기본 서비스 시작

### 3. 메모리 관리 (Memory Management)

- **세그먼테이션**: GDT를 통한 메모리 세그먼트 정의
- **메모리 매핑**: 커널을 특정 물리 메모리 주소(0x1000)에 로드

### 4. 하드웨어 상호작용 (Hardware Interaction)

- **VGA 텍스트 모드**: 화면에 텍스트 출력 (메모리 매핑된 I/O)
- **CPUID 명령**: CPU 정보 얻기
- **BIOS 인터럽트**: 디스크 읽기 등의 저수준 작업

## 확장 가능성 (Possible Extensions)

이 프로젝트를 더 발전시킬 수 있는 몇 가지 방향:

1. **인터럽트 처리**: IDT(Interrupt Descriptor Table) 설정 및 인터럽트 핸들러 구현
2. **메모리 관리 개선**: 페이징 도입, 힙 메모리 관리
3. **키보드 드라이버**: 사용자 입력 처리
4. **간단한 파일 시스템**: 기본적인 파일 읽기/쓰기 기능
5. **태스크 관리**: 멀티태스킹 기능 구현
6. **사용자 모드 지원**: 권한 레벨 설정 및 시스템 콜 구현

## 참고 자료 (References)

- [OSDev Wiki](https://wiki.osdev.org/): 운영체제 개발 관련 종합 자료
- "Operating Systems: Three Easy Pieces" - Remzi H. Arpaci-Dusseau and Andrea C. Arpaci-Dusseau
- "Operating System Concepts" - Silberschatz, Galvin, and Gagne
- Intel 아키텍처 소프트웨어 개발자 매뉴얼

---

이 문서는 My OS 프로젝트의 기본 구조와 작동 방식을 이해하는 데 도움을 주기 위해 작성되었습니다. 운영체제 개발은 복잡한 주제이므로, 각 부분에 대해 더 깊이 이해하고 싶다면 참고 자료를 확인하세요.

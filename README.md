# CHANUX

![Version](https://img.shields.io/badge/version-0.2.0-blue.svg)
![Architecture](https://img.shields.io/badge/arch-i686-green.svg)
![Language](https://img.shields.io/badge/language-C%2FASM-orange.svg)
![License](https://img.shields.io/badge/license-MIT-red.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![CI/CD](https://img.shields.io/badge/CI%2FCD-GitHub%20Actions-blue.svg)

**CHANUX**는 32비트 x86 아키텍처를 위한 교육용 운영체제입니다. 부트로더부터 커널까지 완전히 처음부터 개발된 단순하지만 완전한 운영체제로, 현대적인 운영체제의 핵심 기능들을 학습할 수 있도록 설계되었습니다. GitHub Actions를 통한 자동화된 CI/CD 파이프라인으로 지속적인 통합과 배포를 지원합니다.

## 🚀 주요 특징

### 🔧 시스템 아키텍처
- **32비트 x86 (i686) 아키텍처 지원**
- **보호 모드 (Protected Mode) 동작**
- **2단계 부트 시스템** - 대용량 커널 로딩 지원 (최대 80KB+)
  - Stage 1: BIOS 호환 512바이트 부트로더 (MBR)
  - Stage 2: 확장 부트로더 (GDT 설정, A20 활성화, 보호모드 전환, 커널 로딩)
- **GDT (Global Descriptor Table)** 구성
- **IDT (Interrupt Descriptor Table)** 및 인터럽트 처리
- **PIC (Programmable Interrupt Controller)** 지원

### 💻 하드웨어 지원
- **VGA 텍스트 모드** (80x25 컬러 텍스트, 하드웨어 커서)
- **키보드 드라이버** - PS/2 키보드 지원 (스캔코드 → ASCII 변환)
- **CPU 정보 검출** - CPUID를 통한 프로세서 특성, 캐시, 코어 정보 감지
- **FPU (Floating Point Unit)** 완전 지원 및 초기화
- **HDD (하드 디스크)** - ATA/IDE 인터페이스 (LBA28 모드)
- **타이머 인터럽트** - 시스템 틱 및 스케줄링

### 🧠 커널 기능
- **메모리 관리** - CMOS 및 프로브 기반 메모리 감지
- **프로세스 관리** - 멀티태스킹 지원 (기본 스케줄러, 컨텍스트 스위칭)
- **시스템 콜** - fork, exit, getpid, sleep, yield, kill, waitpid, exec 지원
- **터미널 인터페이스** - 컬러 텍스트 출력, 스크롤링, 커서 제어
- **파일 시스템** - FAT32 완전 지원 (포맷, 읽기/쓰기, 디렉터리 관리)
- **컨텍스트 스위칭** - 어셈블리 최적화된 프로세스 간 전환
- **동적 메모리 관리** - 기본적인 힙 관리자 구현
- **타이머 콜백** - 시스템 하트비트 및 주기적 작업 지원

## 🏗️ 프로젝트 구조

```
chanux/
├── boot/                     # 부트로더 관련 파일
│   ├── stage1.asm           # 1단계 부트로더 (512바이트, MBR)
│   └── stage2.asm           # 2단계 부트로더 (확장 기능)
├── kernel/                   # 커널 소스 코드
│   ├── kernel_entry.asm     # 커널 진입점 (어셈블리)
│   ├── kernel_main.c        # 커널 메인 함수
│   ├── kernel.ld            # 링커 스크립트
│   ├── include/             # 헤더 파일
│   │   ├── kernel.h         # 메인 커널 헤더
│   │   ├── common/          # 공통 유틸리티
│   │   │   ├── types.h      # 기본 타입 정의
│   │   │   └── utils.h      # 유틸리티 함수
│   │   ├── cpu/             # CPU 관련
│   │   │   ├── cpu.h        # CPU 정보 및 기능
│   │   │   └── fpu.h        # 부동소수점 유닛
│   │   ├── interrupts/      # 인터럽트 처리
│   │   │   └── interrupts.h # IDT 및 인터럽트 핸들러
│   │   ├── keyboard/        # 키보드 드라이버
│   │   │   └── keyboard.h   # 키보드 입력 처리
│   │   ├── memory/          # 메모리 관리
│   │   │   └── memory.h     # 메모리 감지 및 관리
│   │   ├── process/         # 프로세스 관리
│   │   │   └── process.h    # 프로세스 및 스케줄링
│   │   ├── storage/         # 저장장치 드라이버
│   │   │   ├── hdd.h        # ATA/IDE 하드디스크
│   │   │   └── fat32.h      # FAT32 파일시스템
│   │   ├── syscalls/        # 시스템 콜
│   │   │   └── syscalls.h   # 시스템 콜 인터페이스
│   │   ├── terminal/        # 터미널 인터페이스
│   │   │   └── terminal.h   # 텍스트 출력 관리
│   │   ├── timer/           # 타이머 시스템
│   │   │   └── pit.h        # PIT 타이머 관리
│   │   └── vga/             # VGA 그래픽
│   │       └── vga.h        # VGA 텍스트 모드
│   └── src/                 # 구현 파일
│       ├── common/          # 공통 함수 구현
│       │   └── utils.c      # 문자열, 메모리 함수
│       ├── cpu/             # CPU/FPU 드라이버
│       │   ├── cpu.c        # CPU 정보 감지
│       │   └── fpu.c        # FPU 초기화 및 제어
│       ├── interrupts/      # IDT 및 인터럽트 핸들러
│       │   ├── idt.c        # IDT 설정
│       │   ├── interrupt_handlers.asm  # 어셈블리 핸들러
│       │   └── interrupt_handlers.c    # C 인터럽트 핸들러
│       ├── keyboard/        # 키보드 입력 처리
│       │   └── keyboard.c   # 스캔코드 → ASCII 변환
│       ├── memory/          # 메모리 할당/관리
│       │   └── memory.c     # 메모리 감지 및 할당
│       ├── process/         # 프로세스 생성/스케줄링
│       │   ├── process.c    # 프로세스 관리
│       │   └── context_switch.asm      # 컨텍스트 스위칭
│       ├── storage/         # 저장장치 드라이버
│       │   ├── hdd.c        # ATA/IDE 드라이버
│       │   └── fat32.c      # FAT32 파일시스템
│       ├── syscalls/        # 시스템 콜 구현
│       │   ├── syscalls.c   # 시스템 콜 핸들러
│       │   └── syscall_interrupt.asm   # 시스템 콜 인터럽트
│       ├── terminal/        # 터미널 출력
│       │   └── terminal.c   # VGA 텍스트 출력
│       ├── timer/           # 타이머 시스템
│       │   └── pit.c        # PIT 타이머 구현
│       └── vga/             # VGA 텍스트 모드
│           └── vga.c        # VGA 하드웨어 제어
├── build/                   # 빌드 출력 파일
│   ├── *.o                  # 오브젝트 파일들
│   ├── *.bin                # 바이너리 파일들
│   └── hard-disk.img        # 부팅 가능한 디스크 이미지
├── docs/                    # 프로젝트 문서
│   ├── boot_system.md       # 부트 시스템 문서
│   ├── fat32_system.md      # FAT32 파일시스템 문서
│   ├── interrupt_system.md  # 인터럽트 시스템 문서
│   ├── kernel_system.md     # 커널 시스템 문서
│   └── process_system.md    # 프로세스 관리 문서
├── .github/                 # GitHub Actions 워크플로
│   └── workflows/           # CI/CD 파이프라인
│       ├── build.yml        # 빌드 워크플로
│       ├── c-cpp.yml        # C/C++ 빌드 체크
│       ├── codeql.yml       # 코드 품질 분석
│       └── label.yml        # 자동 라벨링
├── create_hdd.sh            # 하드디스크 이미지 생성 스크립트
├── move_file.sh             # 파일 이동 유틸리티 스크립트
└── Makefile                 # 빌드 시스템
```

## 🛠️ 개발 환경 설정

### 필수 도구

**macOS (Homebrew 사용):**
```bash
# Cross-compiler 설치
brew install i686-elf-gcc
brew install i686-elf-binutils

# 어셈블러 설치
brew install nasm

# 에뮬레이터 설치 (테스트용)
brew install qemu
```

**Linux (Ubuntu/Debian):**
```bash
# Cross-compiler 도구 설치
sudo apt-get update
sudo apt-get install gcc-multilib nasm qemu-system-x86
```

### 빌드 및 실행

1. **프로젝트 클론:**
```bash
git clone https://github.com/h9661/chanux.git
cd chanux
```

2. **전체 빌드:**
```bash
make all
```

3. **개별 컴포넌트 빌드:**
```bash
make stage1      # 1단계 부트로더만 빌드
make stage2      # 2단계 부트로더만 빌드  
make bootloader  # 두 부트로더 모두 빌드
make kernel      # 커널만 빌드
make hdd         # 하드디스크 이미지 생성
make structure   # 프로젝트 구조 표시
make help        # 사용 가능한 빌드 타겟 목록 표시
```

4. **QEMU에서 실행:**
```bash
# 하드디스크 이미지로 부팅
make run

# 또는 직접 실행
qemu-system-i386 -drive format=raw,file=build/hard-disk.img -m 64

# 디버그 모드로 실행
make debug
```

5. **정리:**
```bash
make clean        # 빌드 파일 삭제
```

## 🔍 시스템 사양

### 메모리 레이아웃
- **1단계 부트로더:** `0x7C00` (BIOS 표준, 512바이트)
- **2단계 부트로더:** `0x7E00` (1단계 직후, 다중 섹터)
- **커널 로드 주소:** `0x10000` (64KB, 안전한 위치)
- **스택:** `0x90000` (576KB, 보호모드용)
- **VGA 버퍼:** `0xB8000` (VGA 텍스트 모드)

### 디스크 레이아웃
- **섹터 0:** 1단계 부트로더 (512바이트)
- **섹터 1:** 예약됨
- **섹터 2-9:** 2단계 부트로더 (최대 4KB)
- **섹터 10+:** 커널 바이너리 (최대 80KB+)

### 기술적 특징
- **크로스 컴파일:** i686-elf-gcc 툴체인 사용
- **링커 스크립트:** 정확한 메모리 레이아웃 제어
- **모듈식 설계:** 각 서브시스템이 독립적으로 개발/테스트 가능
- **하드웨어 추상화:** 포트 I/O 래퍼 함수로 하드웨어 접근
- **인터럽트 기반:** 비동기 이벤트 처리 (키보드, 타이머)
- **메모리 안전성:** 스택 오버플로우 방지를 위한 충분한 여유 공간
- **어셈블리 최적화:** 핵심 부분은 어셈블리로 최적화
- **ELF 바이너리 생성:** 표준 ELF32 형식으로 컴파일
- **CI/CD 자동화:** GitHub Actions를 통한 자동 빌드 및 테스트
- **선점형 스케줄링:** 10ms 타임 퀀텀 기반 라운드 로빈 스케줄러
- **프로세스 생명주기 관리:** READY, RUNNING, BLOCKED, SLEEPING, TERMINATED 상태
- **스퓨리어스 인터럽트 감지:** PIC 안정성 향상
- **성능 모니터링:** 인터럽트 통계 및 프로세스 CPU 시간 추적

### 지원하는 시스템 콜
- `SYS_EXIT` (1) - 프로세스 종료
- `SYS_FORK` (2) - 새 프로세스 생성 (자식 프로세스 생성)
- `SYS_GETPID` (3) - 프로세스 ID 조회
- `SYS_SLEEP` (4) - 프로세스 일시정지 (밀리초 단위)
- `SYS_YIELD` (5) - CPU 양보 (스케줄러에게 제어권 전달)
- `SYS_KILL` (6) - 프로세스 종료 시그널 전송
- `SYS_WAITPID` (7) - 자식 프로세스 대기 (동기화)
- `SYS_EXEC` (8) - 프로그램 실행 (현재 프로세스 교체)

### 파일 시스템 기능
- **FAT32 완전 지원** - 포맷, 마운트, 파일 I/O
- **파일 작업** - 생성, 읽기, 쓰기, 삭제, 크기 조회, 복사, 이동
- **디렉터리 작업** - 생성, 삭제, 탐색, 현재 디렉터리 변경
- **롱 파일네임** (LFN) 지원
- **파일 속성** 관리 (읽기전용, 숨김, 시스템 등)
- **타임스탬프** 지원 (생성, 수정, 접근 시간)
- **공간 관리** - 여유 공간 조회, 클러스터 정보, 통계

### 키보드 지원
- **표준 QWERTY 레이아웃**
- **수식어 키** - Shift, Ctrl, Alt 키 지원
- **Caps Lock** 토글 기능
- **특수 키** - Enter, Backspace, Tab, ESC, Space 처리
- **기능 키** - F1, F2, F3 지원
- **스캔코드** → ASCII 변환 테이블
- **입력 버퍼링** - 키보드 입력 큐 관리

## 🚧 개발 진행 상황

### ✅ 완료된 기능
- [x] **2단계 부트로더** (16비트 → 32비트 모드 전환)
- [x] **GDT 및 보호모드** 설정 (완전한 메모리 보호)
- [x] **IDT 및 인터럽트 처리** (타이머, 키보드, 예외 처리)
- [x] **PIC 초기화** 및 IRQ 관리 (스퓨리어스 인터럽트 감지 포함)
- [x] **VGA 텍스트 모드** 드라이버 (하드웨어 커서, 색상 지원)
- [x] **키보드 입력 처리** (완전한 스캔코드 → ASCII 변환, 수식어 키 지원)
- [x] **메모리 관리** (CMOS/프로브 방식 메모리 감지, 동적 할당 준비)
- [x] **CPU 정보 검출** (CPUID, 캐시, 코어/스레드 정보, 명령어 집합)
- [x] **FPU 완전 초기화** 및 제어 (부동소수점 연산 지원)
- [x] **프로세스 관리 시스템** (PCB, Round-Robin 스케줄러, 상태 관리)
- [x] **컨텍스트 스위칭** (어셈블리 최적화, 선점형 스케줄링)
- [x] **시스템 콜 인터페이스** (8개 시스템 콜: fork, exit, getpid, sleep, yield, kill, waitpid, exec)
- [x] **ATA/IDE HDD 드라이버** (LBA28 모드, 드라이브 감지)
- [x] **FAT32 파일시스템** 완전 구현 (포맷, 읽기/쓰기, 디렉터리 관리, LFN 지원)
- [x] **터미널 인터페이스** (스크롤링, 색상, 커서, 스크롤 버퍼)
- [x] **유틸리티 함수들** (문자열, 메모리, 변환 함수 완전 구현)
- [x] **PIT 타이머 시스템** (100Hz 주파수, 콜백 지원, 정밀한 시간 측정)
- [x] **시스템 하트비트** (5초마다 시스템 상태 표시)
- [x] **프로세스 컨텍스트 스위칭 테스트** 시스템 (멀티프로세싱 데모)
- [x] **고급 인터럽트 관리** (통계 추적, 성능 측정, 오류 복구)
- [x] **프로세스 생명주기 관리** (생성, 종료, 슬립, 블로킹, 대기)
- [x] **GitHub Actions CI/CD** 파이프라인 (자동 빌드 및 테스트)
- [x] **종합적인 문서화** (시스템별 상세 문서, 기술 사양서)

### 🔄 진행 중인 기능
- [ ] **완전한 멀티프로세싱** (다중 프로세스 동시 실행)
- [ ] **고급 스케줄링 알고리즘** (우선순위 기반 스케줄링)
- [ ] **메모리 보호** 및 가상 메모리 시스템
- [ ] **사용자 모드** 프로세스 지원
- [ ] **ELF 실행 파일** 로더 구현
- [ ] **쓰레드 관리 시스템** (동시성 및 동기화)

### 📋 향후 계획
- [ ] **페이징** 및 가상 메모리 관리
- [ ] **동적 메모리 할당** (malloc/free)
- [ ] **고급 프로세스 간 통신** (파이프, 공유 메모리)
- [ ] **네트워킹 스택** (기본 TCP/IP)
- [ ] **그래픽 모드** 지원 (VESA)
- [ ] **USB 드라이버** 및 장치 지원
- [ ] **멀티코어** 지원 (SMP)
- [ ] **오디오 드라이버** 기초
- [ ] **쉘 및 명령어** 인터프리터
- [ ] **APIC 인터럽트 관리** (고급 PIC)
- [ ] **ACPI 통합** 및 전원 관리

## 🔧 개발 가이드

### 새로운 드라이버 추가
1. `kernel/include/` 에 헤더 파일 생성
2. `kernel/src/` 에 구현 파일 작성
3. `Makefile` 에 소스 파일 추가 (KERNEL_C_SOURCES 및 KERNEL_C_OBJS)
4. `kernel_main.c` 에서 초기화 함수 호출

### 시스템 콜 추가
1. `kernel/include/syscalls/syscalls.h` 에 새 시스템 콜 번호 정의
2. `kernel/src/syscalls/syscalls.c` 에 핸들러 구현
3. `syscall_handler` 함수에 새 케이스 추가

### 인터럽트 핸들러 추가
1. `kernel/src/interrupts/interrupt_handlers.asm` 에 어셈블리 래퍼 추가
2. `kernel/src/interrupts/interrupt_handlers.c` 에 C 핸들러 구현
3. `kernel/src/interrupts/idt.c` 에서 IDT 엔트리 설정

### 디버깅
- **QEMU 모니터 모드:** `Ctrl+Alt+2`
- **GDB 디버깅:** `make debug` 후 GDB 연결
- **시리얼 출력:** `/dev/ttyS0` (QEMU 시리얼 포트)
- **메모리 덤프:** QEMU 모니터에서 `info mem`, `x` 명령어 사용

## 📚 학습 자료

### 운영체제 개발 참고서
- "Operating Systems: Design and Implementation" - Tanenbaum
- "Understanding the Linux Kernel" - Bovet & Cesati  
- "Intel 64 and IA-32 Architectures Software Developer's Manual"
- **OSDev Wiki:** https://wiki.osdev.org/
- **Bran's Kernel Development Tutorial**

### 어셈블리 프로그래밍
- **NASM 문서:** https://nasm.us/docs.php
- **x86 Assembly Guide:** https://cs.virginia.edu/~evans/cs216/guides/x86.html
- **Intel x86 Instruction Set Reference**

### 파일 시스템 및 하드웨어
- **Microsoft FAT32 File System Specification**
- **ATA/ATAPI Command Set Standards**
- **VGA Programming Documentation**

## 🤝 기여하기

1. 이 저장소를 포크합니다
2. 새로운 기능 브랜치를 만듭니다 (`git checkout -b feature/새기능`)
3. 변경사항을 커밋합니다 (`git commit -am '새기능 추가'`)
4. 브랜치에 푸시합니다 (`git push origin feature/새기능`)
5. Pull Request를 생성합니다

## 📄 라이선스

이 프로젝트는 MIT 라이선스 하에 배포됩니다. 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.

## 👨‍💻 개발자

- **개발자:** [개발자 이름]
- **이메일:** [이메일 주소]
- **GitHub:** [GitHub 프로필]

## 🙏 감사의 말

이 프로젝트는 운영체제 개발 학습을 위한 교육용 목적으로 개발되었습니다. OSDev 커뮤니티와 다양한 오픈소스 운영체제 프로젝트들로부터 영감을 받았습니다.

---

**참고:** 이 운영체제는 교육 및 학습 목적으로 개발되었으며, 실제 프로덕션 환경에서 사용하기에는 적합하지 않습니다.
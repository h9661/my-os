# Simple OS (SimpleOS)

![Version](https://img.shields.io/badge/version-0.1.0-blue.svg)
![Architecture](https://img.shields.io/badge/arch-i686-green.svg)
![Language](https://img.shields.io/badge/language-C%2FASM-orange.svg)
![License](https://img.shields.io/badge/license-MIT-red.svg)

**Simple OS**는 32비트 x86 아키텍처를 위한 교육용 운영체제입니다. 부트로더부터 커널까지 완전히 처음부터 개발된 단순하지만 완전한 운영체제입니다.

## 🚀 주요 특징

### 🔧 시스템 아키텍처
- **32비트 x86 (i686) 아키텍처 지원**
- **보호 모드 (Protected Mode) 동작**
- **커스텀 부트로더** - 16비트 실모드에서 32비트 보호모드로 전환
- **GDT (Global Descriptor Table)** 구성
- **IDT (Interrupt Descriptor Table)** 및 인터럽트 처리

### 💻 하드웨어 지원
- **VGA 텍스트 모드** (80x25 컬러 텍스트)
- **키보드 드라이버** - PS/2 키보드 지원
- **CPU 정보 검출** - CPUID를 통한 프로세서 특성 감지
- **FPU (Floating Point Unit)** 지원
- **HDD (하드 디스크)** - ATA/IDE 인터페이스 기본 지원

### 🧠 커널 기능
- **메모리 관리** - 기본적인 메모리 할당 및 관리
- **프로세스 관리** - 멀티태스킹 지원 (기본 스케줄러)
- **시스템 콜** - fork, exit, getpid, sleep, yield, kill, waitpid 지원
- **터미널 인터페이스** - 컬러 텍스트 출력 및 기본 명령어 처리
- **컨텍스트 스위칭** - 프로세스 간 전환

## 🏗️ 프로젝트 구조

```
my-os/
├── boot/                     # 부트로더 관련 파일
│   ├── bootloader.asm       # 메인 부트로더
│   ├── disk.asm            # 디스크 I/O 함수
│   └── gdt.asm             # Global Descriptor Table
├── kernel/                   # 커널 소스 코드
│   ├── kernel_entry.asm    # 커널 진입점 (어셈블리)
│   ├── kernel_main.c       # 커널 메인 함수
│   ├── kernel.ld           # 링커 스크립트
│   ├── include/            # 헤더 파일
│   │   ├── kernel.h        # 메인 커널 헤더
│   │   ├── common/         # 공통 유틸리티
│   │   ├── cpu/            # CPU 관련
│   │   ├── interrupts/     # 인터럽트 처리
│   │   ├── keyboard/       # 키보드 드라이버
│   │   ├── memory/         # 메모리 관리
│   │   ├── process/        # 프로세스 관리
│   │   ├── storage/        # 저장장치 드라이버
│   │   ├── syscalls/       # 시스템 콜
│   │   ├── terminal/       # 터미널 인터페이스
│   │   └── vga/            # VGA 그래픽
│   └── src/                # 구현 파일
│       ├── common/         # 공통 함수 구현
│       ├── cpu/            # CPU/FPU 드라이버
│       ├── interrupts/     # IDT 및 인터럽트 핸들러
│       ├── keyboard/       # 키보드 입력 처리
│       ├── memory/         # 메모리 할당/관리
│       ├── process/        # 프로세스 생성/스케줄링
│       ├── storage/        # HDD 드라이버
│       ├── syscalls/       # 시스템 콜 구현
│       ├── terminal/       # 터미널 출력
│       └── vga/            # VGA 텍스트 모드
├── build/                   # 빌드 출력 파일
└── Makefile                # 빌드 시스템
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
git clone <repository-url>
cd my-os
```

2. **전체 빌드:**
```bash
make all
```

3. **개별 컴포넌트 빌드:**
```bash
make bootloader    # 부트로더만 빌드
make kernel        # 커널만 빌드
make hdd          # 하드디스크 이미지 생성
```

4. **QEMU에서 실행:**
```bash
# 하드디스크 이미지로 부팅
qemu-system-i386 -drive format=raw,file=build/hard-disk.img

# 디버그 모드로 실행
make debug
```

5. **정리:**
```bash
make clean        # 빌드 파일 삭제
```

## 🔍 시스템 사양

### 메모리 레이아웃
- **부트로더:** `0x7C00` (BIOS 표준)
- **커널 로드 주소:** `0x1000` (4KB)
- **스택:** `0x9000` (36KB)
- **VGA 버퍼:** `0xB8000` (VGA 텍스트 모드)

### 지원하는 시스템 콜
- `SYS_EXIT` - 프로세스 종료
- `SYS_FORK` - 새 프로세스 생성
- `SYS_GETPID` - 프로세스 ID 조회
- `SYS_SLEEP` - 프로세스 일시정지
- `SYS_YIELD` - CPU 양보
- `SYS_KILL` - 프로세스 종료 시그널
- `SYS_WAITPID` - 자식 프로세스 대기

### 키보드 지원
- **표준 QWERTY 레이아웃**
- **Shift, Ctrl, Alt 키 지원**
- **Caps Lock 지원**
- **특수 키 (Enter, Backspace, Tab, ESC) 처리**

## 🚧 개발 진행 상황

### ✅ 완료된 기능
- [x] 부트로더 (16비트 → 32비트 모드 전환)
- [x] GDT 및 보호모드 설정
- [x] IDT 및 인터럽트 처리
- [x] VGA 텍스트 모드 드라이버
- [x] 키보드 입력 처리
- [x] 기본 메모리 관리
- [x] CPU 정보 검출 (CPUID)
- [x] FPU 초기화
- [x] 프로세스 관리 기초
- [x] 시스템 콜 인터페이스
- [x] HDD 인터페이스 기초
- [x] 터미널 인터페이스

### 🔄 진행 중인 기능
- [ ] 완전한 프로세스 스케줄러
- [ ] 파일 시스템
- [ ] 네트워크 스택
- [ ] 그래픽 모드 지원

### 📋 향후 계획
- [ ] 동적 메모리 할당 개선
- [ ] 사용자 모드 지원
- [ ] 멀티코어 지원
- [ ] USB 드라이버
- [ ] 오디오 지원

## 🔧 개발 가이드

### 새로운 드라이버 추가
1. `kernel/include/` 에 헤더 파일 생성
2. `kernel/src/` 에 구현 파일 작성
3. `Makefile` 에 소스 파일 추가
4. `kernel_main.c` 에서 초기화 함수 호출

### 시스템 콜 추가
1. `kernel/include/syscalls/syscalls.h` 에 새 시스템 콜 번호 정의
2. `kernel/src/syscalls/syscalls.c` 에 핸들러 구현
3. `syscall_handler` 함수에 새 케이스 추가

### 디버깅
- QEMU 모니터 모드: `Ctrl+Alt+2`
- GDB 디버깅: `make debug` 후 GDB 연결
- 시리얼 출력: `/dev/ttyS0` (QEMU 시리얼 포트)

## 📚 학습 자료

### 운영체제 개발 참고서
- "Operating Systems: Design and Implementation" - Tanenbaum
- "Understanding the Linux Kernel" - Bovet & Cesati
- OSDev Wiki: https://wiki.osdev.org/
- Intel 80386 Programmer's Reference Manual

### 어셈블리 프로그래밍
- NASM 문서: https://nasm.us/docs.php
- x86 Assembly Guide: https://cs.virginia.edu/~evans/cs216/guides/x86.html

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
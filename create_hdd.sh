#!/bin/bash
# ============================================================================
# 하드 디스크 이미지 생성 스크립트 (Hard Disk Image Creation Script)
# ============================================================================
# 역할: 1) OS 컴포넌트 빌드 2) 바이너리 검증 3) 디스크 이미지 조립 4) 부팅 가능한 이미지 생성
# 이 스크립트는 부트로더와 커널을 하나의 부팅 가능한 디스크 이미지로 조합함

# === 1단계: 빌드 환경 준비 ===
# 빌드 디렉토리가 존재하지 않으면 생성 (첫 실행 시 필요)
mkdir -p build
echo "📁 Build directory prepared"

# === 2단계: 이전 빌드 정리 ===
# 이전 빌드의 잔재물을 제거하여 깨끗한 빌드 환경 확보
# 오래된 오브젝트 파일이나 바이너리가 새 빌드에 영향을 주는 것을 방지
make clean
echo "🧹 Previous build artifacts cleaned"

# === 3단계: 부트로더 빌드 ===
# NASM 어셈블러를 사용하여 16/32비트 부트로더 어셈블리 코드를 바이너리로 변환
# bootloader.asm → bootloader.bin (정확히 512바이트, 부트 시그니처 포함)
make bootloader
if [ $? -ne 0 ]; then
    echo "❌ Bootloader build failed!"
    exit 1
fi
echo "✅ Bootloader built successfully"

# === 4단계: 커널 빌드 ===  
# GCC 크로스 컴파일러를 사용하여 C 커널과 어셈블리 진입점을 빌드
# kernel.c + kernel_entry.asm → kernel.bin (ELF에서 raw binary로 변환)
make kernel
if [ $? -ne 0 ]; then
    echo "❌ Kernel build failed!"
    exit 1
fi
echo "✅ Kernel built successfully"

# === 5단계: 커널 바이너리 형식 검증 및 변환 ===
# 크로스 컴파일러가 생성한 파일이 ELF 형식인지 확인
# ELF 형식이면 objcopy를 사용하여 raw binary로 변환 (부팅 가능한 형태)
file_type=$(file -b build/kernel.bin)
if [[ $file_type == *"ELF"* ]]; then
    echo "⚠️  WARNING: kernel.bin is still an ELF file. Converting to raw binary..."
    # i686-elf-objcopy: ELF 바이너리를 raw binary로 변환하는 도구
    # -O binary: 출력 형식을 raw binary로 지정
    /opt/homebrew/bin/i686-elf-objcopy -O binary build/kernel.bin build/kernel_raw.bin
    mv build/kernel_raw.bin build/kernel.bin
    echo "🔄 Converted ELF to raw binary format"
else
    echo "✅ Kernel is already in raw binary format"
fi

# === 6단계: 커널 크기 계산 및 분석 ===
# 커널 바이너리의 크기를 바이트 단위로 측정
# 디스크 이미지에서 커널이 차지할 섹터 수를 계산하기 위함
KERNEL_SIZE=$(wc -c < build/kernel.bin)
echo "📏 Kernel size: $KERNEL_SIZE bytes"

# 섹터 수 계산: 1 섹터 = 512바이트, 올림 계산으로 필요한 섹터 수 산출
# 예: 1000바이트 커널 = ceil(1000/512) = 2 섹터 필요
KERNEL_SECTORS=$(( ($KERNEL_SIZE + 511) / 512 ))
echo "💾 Kernel sectors needed: $KERNEL_SECTORS"

# === 7단계: 빈 디스크 이미지 생성 ===
# 10MB 크기의 빈 하드 디스크 이미지 파일 생성
# /dev/zero: 모든 비트가 0인 데이터를 무한히 제공하는 특수 파일
# bs=1M: 블록 크기를 1MB로 설정 (효율적인 I/O를 위함)
# count=10: 10개의 1MB 블록 = 10MB 총 크기
dd if=/dev/zero of=build/hard-disk.img bs=1M count=10 2>/dev/null
echo "💿 Created 10MB blank disk image"

# === 8단계: 부트로더를 첫 번째 섹터(MBR)에 복사 ===
# 하드 디스크의 첫 번째 섹터(LBA 0, 실린더 0, 헤드 0, 섹터 1)는 마스터 부트 레코드(MBR)
# BIOS는 시스템 부팅 시 이 섹터를 RAM의 0x7C00에 로드하고 실행함
# dd 명령어 파라미터 설명:
# - if: 입력 파일 (bootloader.bin, 정확히 512바이트)
# - of: 출력 파일 (hard-disk.img)  
# - conv=notrunc: 출력 파일을 자르지 않음 (기존 크기 유지)
# 이 명령은 디스크 이미지의 첫 512바이트에 부트로더를 기록함
dd if=build/bootloader.bin of=build/hard-disk.img conv=notrunc 2>/dev/null
echo "🚀 Bootloader copied to MBR (sector 0)"

# === 9단계: 커널을 두 번째 섹터부터 복사 ===
# 부트로더 다음 위치(LBA 1)부터 커널 바이너리를 배치
# 부트로더는 INT 0x13을 사용하여 이 위치에서 커널을 로드함
# dd 명령어 파라미터 설명:
# - if: 입력 파일 (kernel.bin)
# - of: 출력 파일 (hard-disk.img)
# - bs=512: 블록 크기를 512바이트(1섹터)로 설정
# - seek=1: 출력 파일에서 1번째 섹터부터 쓰기 시작 (0번째는 부트로더)
# - conv=notrunc: 파일 크기를 자르지 않고 기존 내용 위에 덮어쓰기
dd if=build/kernel.bin of=build/hard-disk.img bs=512 seek=1 conv=notrunc 2>/dev/null
echo "🔧 Kernel copied starting from sector 1"

# === 10단계: 디스크 레이아웃 정보 출력 ===
# 생성된 디스크 이미지의 구조를 사용자에게 명확히 표시
# 이 정보는 디버깅과 시스템 이해에 중요함
echo ""
echo "📋 DISK LAYOUT SUMMARY:"
echo "┌─────────────────────────────────────────────────────────┐"
echo "│ Sector 0 (LBA 0): MBR/Bootloader (512 bytes)           │"
echo "│ - Contains bootloader code + boot signature (0xAA55)   │"
echo "│ - BIOS loads this sector to 0x7C00 and executes it     │"
echo "├─────────────────────────────────────────────────────────┤"
echo "│ Sector 1+ (LBA 1-N): Kernel binary                     │"
echo "│ - Bootloader loads kernel from here to 0x1000          │"
echo "│ - Kernel size: $KERNEL_SECTORS sectors ($KERNEL_SIZE bytes)              │"
echo "└─────────────────────────────────────────────────────────┘"
echo ""
echo "🎯 Memory Layout After Boot:"
echo "  • 0x0000-0x7BFF: Available for kernel use"
echo "  • 0x7C00-0x7DFF: Bootloader code (loaded by BIOS)"
echo "  • 0x1000+: Kernel code (loaded by bootloader)"
echo "  • 0x90000: Stack for kernel functions"

# === 11단계: 바이너리 검증 및 디버깅 정보 ===
# 생성된 디스크 이미지의 첫 부분을 16진수로 덤프하여 내용 확인
# 이는 부트로더가 올바르게 복사되었는지, 부트 시그니처가 있는지 확인하는 데 사용
echo ""
echo "🔍 DISK IMAGE VERIFICATION:"
echo "First 64 bytes of the disk image (should show bootloader code):"
hexdump -C -n 64 build/hard-disk.img

# 부트 시그니처 확인 (섹터의 마지막 2바이트가 0x55AA여야 함)
echo ""
echo "Boot signature verification (bytes 510-511 should be 55 AA):"
hexdump -C -s 510 -n 2 build/hard-disk.img

# 커널 시작 부분 확인 (두 번째 섹터의 시작)
echo ""
echo "Kernel start verification (sector 1, first 32 bytes):"
hexdump -C -s 512 -n 32 build/hard-disk.img

# === 12단계: 빌드 완료 및 사용법 안내 ===
echo ""
echo "✨ BUILD COMPLETED SUCCESSFULLY!"
echo "📁 Output file: build/hard-disk.img"
echo ""
echo "🖥️  How to test the OS:"
echo "1. Using QEMU:"
echo "   qemu-system-i386 -drive format=raw,file=build/hard-disk.img"
echo ""
echo "2. Using VirtualBox:"
echo "   - Create new VM with 'Other' OS type"
echo "   - Convert img to VDI: VBoxManage convertfromraw build/hard-disk.img hard-disk.vdi --format VDI"
echo "   - Attach hard-disk.vdi as primary hard disk"
echo ""
echo "3. Using real hardware (DANGEROUS!):"
echo "   - Write image to USB: sudo dd if=build/hard-disk.img of=/dev/sdX"
echo "   - Boot from USB (WILL OVERWRITE THE USB DEVICE!)"
echo ""
echo "🎉 Your custom OS is ready to boot!"

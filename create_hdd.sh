#!/bin/bash

# Hard Disk Image Creation Script
# Creates bootable hard disk image with bootloader and kernel

set -e  # Exit on error

# Constants
HDD_IMAGE="build/hard-disk.img"
STAGE1_BIN="build/stage1.bin"
STAGE2_BIN="build/stage2.bin"
KERNEL_BIN="build/kernel.bin"
HDD_SIZE_MB=10  # Hard disk image size (10MB)

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Log functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Main function
main() {
    log_info "Starting hard disk image creation..."
    
    # Stage 1: Check required files exist
    check_required_files
    
    # Stage 2: Create blank hard disk image
    create_blank_image
    
    # Stage 3: Install Stage 1 bootloader (MBR - sector 0)
    install_stage1
    
    # Stage 4: Install Stage 2 bootloader (sectors 2-9)
    install_stage2
    
    # Stage 5: Install kernel (starting from sector 10)
    install_kernel
    
    # Stage 6: Show image information
    show_image_info
    
    log_success "Hard disk image created successfully: $HDD_IMAGE"
    log_info "You can now run: make run"
}

# Check required files exist
check_required_files() {
    log_info "Checking required files..."
    
    if [ ! -f "$STAGE1_BIN" ]; then
        log_error "Stage 1 bootloader binary not found: $STAGE1_BIN"
        log_info "Please run: make stage1"
        exit 1
    fi
    
    if [ ! -f "$STAGE2_BIN" ]; then
        log_error "Stage 2 bootloader binary not found: $STAGE2_BIN"
        log_info "Please run: make stage2"
        exit 1
    fi
    
    if [ ! -f "$KERNEL_BIN" ]; then
        log_error "Kernel binary not found: $KERNEL_BIN"
        log_info "Please run: make kernel"
        exit 1
    fi
    
    # Check Stage 1 size (must be exactly 512 bytes)
    STAGE1_SIZE=$(stat -f%z "$STAGE1_BIN" 2>/dev/null || stat -c%s "$STAGE1_BIN" 2>/dev/null)
    if [ "$STAGE1_SIZE" -ne 512 ]; then
        log_error "Stage 1 bootloader must be exactly 512 bytes, but got $STAGE1_SIZE bytes"
        exit 1
    fi
    
    # Check Stage 2 size (should be reasonable, up to 4KB = 8 sectors)
    STAGE2_SIZE=$(stat -f%z "$STAGE2_BIN" 2>/dev/null || stat -c%s "$STAGE2_BIN" 2>/dev/null)
    STAGE2_MAX_SIZE=$((8 * 512))  # 8 sectors = 4KB
    if [ "$STAGE2_SIZE" -gt "$STAGE2_MAX_SIZE" ]; then
        log_error "Stage 2 bootloader too large: $STAGE2_SIZE bytes (max: $STAGE2_MAX_SIZE bytes)"
        exit 1
    fi
    
    log_success "All required files found and validated"
}

# Create blank hard disk image
create_blank_image() {
    log_info "Creating blank hard disk image (${HDD_SIZE_MB}MB)..."
    
    # Create blank file filled with zeros
    # dd: copy data block by block
    # if=/dev/zero: input from zero-filled virtual device
    # of=$HDD_IMAGE: output file
    # bs=1M: block size 1MB
    # count=$HDD_SIZE_MB: number of blocks
    dd if=/dev/zero of="$HDD_IMAGE" bs=1M count=$HDD_SIZE_MB status=none
    
    log_success "Blank image created: $HDD_IMAGE"
}

# Install Stage 1 bootloader to MBR
install_stage1() {
    log_info "Installing Stage 1 bootloader to MBR (sector 0)..."
    
    # Install Stage 1 to first sector (MBR) of hard disk image
    # Copy exactly 512 bytes to sector 0
    # conv=notrunc: overwrite without truncating existing file
    dd if="$STAGE1_BIN" of="$HDD_IMAGE" bs=512 count=1 conv=notrunc status=none
    
    log_success "Stage 1 bootloader installed to MBR"
}

# Install Stage 2 bootloader starting from sector 2
install_stage2() {
    log_info "Installing Stage 2 bootloader starting from sector 2..."
    
    # Install Stage 2 starting from sector 2 (skip sector 1 for safety)
    # seek=2: skip 2 sectors (1024 bytes) in output file before writing
    # This leaves sector 1 empty and places Stage 2 at sectors 2-9
    dd if="$STAGE2_BIN" of="$HDD_IMAGE" bs=512 seek=2 conv=notrunc status=none
    
    log_success "Stage 2 bootloader installed starting from sector 2"
}

# Install kernel starting from sector 10
install_kernel() {
    log_info "Installing kernel starting from sector 10..."
    
    # Install kernel starting from sector 10 (matching KERNEL_START_SECTOR in stage2.asm)
    # seek=10: skip 10 sectors (5120 bytes) in output file before writing
    # This places kernel after Stage 1 (sector 0), gap (sector 1), and Stage 2 (sectors 2-9)
    dd if="$KERNEL_BIN" of="$HDD_IMAGE" bs=512 seek=10 conv=notrunc status=none
    
    log_success "Kernel installed starting from sector 10"
}

# Show hard disk image information
show_image_info() {
    log_info "Hard disk image information:"
    
    # Display file size
    HDD_SIZE=$(stat -f%z "$HDD_IMAGE" 2>/dev/null || stat -c%s "$HDD_IMAGE" 2>/dev/null)
    HDD_SIZE_KB=$((HDD_SIZE / 1024))
    echo "  Image size: $HDD_SIZE bytes ($HDD_SIZE_KB KB)"
    
    # Calculate total sectors
    TOTAL_SECTORS=$((HDD_SIZE / 512))
    echo "  Total sectors: $TOTAL_SECTORS"
    
    # Stage 1 bootloader information
    STAGE1_SIZE=$(stat -f%z "$STAGE1_BIN" 2>/dev/null || stat -c%s "$STAGE1_BIN" 2>/dev/null)
    echo "  Stage 1 Bootloader: sector 0 ($STAGE1_SIZE bytes)"
    
    # Stage 2 bootloader information
    STAGE2_SIZE=$(stat -f%z "$STAGE2_BIN" 2>/dev/null || stat -c%s "$STAGE2_BIN" 2>/dev/null)
    STAGE2_SECTORS=$(((STAGE2_SIZE + 511) / 512))  # Round up
    STAGE2_END_SECTOR=$((2 + STAGE2_SECTORS - 1))
    echo "  Stage 2 Bootloader: sectors 2-$STAGE2_END_SECTOR ($STAGE2_SIZE bytes)"
    
    # Kernel information
    KERNEL_SIZE=$(stat -f%z "$KERNEL_BIN" 2>/dev/null || stat -c%s "$KERNEL_BIN" 2>/dev/null)
    KERNEL_SECTORS=$(((KERNEL_SIZE + 511) / 512))  # Round up calculation
    KERNEL_END_SECTOR=$((10 + KERNEL_SECTORS - 1))
    echo "  Kernel: sectors 10-$KERNEL_END_SECTOR ($KERNEL_SIZE bytes)"
    
    # Verify boot signature
    echo
    log_info "Verifying boot signature..."
    
    # Check last 2 bytes for 0x55AA (little endian: 0xAA55)
    SIGNATURE=$(hexdump -s 510 -n 2 -e '1/2 "%04x"' "$HDD_IMAGE")
    if [ "$SIGNATURE" = "aa55" ]; then
        log_success "Boot signature verified: 0x55AA"
    else
        log_warning "Boot signature not found or incorrect: 0x$SIGNATURE"
        log_warning "Expected: 0xAA55"
    fi
}

# Error handling - cleanup on script failure
cleanup() {
    if [ $? -ne 0 ]; then
        log_error "Script failed! Cleaning up..."
        # Remove failed image file (optional)
        # rm -f "$HDD_IMAGE"
    fi
}

# Register signal handler (when script is interrupted with Ctrl+C, etc.)
trap cleanup EXIT

# Start script execution
main "$@"

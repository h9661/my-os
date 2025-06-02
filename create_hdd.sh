#!/bin/bash

# Hard Disk Image Creation Script
# Creates bootable hard disk image with bootloader and kernel

set -e  # Exit on error

# Constants
HDD_IMAGE="build/hard-disk.img"
BOOTLOADER_BIN="build/bootloader.bin"
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
    
    # Stage 3: Install bootloader (MBR - first sector)
    install_bootloader
    
    # Stage 4: Install kernel (starting from second sector)
    install_kernel
    
    # Stage 5: Show image information
    show_image_info
    
    log_success "Hard disk image created successfully: $HDD_IMAGE"
    log_info "You can now run: make run"
}

# Check required files exist
check_required_files() {
    log_info "Checking required files..."
    
    if [ ! -f "$BOOTLOADER_BIN" ]; then
        log_error "Bootloader binary not found: $BOOTLOADER_BIN"
        log_info "Please run: make bootloader"
        exit 1
    fi
    
    if [ ! -f "$KERNEL_BIN" ]; then
        log_error "Kernel binary not found: $KERNEL_BIN"
        log_info "Please run: make kernel"
        exit 1
    fi
    
    # Check bootloader size (must be exactly 512 bytes)
    BOOTLOADER_SIZE=$(stat -f%z "$BOOTLOADER_BIN" 2>/dev/null || stat -c%s "$BOOTLOADER_BIN" 2>/dev/null)
    if [ "$BOOTLOADER_SIZE" -ne 512 ]; then
        log_error "Bootloader must be exactly 512 bytes, but got $BOOTLOADER_SIZE bytes"
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

# Install bootloader to MBR
install_bootloader() {
    log_info "Installing bootloader to MBR (sector 0)..."
    
    # Install bootloader to first sector (MBR) of hard disk image
    # Copy exactly 512 bytes to sector 0
    # conv=notrunc: overwrite without truncating existing file
    dd if="$BOOTLOADER_BIN" of="$HDD_IMAGE" bs=512 count=1 conv=notrunc status=none
    
    log_success "Bootloader installed to MBR"
}

# Install kernel starting from sector 1
install_kernel() {
    log_info "Installing kernel starting from sector 1..."
    
    # Install kernel starting from second sector (sector 1)
    # seek=1: skip 1 sector (512 bytes) in output file before writing
    # This places kernel right after bootloader (sector 0)
    dd if="$KERNEL_BIN" of="$HDD_IMAGE" bs=512 seek=1 conv=notrunc status=none
    
    log_success "Kernel installed starting from sector 1"
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
    
    # Bootloader information
    BOOTLOADER_SIZE=$(stat -f%z "$BOOTLOADER_BIN" 2>/dev/null || stat -c%s "$BOOTLOADER_BIN" 2>/dev/null)
    echo "  Bootloader: sector 0 ($BOOTLOADER_SIZE bytes)"
    
    # Kernel information
    KERNEL_SIZE=$(stat -f%z "$KERNEL_BIN" 2>/dev/null || stat -c%s "$KERNEL_BIN" 2>/dev/null)
    KERNEL_SECTORS=$(((KERNEL_SIZE + 511) / 512))  # Round up calculation
    echo "  Kernel: sectors 1-$KERNEL_SECTORS ($KERNEL_SIZE bytes)"
    
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

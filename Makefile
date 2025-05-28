# Makefile for building a simple operating system

# Compiler and linker settings
CC = /opt/homebrew/bin/i686-elf-gcc
CFLAGS = -m32 -fno-pic -fno-builtin -fno-stack-protector -nostdlib -nodefaultlibs -Wall -Wextra -c
LD = /opt/homebrew/bin/i686-elf-ld
LDFLAGS = -Ttext 0x1000 -o
NASM = nasm
NASMFLAGS = -f elf32
OBJCOPY = /opt/homebrew/bin/i686-elf-objcopy

# Directories
BUILD_DIR = build
BOOT_DIR = boot
KERNEL_DIR = kernel

# Target files
OS_IMAGE = $(BUILD_DIR)/os-image.bin
HDD_IMAGE = $(BUILD_DIR)/hard-disk.img
BOOT_BIN = $(BUILD_DIR)/bootloader.bin
KERNEL_BIN = $(BUILD_DIR)/kernel.bin

# Source files
BOOT_SRC = $(BOOT_DIR)/bootloader.asm
KERNEL_ENTRY = $(KERNEL_DIR)/kernel_entry.asm
KERNEL_C = $(KERNEL_DIR)/kernel.c

# Object files
KERNEL_ENTRY_OBJ = $(BUILD_DIR)/kernel_entry.o
KERNEL_C_OBJ = $(BUILD_DIR)/kernel.o

.PHONY: all clean run debug bootloader kernel hdd

all: $(HDD_IMAGE)

bootloader: $(BOOT_BIN)

kernel: $(KERNEL_BIN)

hdd: bootloader kernel
	./create_hdd.sh

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build the final OS image
$(OS_IMAGE): $(BOOT_BIN) $(KERNEL_BIN) | $(BUILD_DIR)
	cat $^ > $@

# Create the hard disk image
$(HDD_IMAGE): $(BOOT_BIN) $(KERNEL_BIN) | $(BUILD_DIR)
	./create_hdd.sh

# Build the bootloader
$(BOOT_BIN): $(BOOT_SRC) $(BOOT_DIR)/gdt.asm $(BOOT_DIR)/disk.asm | $(BUILD_DIR)
	$(NASM) -f bin -o $@ $<

# Build the kernel
$(KERNEL_BIN): $(KERNEL_ENTRY_OBJ) $(KERNEL_C_OBJ) | $(BUILD_DIR)
	$(LD) -T $(KERNEL_DIR)/kernel.ld -o $(BUILD_DIR)/kernel_elf.o $^
	$(OBJCOPY) -O binary $(BUILD_DIR)/kernel_elf.o $@

# Compile kernel entry assembly
$(KERNEL_ENTRY_OBJ): $(KERNEL_ENTRY) | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) -o $@ $<

# Compile kernel C code
$(KERNEL_C_OBJ): $(KERNEL_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)/*

# Run the OS in QEMU as a hard disk
run: $(HDD_IMAGE)
	qemu-system-i386 -drive format=raw,file=$(HDD_IMAGE),if=ide,index=0,media=disk

# Debug with QEMU
debug: $(HDD_IMAGE)
	qemu-system-i386 -drive format=raw,file=$(HDD_IMAGE),if=ide,index=0,media=disk -S -s
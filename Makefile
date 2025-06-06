# Makefile for building a simple operating system

# Compiler and linker settings
CC = /opt/homebrew/bin/i686-elf-gcc
CFLAGS = -m32 -fno-pic -fno-builtin -fno-stack-protector -nostdlib -nodefaultlibs -Wall -Wextra -c -I$(KERNEL_DIR)
LD = /opt/homebrew/bin/i686-elf-ld
LDFLAGS = -T $(KERNEL_DIR)/kernel.ld -o
NASM = nasm
NASMFLAGS = -f elf32
OBJCOPY = /opt/homebrew/bin/i686-elf-objcopy

# Directories
BUILD_DIR = build
BOOT_DIR = boot
KERNEL_DIR = kernel
KERNEL_SRC_DIR = $(KERNEL_DIR)/src
KERNEL_INC_DIR = $(KERNEL_DIR)/include

# Target files
HDD_IMAGE = $(BUILD_DIR)/hard-disk.img
BOOT_BIN = $(BUILD_DIR)/bootloader.bin
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
KERNEL_ELF = $(BUILD_DIR)/kernel_elf.o

# Source files
BOOT_SRC = $(BOOT_DIR)/bootloader.asm
KERNEL_ENTRY = $(KERNEL_DIR)/kernel_entry.asm
KERNEL_MAIN = $(KERNEL_DIR)/kernel_main.c

# Kernel C source files
KERNEL_C_SOURCES = $(KERNEL_MAIN) \
                   $(KERNEL_SRC_DIR)/common/utils.c \
                   $(KERNEL_SRC_DIR)/memory/memory.c \
                   $(KERNEL_SRC_DIR)/vga/vga.c \
                   $(KERNEL_SRC_DIR)/terminal/terminal.c \
                   $(KERNEL_SRC_DIR)/cpu/cpu.c \
                   $(KERNEL_SRC_DIR)/cpu/fpu.c \
                   $(KERNEL_SRC_DIR)/interrupts/idt.c \
                   $(KERNEL_SRC_DIR)/interrupts/interrupt_handlers.c \
                   $(KERNEL_SRC_DIR)/keyboard/keyboard.c \
                   $(KERNEL_SRC_DIR)/process/process.c \
                   $(KERNEL_SRC_DIR)/syscalls/syscalls.c \
                   $(KERNEL_SRC_DIR)/storage/hdd.c \
                   $(KERNEL_SRC_DIR)/storage/fat32.c

# Assembly source files
KERNEL_ASM_SOURCES = $(KERNEL_ENTRY) \
                     $(KERNEL_SRC_DIR)/interrupts/interrupt_handlers.asm \
                     $(KERNEL_SRC_DIR)/process/context_switch.asm

# Object files
KERNEL_ENTRY_OBJ = $(BUILD_DIR)/kernel_entry.o
KERNEL_C_OBJS = $(BUILD_DIR)/kernel_main.o \
                $(BUILD_DIR)/utils.o \
                $(BUILD_DIR)/memory.o \
                $(BUILD_DIR)/vga.o \
                $(BUILD_DIR)/terminal.o \
                $(BUILD_DIR)/cpu.o \
                $(BUILD_DIR)/fpu.o \
                $(BUILD_DIR)/idt.o \
                $(BUILD_DIR)/interrupt_handlers.o \
                $(BUILD_DIR)/keyboard.o \
                $(BUILD_DIR)/process.o \
                $(BUILD_DIR)/syscalls.o \
                $(BUILD_DIR)/hdd.o \
                $(BUILD_DIR)/fat32.o

KERNEL_ASM_OBJS = $(BUILD_DIR)/interrupt_handlers_asm.o \
                  $(BUILD_DIR)/context_switch.o

.PHONY: all clean run debug stage1 stage2 kernel hdd structure help

all: $(HDD_IMAGE)

bootloader: $(BOOT_BIN)

kernel: $(KERNEL_BIN)

hdd: bootloader kernel
	./create_hdd.sh

# Show the project structure
structure:
	@echo "Project structure:"
	@find . -type f -name "*.c" -o -name "*.h" -o -name "*.asm" | grep -v build | sort

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Create the hard disk image
$(HDD_IMAGE): $(BOOT_BIN) $(KERNEL_BIN) | $(BUILD_DIR)
	./create_hdd.sh

# Build the bootloader
$(BOOT_BIN): $(BOOT_SRC) $(BOOT_DIR)/gdt.asm $(BOOT_DIR)/disk.asm | $(BUILD_DIR)
	$(NASM) -f bin -o $@ $<

# Build the kernel binary
$(KERNEL_BIN): $(KERNEL_ELF) | $(BUILD_DIR)
	$(OBJCOPY) -O binary $< $@

# Link the kernel ELF
$(KERNEL_ELF): $(KERNEL_ENTRY_OBJ) $(KERNEL_C_OBJS) $(KERNEL_ASM_OBJS) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) $@ $^

# Build kernel entry assembly
$(KERNEL_ENTRY_OBJ): $(KERNEL_ENTRY) | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) -o $@ $<

# Build main kernel C file
$(BUILD_DIR)/kernel_main.o: $(KERNEL_MAIN) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Build utils.c
$(BUILD_DIR)/utils.o: $(KERNEL_SRC_DIR)/common/utils.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Build memory.c
$(BUILD_DIR)/memory.o: $(KERNEL_SRC_DIR)/memory/memory.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Build vga.c
$(BUILD_DIR)/vga.o: $(KERNEL_SRC_DIR)/vga/vga.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Build cpu.c
$(BUILD_DIR)/cpu.o: $(KERNEL_SRC_DIR)/cpu/cpu.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Build fpu.c
$(BUILD_DIR)/fpu.o: $(KERNEL_SRC_DIR)/cpu/fpu.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Build interrupt-related C files
$(BUILD_DIR)/idt.o: $(KERNEL_SRC_DIR)/interrupts/idt.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/interrupt_handlers.o: $(KERNEL_SRC_DIR)/interrupts/interrupt_handlers.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Build keyboard.c
$(BUILD_DIR)/keyboard.o: $(KERNEL_SRC_DIR)/keyboard/keyboard.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Build interrupt assembly handlers
$(BUILD_DIR)/interrupt_handlers_asm.o: $(KERNEL_SRC_DIR)/interrupts/interrupt_handlers.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) -o $@ $<

# Build process.c
$(BUILD_DIR)/process.o: $(KERNEL_SRC_DIR)/process/process.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Build terminal.c
$(BUILD_DIR)/terminal.o: $(KERNEL_SRC_DIR)/terminal/terminal.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Build syscalls.c
$(BUILD_DIR)/syscalls.o: $(KERNEL_SRC_DIR)/syscalls/syscalls.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Build hdd.c
$(BUILD_DIR)/hdd.o: $(KERNEL_SRC_DIR)/storage/hdd.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Build fat32.c
$(BUILD_DIR)/fat32.o: $(KERNEL_SRC_DIR)/storage/fat32.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Build context_switch.asm
$(BUILD_DIR)/context_switch.o: $(KERNEL_SRC_DIR)/process/context_switch.asm | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) -o $@ $<

# Run the OS in QEMU
run: $(HDD_IMAGE)
	qemu-system-i386 -drive format=raw,file=$(HDD_IMAGE) -m 64

# Run the OS in QEMU with debugging
debug: $(HDD_IMAGE)
	qemu-system-i386 -drive format=raw,file=$(HDD_IMAGE) -m 64 -s -S

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)/*

# Help target
help:
	@echo "Available targets:"
	@echo "  all       - Build the complete OS image"
	@echo "  bootloader- Build only the bootloader"
	@echo "  kernel    - Build only the kernel"
	@echo "  hdd       - Create the hard disk image"
	@echo "  run       - Run the OS in QEMU"
	@echo "  debug     - Run the OS in QEMU with debugging"
	@echo "  clean     - Remove all build artifacts"
	@echo "  structure - Show project structure"
	@echo "  help      - Show this help message"
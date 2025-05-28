# Simple Operating System

A minimal operating system built from scratch with a modular, maintainable architecture.

## Project Structure

```
my-os/
├── boot/                          # Bootloader components
│   ├── bootloader.asm            # Main bootloader
│   ├── disk.asm                  # Disk I/O routines
│   └── gdt.asm                   # Global Descriptor Table
├── kernel/                        # Kernel components
│   ├── include/                   # Header files
│   │   ├── common/               # Common utilities
│   │   │   ├── types.h           # Basic type definitions
│   │   │   └── utils.h           # Utility functions
│   │   ├── cpu/                  # CPU-related headers
│   │   │   └── cpu.h             # CPU information and CPUID
│   │   ├── memory/               # Memory management
│   │   │   └── memory.h          # Memory detection and info
│   │   ├── vga/                  # VGA text mode
│   │   │   └── vga.h             # VGA display functions
│   │   └── kernel.h              # Main kernel header
│   ├── src/                      # Source implementations
│   │   ├── common/               # Common utilities
│   │   │   └── utils.c           # String and memory utilities
│   │   ├── cpu/                  # CPU management
│   │   │   └── cpu.c             # CPU detection and info
│   │   ├── memory/               # Memory management
│   │   │   └── memory.c          # Memory detection methods
│   │   └── vga/                  # VGA implementation
│   │       └── vga.c             # Terminal and display functions
│   ├── kernel_entry.asm          # Kernel entry point (assembly)
│   ├── kernel_main.c             # Main kernel code (C)
│   └── kernel.ld                 # Kernel linker script
├── build/                        # Build artifacts
├── docs/                         # Documentation
├── create_hdd.sh                 # Disk image creation script
├── Makefile                      # Build configuration
└── README.md                     # This file
```

## Features

- **Modular Architecture**: Well-organized codebase with clear separation of concerns
- **Memory Detection**: Multiple methods (CMOS, probing) for detecting system memory
- **CPU Information**: CPUID-based CPU vendor detection
- **VGA Text Mode**: Colorful terminal output with formatting functions
- **Hardware Abstraction**: Clean interfaces for hardware components

## Building

Requirements:
- `i686-elf-gcc` cross-compiler
- `nasm` assembler
- `qemu-system-i386` for testing

### Build Commands

```bash
# Build everything
make

# Build specific components
make bootloader  # Build only bootloader
make kernel     # Build only kernel

# Testing
make run        # Run in QEMU
make debug      # Run in QEMU with debugging

# Utilities
make structure  # Show project structure
make clean      # Clean build artifacts
make help       # Show all available targets
```

## Memory Detection

The kernel implements multiple memory detection methods:

1. **CMOS Method**: Reads memory size from CMOS registers (0x17-0x18)
2. **Probing Method**: Tests memory accessibility by writing/reading test values
3. **Extensible Design**: Easy to add E820 or other detection methods

## VGA Terminal

Features include:
- Color support with 16 foreground/background colors
- Formatted output functions
- Header and separator printing
- Automatic line wrapping and scrolling

## CPU Detection

- CPUID instruction support detection
- CPU vendor string extraction
- Maximum CPUID level detection
- Extensible for additional CPU features

## Development

### Adding New Modules

1. Create header file in `kernel/include/[module]/`
2. Create implementation in `kernel/src/[module]/`
3. Include in main kernel header `kernel/include/kernel.h`
4. Add to Makefile build rules

### Code Style

- Use descriptive function and variable names
- Include comprehensive comments
- Follow consistent indentation (4 spaces)
- Use proper header guards
- Organize functions logically within files

## Memory Layout

```
0x0000-0x7BFF: Available for kernel use
0x7C00-0x7DFF: Bootloader code (loaded by BIOS)
0x1000+:       Kernel code (loaded by bootloader)
0x90000:       Stack for kernel functions
0xB8000:       VGA text buffer
```

## License

This project is for educational purposes. Feel free to use and modify as needed.

## Contributing

1. Follow the existing code style
2. Add appropriate documentation
3. Test changes thoroughly
4. Update this README if adding new features

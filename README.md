# A simple operating system 

A minimal operating system built from scratch featuring:
- Custom bootloader and kernel
- VGA text mode and graphics support
- Keyboard input handling
- Simple window manager/shell
- Text Editor (like Notepad)
- Basic Web Browser (HTML viewer)

## Requirements

To build this OS, you need:
- **NASM** - Netwide Assembler for bootloader
- **GCC Cross-Compiler** (i686-elf-gcc) - For kernel compilation
- **QEMU** - For testing the OS
- **xorriso** or **mkisofs** - For creating bootable ISO

### Windows Setup (using WSL or MinGW)

1. Install WSL2 with Ubuntu, or use MSYS2/MinGW
2. Install required tools:
```bash
# Ubuntu/WSL
sudo apt update
sudo apt install nasm gcc make qemu-system-x86 xorriso grub-pc-bin grub-common

# For cross-compiler (recommended)
sudo apt install gcc-multilib
```

## Building

```bash
# Build the OS
make all

# Create bootable ISO
make iso

# Run in QEMU
make run

# Clean build files
make clean
```

## Project Structure

```
kernel/
├── boot/
│   └── boot.asm          # Multiboot bootloader
├── kernel/
│   ├── kernel.c          # Kernel entry point
│   ├── kernel.h          # Kernel header
│   ├── vga.c             # VGA driver
│   ├── vga.h             # VGA header
│   ├── keyboard.c        # Keyboard driver
│   ├── keyboard.h        # Keyboard header
│   ├── idt.c             # Interrupt Descriptor Table
│   ├── idt.h             # IDT header
│   ├── io.h              # I/O port operations
│   ├── string.c          # String utilities
│   ├── string.h          # String header
│   ├── memory.c          # Memory management
│   ├── memory.h          # Memory header
│   ├── shell.c           # Shell/Window manager
│   ├── shell.h           # Shell header
│   ├── apps/
│   │   ├── notepad.c     # Text editor
│   │   ├── notepad.h     # Notepad header
│   │   ├── browser.c     # Web browser
│   │   └── browser.h     # Browser header
│   └── linker.ld         # Linker script
├── iso/
│   └── grub/
│       └── grub.cfg      # GRUB configuration
├── Makefile              # Build system
└── README.md             # This file
```

## Features

### Text Editor (Notepad)
- Create and edit text files
- Basic cursor navigation
- Save functionality (to memory buffer)

### Web Browser
- Basic HTML rendering
- Supports: headings, paragraphs, links, lists
- Simple text-based display

## Controls

- **F1** - Launch Text Editor
- **F2** - Launch Web Browser
- **ESC** - Return to Shell
- **Arrow Keys** - Navigate
- **Enter** - Execute command / Select

## License

Educational project - Free to use and modify.

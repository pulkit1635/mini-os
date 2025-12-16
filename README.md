# MiniOS

A tiny hobby operating system built from scratch to show how boot code, a kernel, and a couple of apps fit together. It ships with:
- Custom bootloader and kernel
- VGA text mode and graphics support
- Keyboard input handling
- Simple window manager/shell
- Text Editor (like Notepad)
- Basic Web Browser (HTML viewer)

## Requirements

To build the OS you need:
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

## Quick Start

On Linux/WSL:
```bash
./build.sh        # build the kernel
./build.sh run    # build and boot in QEMU
./build.sh iso    # build a bootable ISO
```

On Windows (CMD or PowerShell):
```cmd
build.bat
build.bat run
```

If you prefer plain Make:
```bash
make all   # build the OS
make iso   # create a bootable ISO
make run   # launch QEMU with the built kernel
make clean # remove build output
```

## Project Structure (at a glance)

```
kernel/
├── boot/boot.asm        # Multiboot bootloader
├── kernel.c             # Kernel entry point and boot flow
├── vga.*                # VGA text-mode driver
├── keyboard.*           # PS/2 keyboard driver
├── idt.*                # Interrupt handling
├── memory.*             # Tiny heap allocator
├── string.*             # Minimal libc-style helpers
├── shell.*              # Command shell + launcher
└── apps/
    ├── notepad.*        # Text editor
    └── browser.*        # Text-mode browser
iso/                     # GRUB config and ISO layout
Makefile, build.sh/.bat  # Build scripts
README.md                # This file
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

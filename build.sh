#!/bin/bash
# MiniOS Build Script for Linux/WSL
# Requires: nasm, i686-elf-gcc (or gcc with multilib), qemu, grub-mkrescue

set -e

echo "========================================"
echo "MiniOS Build System"
echo "========================================"
echo

# Check for build tools
check_tool() {
    if command -v "$1" &> /dev/null; then
        echo "[OK] $1 found"
        return 0
    else
        echo "[ERROR] $1 not found"
        return 1
    fi
}

check_tool nasm || exit 1

# Find C compiler
if command -v i686-elf-gcc &> /dev/null; then
    CC="i686-elf-gcc"
    LD="i686-elf-ld"
    EXTRA_CFLAGS=""
    EXTRA_LDFLAGS=""
    echo "[OK] i686-elf-gcc found"
else
    CC="gcc"
    LD="ld"
    EXTRA_CFLAGS="-m32"
    EXTRA_LDFLAGS="-m elf_i386"
    echo "[OK] Using gcc with -m32"
fi

# Create build directories
mkdir -p build/apps

# Compiler flags
CFLAGS="-std=gnu99 -ffreestanding -O2 -Wall -Wextra -nostdlib -nostdinc -fno-builtin -fno-stack-protector -fno-pie $EXTRA_CFLAGS"
LDFLAGS="-T kernel/linker.ld -nostdlib $EXTRA_LDFLAGS"

echo
echo "[BUILD] Assembling bootloader..."
nasm -f elf32 boot/boot.asm -o build/boot.o
echo "[OK] boot.o created"

echo "[BUILD] Compiling kernel..."
$CC $CFLAGS -Ikernel -c kernel/string.c -o build/string.o
$CC $CFLAGS -Ikernel -c kernel/vga.c -o build/vga.o
$CC $CFLAGS -Ikernel -c kernel/idt.c -o build/idt.o
$CC $CFLAGS -Ikernel -c kernel/keyboard.c -o build/keyboard.o
$CC $CFLAGS -Ikernel -c kernel/memory.c -o build/memory.o
$CC $CFLAGS -Ikernel -c kernel/audio.c -o build/audio.o
$CC $CFLAGS -Ikernel -c kernel/shell.c -o build/shell.o
$CC $CFLAGS -Ikernel -c kernel/apps/notepad.c -o build/apps/notepad.o
$CC $CFLAGS -Ikernel -c kernel/apps/css.c -o build/apps/css.o
$CC $CFLAGS -Ikernel -c kernel/apps/javascript.c -o build/apps/javascript.o
$CC $CFLAGS -Ikernel -c kernel/apps/browser.c -o build/apps/browser.o
$CC $CFLAGS -Ikernel -c kernel/kernel.c -o build/kernel.o
echo "[OK] All C files compiled"

echo "[BUILD] Linking kernel..."
$LD $LDFLAGS -o build/kernel.bin \
    build/boot.o \
    build/string.o \
    build/vga.o \
    build/idt.o \
    build/keyboard.o \
    build/memory.o \
    build/audio.o \
    build/shell.o \
    build/apps/notepad.o \
    build/apps/css.o \
    build/apps/javascript.o \
    build/apps/browser.o
echo "[OK] kernel.bin created"

# Create ISO if requested
if [ "$1" = "iso" ] || [ "$1" = "run-iso" ]; then
    echo
    echo "[BUILD] Creating ISO image..."
    mkdir -p iso/boot/grub
    cp build/kernel.bin iso/boot/kernel.bin
    
    if command -v grub-mkrescue &> /dev/null; then
        grub-mkrescue -o minios.iso iso 2>/dev/null
        echo "[OK] minios.iso created"
    else
        echo "[WARN] grub-mkrescue not found, ISO not created"
    fi
fi

echo
echo "========================================"
echo "Build Complete!"
echo "========================================"
echo

# Run if requested
case "$1" in
    run)
        echo "[RUN] Starting QEMU..."
        qemu-system-i386 -kernel build/kernel.bin
        ;;
    run-iso)
        if [ -f minios.iso ]; then
            echo "[RUN] Starting QEMU with ISO..."
            qemu-system-i386 -cdrom minios.iso
        else
            echo "[ERROR] minios.iso not found"
        fi
        ;;
    debug)
        echo "[DEBUG] Starting QEMU with debug output..."
        qemu-system-i386 -kernel build/kernel.bin -d int -no-reboot
        ;;
    *)
        echo "Usage:"
        echo "  ./build.sh          - Build kernel only"
        echo "  ./build.sh iso      - Build kernel and create ISO"
        echo "  ./build.sh run      - Build and run in QEMU"
        echo "  ./build.sh run-iso  - Build ISO and run in QEMU"
        echo "  ./build.sh debug    - Build and run with debug"
        ;;
esac

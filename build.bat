@echo off
REM MiniOS Build Script for Windows
REM Requires: NASM, i686-elf cross-compiler (or MinGW), QEMU

echo ========================================
echo MiniOS Build System
echo ========================================
echo.

REM Check for build tools
where nasm >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] NASM not found. Please install NASM.
    echo Download from: https://www.nasm.us/
    exit /b 1
)
echo [OK] NASM found

REM Try to find cross-compiler or MinGW gcc
where i686-elf-gcc >nul 2>&1
if %ERRORLEVEL% equ 0 (
    set CC=i686-elf-gcc
    set LD=i686-elf-ld
    echo [OK] Cross-compiler found
) else (
    where gcc >nul 2>&1
    if %ERRORLEVEL% equ 0 (
        set CC=gcc
        set LD=ld
        set EXTRA_FLAGS=-m32
        echo [OK] GCC found ^(using -m32 mode^)
    ) else (
        echo [ERROR] No suitable C compiler found.
        echo Please install i686-elf cross-compiler or MinGW.
        exit /b 1
    )
)

REM Create build directory
if not exist build mkdir build
if not exist build\apps mkdir build\apps

echo.
echo [BUILD] Assembling bootloader...
nasm -f elf32 boot\boot.asm -o build\boot.o
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Assembly failed!
    exit /b 1
)
echo [OK] boot.o created

echo [BUILD] Compiling kernel...
set CFLAGS=-std=gnu99 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin -fno-stack-protector %EXTRA_FLAGS%

%CC% %CFLAGS% -Ikernel -c kernel\string.c -o build\string.o
%CC% %CFLAGS% -Ikernel -c kernel\vga.c -o build\vga.o
%CC% %CFLAGS% -Ikernel -c kernel\idt.c -o build\idt.o
%CC% %CFLAGS% -Ikernel -c kernel\keyboard.c -o build\keyboard.o
%CC% %CFLAGS% -Ikernel -c kernel\memory.c -o build\memory.o
%CC% %CFLAGS% -Ikernel -c kernel\audio.c -o build\audio.o
%CC% %CFLAGS% -Ikernel -c kernel\shell.c -o build\shell.o
%CC% %CFLAGS% -Ikernel -c kernel\apps\notepad.c -o build\apps\notepad.o
%CC% %CFLAGS% -Ikernel -c kernel\apps\css.c -o build\apps\css.o
%CC% %CFLAGS% -Ikernel -c kernel\apps\javascript.c -o build\apps\javascript.o
%CC% %CFLAGS% -Ikernel -c kernel\apps\browser.c -o build\apps\browser.o
%CC% %CFLAGS% -Ikernel -c kernel\kernel.c -o build\kernel.o

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Compilation failed!
    exit /b 1
)
echo [OK] All C files compiled

echo [BUILD] Linking kernel...
%LD% -T kernel\linker.ld -nostdlib -o build\kernel.bin ^
    build\boot.o ^
    build\string.o ^
    build\vga.o ^
    build\idt.o ^
    build\keyboard.o ^
    build\memory.o ^
    build\audio.o ^
    build\shell.o ^
    build\apps\notepad.o ^
    build\apps\css.o ^
    build\apps\javascript.o ^
    build\apps\browser.o

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Linking failed!
    exit /b 1
)
echo [OK] kernel.bin created

echo.
echo ========================================
echo Build Complete!
echo ========================================
echo.
echo To run in QEMU:
echo   qemu-system-i386 -kernel build\kernel.bin
echo.
echo To create ISO (requires grub-mkrescue):
echo   1. Copy build\kernel.bin to iso\boot\kernel.bin
echo   2. Run: grub-mkrescue -o minios.iso iso
echo.

if "%1"=="run" (
    echo [RUN] Starting QEMU...
    qemu-system-i386 -kernel build\kernel.bin
)

exit /b 0

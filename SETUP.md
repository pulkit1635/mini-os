# MiniOS Development Environment Setup Guide for Windows

## Quick Start

### Option 1: Using WSL2 (Recommended)

1. **Enable WSL2** (if not already):
   ```powershell
   wsl --install
   ```

2. **Install Ubuntu from Microsoft Store**

3. **In WSL2 Ubuntu terminal, install tools:**
   ```bash
   sudo apt update
   sudo apt install -y nasm gcc make qemu-system-x86 xorriso mtools grub-pc-bin grub-common gcc-multilib
   ```

4. **Navigate to project and build:**
   ```bash
   cd /mnt/c/Users/Harshit/Desktop/kernel
   chmod +x build.sh
   ./build.sh run
   ```

### Option 2: Using MSYS2/MinGW

1. **Download and install MSYS2** from: https://www.msys2.org/

2. **Open MSYS2 MINGW32 terminal and install tools:**
   ```bash
   pacman -Syu
   pacman -S mingw-w64-i686-gcc mingw-w64-i686-binutils nasm make
   ```

3. **Download QEMU for Windows** from: https://www.qemu.org/download/#windows

4. **Build the kernel:**
   ```bash
   cd /c/Users/Harshit/Desktop/kernel
   ./build.sh run
   ```

### Option 3: Using Docker

1. **Install Docker Desktop for Windows**

2. **Create and run build container:**
   ```powershell
   docker run -it --rm -v ${PWD}:/os -w /os ubuntu:22.04 bash -c "
     apt update && 
         apt install -y nasm gcc make qemu-system-x86 xorriso mtools grub-pc-bin grub-common gcc-multilib &&
     chmod +x build.sh &&
     ./build.sh iso
   "
   ```

## Building the Kernel

### From WSL/Linux:
```bash
# Just build
./build.sh

# Build and run in QEMU
./build.sh run

# Build ISO
./build.sh iso

# Build ISO and run
./build.sh run-iso
```

### From Windows CMD:
```cmd
build.bat
build.bat run
```

## Running in QEMU

After building, you can run:
```bash
# Run kernel directly
qemu-system-i386 -kernel build/kernel.bin

# Run from ISO
qemu-system-i386 -cdrom minios.iso

# Run with specific memory
qemu-system-i386 -kernel build/kernel.bin -m 128M
```

## Testing the OS

Once the OS boots:

1. **Press any key** to start the shell
2. **Type `help`** to see available commands
3. **Press F1** to open the Text Editor (Notepad)
4. **Press F2** to open the Web Browser
5. **Press ESC** to return to shell from any application

### Shell Commands:
- `help` - Show help
- `clear` - Clear screen
- `notepad` - Open text editor
- `browser` - Open web browser
- `meminfo` - Show memory info
- `about` - About MiniOS
- `reboot` - Reboot system

### Notepad Controls:
- Arrow keys - Move cursor
- Enter - New line
- Backspace - Delete character
- Ctrl+N - New document
- ESC - Exit to shell

### Browser Controls:
- Up/Down - Scroll
- Tab - Select next link
- Enter - Open link
- Backspace - Go home
- ESC - Exit to shell

## Troubleshooting

### "No compiler found"
Install GCC cross-compiler or use WSL2 with multilib support:
```bash
sudo apt install gcc-multilib
```

### "NASM not found"
Install NASM:
- Windows: https://www.nasm.us/
- WSL/Linux: `sudo apt install nasm`

### "grub-mkrescue not found"
This is only needed for ISO creation:
```bash
sudo apt install grub-pc-bin grub-common xorriso mtools
```

### "Cannot run 32-bit code"
Ensure you have 32-bit support:
```bash
sudo apt install libc6-dev-i386
```

## Project Structure

```
kernel/
├── boot/
│   └── boot.asm          # Multiboot bootloader & ISRs
├── kernel/
│   ├── kernel.c          # Kernel entry point
│   ├── kernel.h          # Kernel header
│   ├── vga.c/h           # VGA text mode driver
│   ├── keyboard.c/h      # PS/2 keyboard driver
│   ├── idt.c/h           # Interrupt handling
│   ├── memory.c/h        # Memory management
│   ├── string.c/h        # String utilities
│   ├── shell.c/h         # Command shell
│   ├── io.h              # I/O port operations
│   ├── linker.ld         # Linker script
│   └── apps/
│       ├── notepad.c/h   # Text editor
│       └── browser.c/h   # Web browser
├── iso/
│   └── boot/grub/
│       └── grub.cfg      # GRUB configuration
├── build/                # Build output (created during build)
├── Makefile              # Make build system
├── build.sh              # Linux/WSL build script
├── build.bat             # Windows build script
├── README.md             # Project documentation
└── SETUP.md              # This file
```

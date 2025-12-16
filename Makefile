# MiniOS Makefile
# Build system for MiniOS kernel

# Compilers and tools
ASM = nasm
CC = i686-elf-gcc
LD = i686-elf-ld

# If cross-compiler not available, try regular gcc with -m32
ifeq ($(shell which $(CC) 2>/dev/null),)
    CC = gcc
    LD = ld
    EXTRA_CFLAGS = -m32
    EXTRA_LDFLAGS = -m elf_i386
endif

# Flags
ASMFLAGS = -f elf32
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -nostdlib -nostdinc \
         -fno-builtin -fno-stack-protector -fno-pie -no-pie $(EXTRA_CFLAGS)
LDFLAGS = -T kernel/linker.ld -nostdlib $(EXTRA_LDFLAGS)

# Directories
BOOT_DIR = boot
KERNEL_DIR = kernel
ISO_DIR = iso
BUILD_DIR = build

# Source files
ASM_SOURCES = $(BOOT_DIR)/boot.asm
C_SOURCES = $(KERNEL_DIR)/kernel.c \
			$(KERNEL_DIR)/vga.c \
			$(KERNEL_DIR)/idt.c \
			$(KERNEL_DIR)/keyboard.c \
			$(KERNEL_DIR)/memory.c \
			$(KERNEL_DIR)/string.c \
			$(KERNEL_DIR)/audio.c \
			$(KERNEL_DIR)/shell.c \
			$(KERNEL_DIR)/apps/notepad.c \
			$(KERNEL_DIR)/apps/css.c \
			$(KERNEL_DIR)/apps/javascript.c \
			$(KERNEL_DIR)/apps/browser.c

# Object files
ASM_OBJECTS = $(BUILD_DIR)/boot.o
C_OBJECTS = $(patsubst $(KERNEL_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))

# Output files
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
ISO_FILE = minios.iso

# Default target
all: $(KERNEL_BIN)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/apps

# Assemble boot code
$(BUILD_DIR)/boot.o: $(BOOT_DIR)/boot.asm | $(BUILD_DIR)
	$(ASM) $(ASMFLAGS) $< -o $@

# Compile C source files
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(KERNEL_DIR) -c $< -o $@

# Link kernel
$(KERNEL_BIN): $(ASM_OBJECTS) $(C_OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

# Create ISO image
iso: $(KERNEL_BIN)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_BIN) $(ISO_DIR)/boot/kernel.bin
	cp $(ISO_DIR)/boot/grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg 2>/dev/null || true
	grub-mkrescue -o $(ISO_FILE) $(ISO_DIR)

# Run in QEMU
run: $(KERNEL_BIN)
	qemu-system-i386 -kernel $(KERNEL_BIN)

# Run ISO in QEMU
run-iso: iso
	qemu-system-i386 -cdrom $(ISO_FILE)

# Run with debug output
debug: $(KERNEL_BIN)
	qemu-system-i386 -kernel $(KERNEL_BIN) -d int -no-reboot

# Clean build files
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(ISO_FILE)

# Phony targets
.PHONY: all iso run run-iso debug clean

# Dependencies
$(BUILD_DIR)/kernel.o: $(KERNEL_DIR)/kernel.c $(KERNEL_DIR)/kernel.h $(KERNEL_DIR)/vga.h $(KERNEL_DIR)/idt.h $(KERNEL_DIR)/keyboard.h $(KERNEL_DIR)/memory.h $(KERNEL_DIR)/string.h $(KERNEL_DIR)/shell.h $(KERNEL_DIR)/io.h
$(BUILD_DIR)/vga.o: $(KERNEL_DIR)/vga.c $(KERNEL_DIR)/vga.h $(KERNEL_DIR)/io.h $(KERNEL_DIR)/string.h
$(BUILD_DIR)/idt.o: $(KERNEL_DIR)/idt.c $(KERNEL_DIR)/idt.h $(KERNEL_DIR)/io.h $(KERNEL_DIR)/vga.h $(KERNEL_DIR)/string.h
$(BUILD_DIR)/keyboard.o: $(KERNEL_DIR)/keyboard.c $(KERNEL_DIR)/keyboard.h $(KERNEL_DIR)/idt.h $(KERNEL_DIR)/io.h $(KERNEL_DIR)/vga.h
$(BUILD_DIR)/memory.o: $(KERNEL_DIR)/memory.c $(KERNEL_DIR)/memory.h $(KERNEL_DIR)/string.h
$(BUILD_DIR)/string.o: $(KERNEL_DIR)/string.c $(KERNEL_DIR)/string.h
$(BUILD_DIR)/shell.o: $(KERNEL_DIR)/shell.c $(KERNEL_DIR)/shell.h $(KERNEL_DIR)/vga.h $(KERNEL_DIR)/keyboard.h $(KERNEL_DIR)/string.h $(KERNEL_DIR)/memory.h $(KERNEL_DIR)/apps/notepad.h $(KERNEL_DIR)/apps/browser.h
$(BUILD_DIR)/audio.o: $(KERNEL_DIR)/audio.c $(KERNEL_DIR)/audio.h $(KERNEL_DIR)/io.h
$(BUILD_DIR)/apps/notepad.o: $(KERNEL_DIR)/apps/notepad.c $(KERNEL_DIR)/apps/notepad.h $(KERNEL_DIR)/vga.h $(KERNEL_DIR)/keyboard.h $(KERNEL_DIR)/string.h $(KERNEL_DIR)/memory.h $(KERNEL_DIR)/io.h
$(BUILD_DIR)/apps/css.o: $(KERNEL_DIR)/apps/css.c $(KERNEL_DIR)/apps/css.h $(KERNEL_DIR)/vga.h $(KERNEL_DIR)/string.h
$(BUILD_DIR)/apps/javascript.o: $(KERNEL_DIR)/apps/javascript.c $(KERNEL_DIR)/apps/javascript.h $(KERNEL_DIR)/string.h $(KERNEL_DIR)/memory.h
$(BUILD_DIR)/apps/browser.o: $(KERNEL_DIR)/apps/browser.c $(KERNEL_DIR)/apps/browser.h $(KERNEL_DIR)/apps/css.h $(KERNEL_DIR)/apps/javascript.h $(KERNEL_DIR)/audio.h $(KERNEL_DIR)/vga.h $(KERNEL_DIR)/keyboard.h $(KERNEL_DIR)/string.h $(KERNEL_DIR)/memory.h $(KERNEL_DIR)/io.h

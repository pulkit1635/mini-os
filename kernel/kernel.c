/*
 * MiniOS Kernel
 * Brings the machine up, wires interrupts, and hands control to the shell apps.
 */

#include "kernel.h"
#include "vga.h"
#include "idt.h"
#include "keyboard.h"
#include "memory.h"
#include "string.h"
#include "shell.h"
#include "io.h"

void kernel_panic(const char* message) {
    cli();
    
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
    vga_clear();
    
    vga_puts("\n\n");
    vga_puts("  *** KERNEL PANIC ***\n\n");
    vga_puts("  ");
    vga_puts(message);
    vga_puts("\n\n");
    vga_puts("  System halted. Please restart your computer.\n");
    
    // Halt forever
    for (;;) {
        hlt();
    }
}

void kernel_main(uint32_t magic, uint32_t* mboot_info) {
    (void)mboot_info;  // Suppress unused warning for now
    
    // Initialize VGA
    vga_init();
    
    // Show boot message
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    vga_puts("MiniOS Kernel Loading...\n");
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    
    // Verify multiboot
    if (magic != MULTIBOOT_MAGIC) {
        kernel_panic("Invalid multiboot magic number!");
    }
    vga_puts("[OK] Multiboot verified\n");
    
    // Initialize IDT (Interrupt Descriptor Table)
    vga_puts("[..] Initializing IDT...\n");
    idt_init();
    vga_puts("[OK] IDT initialized\n");
    
    // Initialize memory manager
    vga_puts("[..] Initializing memory manager...\n");
    memory_init((void*)KERNEL_HEAP_START, KERNEL_HEAP_SIZE);
    vga_printf("[OK] Heap: %u bytes at 0x%X\n", KERNEL_HEAP_SIZE, KERNEL_HEAP_START);
    
    // Initialize keyboard
    vga_puts("[..] Initializing keyboard...\n");
    keyboard_init();
    vga_puts("[OK] Keyboard initialized\n");
    
    // Enable interrupts
    vga_puts("[..] Enabling interrupts...\n");
    sti();
    vga_puts("[OK] Interrupts enabled\n");
    
    // Boot complete
    vga_puts("\n");
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    vga_puts("=================================\n");
    vga_puts("  MiniOS v1.0 - Boot Complete!  \n");
    vga_puts("=================================\n");
    vga_set_color(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    vga_puts("\nPress any key to start shell...\n");
    
    // Wait for keypress
    keyboard_getchar();
    
    // Start shell
    shell_init();
    shell_run();
    
    // Should never reach here
    kernel_panic("Shell exited unexpectedly!");
}

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
#include "disk.h"
#include "network.h"
#include "gui.h"
#include "audio.h"

// Provided by the linker; marks the end of the kernel image
extern uint32_t _kernel_end;

// Align a value up to the nearest 4 KB boundary
static inline uint32_t align4k(uint32_t addr) {
    return (addr + 0xFFF) & ~0xFFF;
}

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
    uint32_t heap_start = align4k((uint32_t)&_kernel_end);
    memory_init((void*)heap_start, KERNEL_HEAP_SIZE);
    vga_printf("[OK] Heap: %u bytes at 0x%X (kernel end 0x%X)\n", KERNEL_HEAP_SIZE, heap_start, (uint32_t)&_kernel_end);
    
    // Initialize keyboard
    vga_puts("[..] Initializing keyboard...\n");
    keyboard_init();
    vga_puts("[OK] Keyboard initialized\n");
    
    // Initialize disk subsystem
    vga_puts("[..] Initializing disk subsystem...\n");
    disk_init();
    vga_printf("[OK] Detected %d disk(s)\n", disk_get_count());
    
    // Initialize network subsystem
    vga_puts("[..] Initializing network...\n");
    network_init();
    vga_puts("[OK] Network initialized (WiFi simulated)\n");
    
    // Initialize audio
    vga_puts("[..] Initializing audio...\n");
    audio_init();
    vga_puts("[OK] Audio initialized (PC Speaker)\n");
    
    // Initialize GUI subsystem
    vga_puts("[..] Initializing GUI...\n");
    gui_init();
    vga_puts("[OK] GUI initialized\n");
    
    // Configure PIC interrupt masks: mask timer (IRQ0/bit0), keep keyboard (IRQ1/bit1) unmasked
    vga_puts("[..] Configuring interrupt masks...\n");
    uint8_t pic_mask = inb(0x21);
    pic_mask |= 0x01;   // Mask timer interrupt (IRQ0)
    pic_mask &= ~0x02;  // Unmask keyboard interrupt (IRQ1) - make sure it stays enabled
    outb(0x21, pic_mask);
    vga_puts("[OK] Interrupt masks configured\n");
    
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
    
    // Play startup sound
    audio_play_effect(SFX_STARTUP);
    
    // Wait for keypress (fallback to polling in case IRQ1 isn't firing)
    keyboard_wait_char_poll();
    
    // Start shell
    shell_init();
    shell_run();
    
    // Should never reach here
    kernel_panic("Shell exited unexpectedly!");
}

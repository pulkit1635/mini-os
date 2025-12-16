#include "idt.h"
#include "io.h"
#include "vga.h"
#include "string.h"

// IDT with 256 entries
static idt_entry_t idt[256];
static idt_ptr_t idt_ptr;

// IRQ handlers array
static irq_handler_t irq_handlers[16] = { 0 };

// Exception messages
static const char* exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 FPU Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Security Exception",
    "Reserved"
};

void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = selector;
    idt[num].zero = 0;
    idt[num].flags = flags;
}

// Remap the PIC (Programmable Interrupt Controller)
static void pic_remap(void) {
    // Save masks
    uint8_t mask1 = inb(0x21);
    uint8_t mask2 = inb(0xA1);
    
    // Start initialization sequence
    outb(0x20, 0x11);
    io_wait();
    outb(0xA0, 0x11);
    io_wait();
    
    // Set vector offsets
    outb(0x21, 0x20);  // Master PIC vector offset (IRQ0-7 -> INT 32-39)
    io_wait();
    outb(0xA1, 0x28);  // Slave PIC vector offset (IRQ8-15 -> INT 40-47)
    io_wait();
    
    // Tell Master PIC there's a slave at IRQ2
    outb(0x21, 0x04);
    io_wait();
    // Tell Slave PIC its cascade identity
    outb(0xA1, 0x02);
    io_wait();
    
    // Set 8086 mode
    outb(0x21, 0x01);
    io_wait();
    outb(0xA1, 0x01);
    io_wait();
    
    // Restore masks
    outb(0x21, mask1);
    outb(0xA1, mask2);
}

void idt_init(void) {
    // Set up IDT pointer
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base = (uint32_t)&idt;
    
    // Clear IDT
    memset(&idt, 0, sizeof(idt_entry_t) * 256);
    
    // Remap PIC
    pic_remap();
    
    // Set ISR gates (exceptions)
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);
    idt_set_gate(2, (uint32_t)isr2, 0x08, 0x8E);
    idt_set_gate(3, (uint32_t)isr3, 0x08, 0x8E);
    idt_set_gate(4, (uint32_t)isr4, 0x08, 0x8E);
    idt_set_gate(5, (uint32_t)isr5, 0x08, 0x8E);
    idt_set_gate(6, (uint32_t)isr6, 0x08, 0x8E);
    idt_set_gate(7, (uint32_t)isr7, 0x08, 0x8E);
    idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E);
    idt_set_gate(9, (uint32_t)isr9, 0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);
    
    // Set IRQ gates (hardware interrupts)
    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2, 0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3, 0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4, 0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5, 0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6, 0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7, 0x08, 0x8E);
    idt_set_gate(40, (uint32_t)irq8, 0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9, 0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);
    
    // Load IDT
    idt_flush((uint32_t)&idt_ptr);
}

void irq_register_handler(int irq, irq_handler_t handler) {
    if (irq >= 0 && irq < 16) {
        irq_handlers[irq] = handler;
    }
}

// ISR handler (called from assembly)
void isr_handler(registers_t* regs) {
    vga_set_color(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED));
    vga_printf("\n*** EXCEPTION: %s ***\n", exception_messages[regs->int_no]);
    vga_printf("Error Code: 0x%X\n", regs->err_code);
    vga_printf("EIP: 0x%X  CS: 0x%X  EFLAGS: 0x%X\n", regs->eip, regs->cs, regs->eflags);
    vga_printf("EAX: 0x%X  EBX: 0x%X  ECX: 0x%X  EDX: 0x%X\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
    vga_printf("ESP: 0x%X  EBP: 0x%X  ESI: 0x%X  EDI: 0x%X\n", regs->esp, regs->ebp, regs->esi, regs->edi);
    vga_printf("\nSystem Halted.\n");
    
    // Halt the system
    for (;;) {
        hlt();
    }
}

// IRQ handler (called from assembly)
void irq_handler(registers_t* regs) {
    // Call registered handler if exists
    int irq = regs->int_no - 32;
    if (irq >= 0 && irq < 16 && irq_handlers[irq]) {
        irq_handlers[irq](regs);
    }
    
    // Send EOI (End of Interrupt) to PIC
    if (regs->int_no >= 40) {
        // Send to slave PIC
        outb(0xA0, 0x20);
    }
    // Send to master PIC
    outb(0x20, 0x20);
}

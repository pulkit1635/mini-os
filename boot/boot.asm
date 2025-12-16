; MiniOS Bootloader
; Multiboot compliant bootloader for GRUB

MBALIGN  equ  1 << 0                ; Align loaded modules on page boundaries
MEMINFO  equ  1 << 1                ; Provide memory map
FLAGS    equ  MBALIGN | MEMINFO     ; Multiboot flag field
MAGIC    equ  0x1BADB002            ; Magic number for bootloader
CHECKSUM equ -(MAGIC + FLAGS)       ; Checksum for verification

; Multiboot header
section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

; Stack setup
section .bss
align 16
stack_bottom:
    resb 16384                      ; 16 KB stack
stack_top:

; Entry point
section .text
global _start
extern kernel_main

_start:
    ; Set up stack pointer
    mov esp, stack_top

    ; Push multiboot info and magic number
    push ebx                        ; Multiboot info structure pointer
    push eax                        ; Multiboot magic number

    ; Call kernel main function
    call kernel_main

    ; If kernel returns, halt the CPU
    cli
.hang:
    hlt
    jmp .hang

; GDT (Global Descriptor Table) - needed for protected mode
global gdt_flush
gdt_flush:
    mov eax, [esp + 4]              ; Get GDT pointer
    lgdt [eax]                      ; Load GDT
    mov ax, 0x10                    ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush                 ; Far jump to code segment
.flush:
    ret

; IDT (Interrupt Descriptor Table) loading
global idt_flush
idt_flush:
    mov eax, [esp + 4]              ; Get IDT pointer
    lidt [eax]                      ; Load IDT
    ret

; ISR (Interrupt Service Routine) stubs
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli
    push byte 0                     ; Push dummy error code
    push byte %1                    ; Push interrupt number
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli
    push byte %1                    ; Push interrupt number (error code already pushed)
    jmp isr_common_stub
%endmacro

; IRQ (Interrupt Request) stubs
%macro IRQ 2
global irq%1
irq%1:
    cli
    push byte 0
    push byte %2
    jmp irq_common_stub
%endmacro

; Exception ISRs
ISR_NOERRCODE 0                     ; Division by Zero
ISR_NOERRCODE 1                     ; Debug
ISR_NOERRCODE 2                     ; Non-Maskable Interrupt
ISR_NOERRCODE 3                     ; Breakpoint
ISR_NOERRCODE 4                     ; Overflow
ISR_NOERRCODE 5                     ; Bound Range Exceeded
ISR_NOERRCODE 6                     ; Invalid Opcode
ISR_NOERRCODE 7                     ; Device Not Available
ISR_ERRCODE   8                     ; Double Fault
ISR_NOERRCODE 9                     ; Coprocessor Segment Overrun
ISR_ERRCODE   10                    ; Invalid TSS
ISR_ERRCODE   11                    ; Segment Not Present
ISR_ERRCODE   12                    ; Stack-Segment Fault
ISR_ERRCODE   13                    ; General Protection Fault
ISR_ERRCODE   14                    ; Page Fault
ISR_NOERRCODE 15                    ; Reserved
ISR_NOERRCODE 16                    ; x87 FPU Error
ISR_ERRCODE   17                    ; Alignment Check
ISR_NOERRCODE 18                    ; Machine Check
ISR_NOERRCODE 19                    ; SIMD FP Exception
ISR_NOERRCODE 20                    ; Virtualization Exception
ISR_NOERRCODE 21                    ; Reserved
ISR_NOERRCODE 22                    ; Reserved
ISR_NOERRCODE 23                    ; Reserved
ISR_NOERRCODE 24                    ; Reserved
ISR_NOERRCODE 25                    ; Reserved
ISR_NOERRCODE 26                    ; Reserved
ISR_NOERRCODE 27                    ; Reserved
ISR_NOERRCODE 28                    ; Reserved
ISR_NOERRCODE 29                    ; Reserved
ISR_NOERRCODE 30                    ; Security Exception
ISR_NOERRCODE 31                    ; Reserved

; Hardware IRQs (remapped to 32-47)
IRQ 0, 32                           ; Timer
IRQ 1, 33                           ; Keyboard
IRQ 2, 34                           ; Cascade
IRQ 3, 35                           ; COM2
IRQ 4, 36                           ; COM1
IRQ 5, 37                           ; LPT2
IRQ 6, 38                           ; Floppy
IRQ 7, 39                           ; LPT1
IRQ 8, 40                           ; CMOS RTC
IRQ 9, 41                           ; Free
IRQ 10, 42                          ; Free
IRQ 11, 43                          ; Free
IRQ 12, 44                          ; PS/2 Mouse
IRQ 13, 45                          ; FPU
IRQ 14, 46                          ; Primary ATA
IRQ 15, 47                          ; Secondary ATA

extern isr_handler
extern irq_handler

; Common ISR stub
isr_common_stub:
    pusha                           ; Push all general-purpose registers
    mov ax, ds
    push eax                        ; Save data segment descriptor
    mov ax, 0x10                    ; Load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    call isr_handler
    pop eax                         ; Restore data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa                            ; Restore registers
    add esp, 8                      ; Clean up error code and ISR number
    sti
    iret                            ; Return from interrupt

; Common IRQ stub
irq_common_stub:
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    call irq_handler
    pop ebx
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx
    popa
    add esp, 8
    sti
    iret

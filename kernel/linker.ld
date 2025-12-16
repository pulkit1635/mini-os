/* MiniOS Linker Script */

ENTRY(_start)

SECTIONS
{
    /* Start at 1 MB - standard for kernels */
    . = 1M;

    /* Multiboot header must come first */
    .multiboot ALIGN(4K) :
    {
        *(.multiboot)
    }

    /* Text section (code) */
    .text ALIGN(4K) :
    {
        *(.text)
    }

    /* Read-only data */
    .rodata ALIGN(4K) :
    {
        *(.rodata)
        *(.rodata.*)
    }

    /* Initialized data */
    .data ALIGN(4K) :
    {
        *(.data)
    }

    /* Uninitialized data (BSS) */
    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss)
    }

    /* End of kernel */
    _kernel_end = .;
}

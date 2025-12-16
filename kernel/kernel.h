#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Multiboot magic number
#define MULTIBOOT_MAGIC 0x2BADB002

// Kernel version
#define KERNEL_VERSION_MAJOR 1
#define KERNEL_VERSION_MINOR 0
#define KERNEL_VERSION_PATCH 0

// Heap configuration (size is fixed, start is computed at runtime)
#define KERNEL_HEAP_SIZE  0x400000    // 4 MB

// Function declarations
void kernel_main(uint32_t magic, uint32_t* mboot_info);
void kernel_panic(const char* message);

#endif // KERNEL_H

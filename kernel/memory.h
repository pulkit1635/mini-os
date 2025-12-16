#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

// Memory block header
typedef struct memory_block {
    size_t size;
    int free;
    struct memory_block* next;
} memory_block_t;

// Initialize memory manager
void memory_init(void* heap_start, size_t heap_size);

// Allocate memory
void* kmalloc(size_t size);

// Allocate zeroed memory
void* kcalloc(size_t count, size_t size);

// Reallocate memory
void* krealloc(void* ptr, size_t size);

// Free memory
void kfree(void* ptr);

// Get free memory size
size_t memory_get_free(void);

// Get total heap size
size_t memory_get_total(void);

#endif // MEMORY_H

#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Memory block header with magic number for validation
#define MEMORY_BLOCK_MAGIC 0xDEADBEEF
#define MEMORY_BLOCK_FREE_MAGIC 0xFEEDFACE

typedef struct memory_block {
    uint32_t magic;           // Magic number for block validation
    size_t size;
    int free;
    struct memory_block* next;
    struct memory_block* prev; // Double-linked list for better coalescing
} memory_block_t;

// Memory statistics structure
typedef struct {
    size_t total_size;        // Total heap size
    size_t free_size;         // Current free memory
    size_t used_size;         // Current used memory
    size_t largest_free;      // Largest free block
    size_t block_count;       // Total number of blocks
    size_t free_block_count;  // Number of free blocks
    size_t alloc_count;       // Total allocations made
    size_t free_count;        // Total frees made
    size_t failed_allocs;     // Failed allocation attempts
} memory_stats_t;

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

// Get used memory size
size_t memory_get_used(void);

// Get largest free block
size_t memory_get_largest_free(void);

// Get detailed memory statistics
void memory_get_stats(memory_stats_t* stats);

// Validate memory integrity
bool memory_validate(void);

// Defragment memory (merge adjacent free blocks)
void memory_defragment(void);

// Check if pointer is valid heap pointer
bool memory_is_valid_ptr(void* ptr);

// Get block size for an allocation
size_t memory_get_block_size(void* ptr);

// Debug: dump memory map
void memory_dump(void);

#endif // MEMORY_H

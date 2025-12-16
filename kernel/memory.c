#include "memory.h"
#include "string.h"

// Heap management
static uint8_t* heap_start = NULL;
static uint8_t* heap_end = NULL;
static size_t heap_size = 0;
static memory_block_t* free_list = NULL;

// Memory statistics
static size_t total_allocs = 0;
static size_t total_frees = 0;
static size_t failed_allocs = 0;

// Alignment
#define ALIGN_SIZE 8
#define ALIGN(size) (((size) + (ALIGN_SIZE - 1)) & ~(ALIGN_SIZE - 1))
#define BLOCK_SIZE ALIGN(sizeof(memory_block_t))
#define MIN_BLOCK_SIZE 16

void memory_init(void* start, size_t size) {
    heap_start = (uint8_t*)start;
    heap_size = size;
    heap_end = heap_start + heap_size;
    
    // Initialize statistics
    total_allocs = 0;
    total_frees = 0;
    failed_allocs = 0;
    
    // Initialize the free list with one big block
    free_list = (memory_block_t*)heap_start;
    free_list->magic = MEMORY_BLOCK_FREE_MAGIC;
    free_list->size = heap_size - BLOCK_SIZE;
    free_list->free = 1;
    free_list->next = NULL;
    free_list->prev = NULL;
}

// Validate a block's magic number (with bounds checking)
static bool validate_block(memory_block_t* block) {
    if (block == NULL) return false;
    
    // Check if block is within heap bounds before dereferencing
    if ((uint8_t*)block < heap_start || 
        (uint8_t*)block >= heap_end ||
        (uint8_t*)block + sizeof(memory_block_t) > heap_end) {
        return false;
    }
    
    return (block->magic == MEMORY_BLOCK_MAGIC || 
            block->magic == MEMORY_BLOCK_FREE_MAGIC);
}

// Find a free block of at least the requested size (first fit)
static memory_block_t* find_free_block(size_t size) {
    memory_block_t* current = free_list;
    
    while (current != NULL) {
        if (!validate_block(current)) {
            // Memory corruption detected
            return NULL;
        }
        if (current->free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

// Split a block if it's too large
static void split_block(memory_block_t* block, size_t size) {
    if (block->size >= size + BLOCK_SIZE + MIN_BLOCK_SIZE) {
        memory_block_t* new_block = (memory_block_t*)((uint8_t*)block + BLOCK_SIZE + size);
        new_block->magic = MEMORY_BLOCK_FREE_MAGIC;
        new_block->size = block->size - size - BLOCK_SIZE;
        new_block->free = 1;
        new_block->next = block->next;
        new_block->prev = block;
        
        if (block->next) {
            block->next->prev = new_block;
        }
        
        block->size = size;
        block->next = new_block;
    }
}

void* kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    // Align the requested size
    size = ALIGN(size);
    if (size < MIN_BLOCK_SIZE) {
        size = MIN_BLOCK_SIZE;
    }
    
    memory_block_t* block = find_free_block(size);
    
    if (block == NULL) {
        failed_allocs++;
        return NULL;  // Out of memory
    }
    
    split_block(block, size);
    block->free = 0;
    block->magic = MEMORY_BLOCK_MAGIC;  // Mark as allocated
    
    total_allocs++;
    
    return (void*)((uint8_t*)block + BLOCK_SIZE);
}

void* kcalloc(size_t count, size_t size) {
    // Check for overflow
    if (count != 0 && size > (size_t)-1 / count) {
        failed_allocs++;
        return NULL;
    }
    
    size_t total = count * size;
    void* ptr = kmalloc(total);
    
    if (ptr != NULL) {
        memset(ptr, 0, total);
    }
    
    return ptr;
}

void* krealloc(void* ptr, size_t size) {
    if (ptr == NULL) {
        return kmalloc(size);
    }
    
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    // Validate the pointer
    if (!memory_is_valid_ptr(ptr)) {
        return NULL;
    }
    
    memory_block_t* block = (memory_block_t*)((uint8_t*)ptr - BLOCK_SIZE);
    
    if (!validate_block(block) || block->free) {
        return NULL;  // Invalid or already freed
    }
    
    size = ALIGN(size);
    
    if (block->size >= size) {
        // Current block is big enough, potentially split it
        if (block->size > size + BLOCK_SIZE + MIN_BLOCK_SIZE) {
            split_block(block, size);
        }
        return ptr;
    }
    
    // Try to expand into next block if it's free
    if (block->next && block->next->free && 
        block->size + BLOCK_SIZE + block->next->size >= size) {
        // Merge with next block
        block->size += BLOCK_SIZE + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
        // Potentially split if too large
        if (block->size > size + BLOCK_SIZE + MIN_BLOCK_SIZE) {
            split_block(block, size);
        }
        return ptr;
    }
    
    // Need to allocate a new block
    void* new_ptr = kmalloc(size);
    if (new_ptr != NULL) {
        memcpy(new_ptr, ptr, block->size);
        kfree(ptr);
    }
    
    return new_ptr;
}

// Merge adjacent free blocks
static void coalesce_blocks(void) {
    memory_block_t* current = free_list;
    
    while (current != NULL && current->next != NULL) {
        if (!validate_block(current)) {
            break;  // Memory corruption
        }
        
        if (current->free && current->next->free) {
            // Merge current with next
            current->size += BLOCK_SIZE + current->next->size;
            current->next = current->next->next;
            if (current->next) {
                current->next->prev = current;
            }
            // Don't advance, try to merge again
        } else {
            current = current->next;
        }
    }
}

void kfree(void* ptr) {
    if (ptr == NULL) {
        return;
    }
    
    // Validate the pointer is within heap bounds
    if (!memory_is_valid_ptr(ptr)) {
        return;  // Invalid pointer
    }
    
    memory_block_t* block = (memory_block_t*)((uint8_t*)ptr - BLOCK_SIZE);
    
    // Validate block magic
    if (!validate_block(block)) {
        return;  // Memory corruption or invalid pointer
    }
    
    if (block->free) {
        return;  // Double free protection
    }
    
    block->free = 1;
    block->magic = MEMORY_BLOCK_FREE_MAGIC;  // Mark as free
    total_frees++;
    
    coalesce_blocks();
}

size_t memory_get_free(void) {
    size_t free_size = 0;
    memory_block_t* current = free_list;
    
    while (current != NULL) {
        if (!validate_block(current)) break;
        if (current->free) {
            free_size += current->size;
        }
        current = current->next;
    }
    
    return free_size;
}

size_t memory_get_total(void) {
    return heap_size;
}

size_t memory_get_used(void) {
    return heap_size - memory_get_free();
}

size_t memory_get_largest_free(void) {
    size_t largest = 0;
    memory_block_t* current = free_list;
    
    while (current != NULL) {
        if (!validate_block(current)) break;
        if (current->free && current->size > largest) {
            largest = current->size;
        }
        current = current->next;
    }
    
    return largest;
}

void memory_get_stats(memory_stats_t* stats) {
    if (stats == NULL) return;
    
    stats->total_size = heap_size;
    stats->free_size = 0;
    stats->used_size = 0;
    stats->largest_free = 0;
    stats->block_count = 0;
    stats->free_block_count = 0;
    stats->alloc_count = total_allocs;
    stats->free_count = total_frees;
    stats->failed_allocs = failed_allocs;
    
    memory_block_t* current = free_list;
    
    while (current != NULL) {
        if (!validate_block(current)) break;
        
        stats->block_count++;
        
        if (current->free) {
            stats->free_size += current->size;
            stats->free_block_count++;
            if (current->size > stats->largest_free) {
                stats->largest_free = current->size;
            }
        } else {
            stats->used_size += current->size;
        }
        
        current = current->next;
    }
}

bool memory_validate(void) {
    memory_block_t* current = free_list;
    
    while (current != NULL) {
        // Check magic number
        if (!validate_block(current)) {
            return false;  // Corruption detected
        }
        
        // Check bounds
        if ((uint8_t*)current < heap_start || 
            (uint8_t*)current >= heap_end) {
            return false;
        }
        
        // Check size sanity
        if (current->size > heap_size) {
            return false;
        }
        
        // Check linked list consistency
        if (current->next && current->next->prev != current) {
            return false;
        }
        
        current = current->next;
    }
    
    return true;
}

void memory_defragment(void) {
    // First pass: coalesce adjacent free blocks
    coalesce_blocks();
    
    // Note: Full defragmentation (moving allocated blocks) is not
    // implemented as it would require updating all pointers, which
    // is not feasible without a garbage collector or memory handles.
}

bool memory_is_valid_ptr(void* ptr) {
    if (ptr == NULL) return false;
    
    uint8_t* p = (uint8_t*)ptr;
    
    // Check if pointer is within heap bounds
    if (p < heap_start + BLOCK_SIZE || p >= heap_end) {
        return false;
    }
    
    // Check if it points to a valid block
    memory_block_t* block = (memory_block_t*)(p - BLOCK_SIZE);
    return validate_block(block);
}

size_t memory_get_block_size(void* ptr) {
    if (!memory_is_valid_ptr(ptr)) {
        return 0;
    }
    
    memory_block_t* block = (memory_block_t*)((uint8_t*)ptr - BLOCK_SIZE);
    return block->size;
}

void memory_dump(void) {
    // This function would typically output to console
    // For now, it's a placeholder for debugging
}

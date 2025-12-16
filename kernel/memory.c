#include "memory.h"
#include "string.h"

// Heap management
static uint8_t* heap_start = NULL;
static size_t heap_size = 0;
static memory_block_t* free_list = NULL;

// Alignment
#define ALIGN_SIZE 8
#define ALIGN(size) (((size) + (ALIGN_SIZE - 1)) & ~(ALIGN_SIZE - 1))
#define BLOCK_SIZE ALIGN(sizeof(memory_block_t))

void memory_init(void* start, size_t size) {
    heap_start = (uint8_t*)start;
    heap_size = size;
    
    // Initialize the free list with one big block
    free_list = (memory_block_t*)heap_start;
    free_list->size = heap_size - BLOCK_SIZE;
    free_list->free = 1;
    free_list->next = NULL;
}

// Find a free block of at least the requested size
static memory_block_t* find_free_block(size_t size) {
    memory_block_t* current = free_list;
    
    while (current != NULL) {
        if (current->free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

// Split a block if it's too large
static void split_block(memory_block_t* block, size_t size) {
    if (block->size >= size + BLOCK_SIZE + ALIGN_SIZE) {
        memory_block_t* new_block = (memory_block_t*)((uint8_t*)block + BLOCK_SIZE + size);
        new_block->size = block->size - size - BLOCK_SIZE;
        new_block->free = 1;
        new_block->next = block->next;
        
        block->size = size;
        block->next = new_block;
    }
}

void* kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    size = ALIGN(size);
    
    memory_block_t* block = find_free_block(size);
    
    if (block == NULL) {
        return NULL;  // Out of memory
    }
    
    split_block(block, size);
    block->free = 0;
    
    return (void*)((uint8_t*)block + BLOCK_SIZE);
}

void* kcalloc(size_t count, size_t size) {
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
    
    memory_block_t* block = (memory_block_t*)((uint8_t*)ptr - BLOCK_SIZE);
    
    if (block->size >= size) {
        return ptr;  // Current block is big enough
    }
    
    void* new_ptr = kmalloc(size);
    if (new_ptr != NULL) {
        memcpy(new_ptr, ptr, block->size);
        kfree(ptr);
    }
    
    return new_ptr;
}

// Merge adjacent free blocks
static void merge_free_blocks(void) {
    memory_block_t* current = free_list;
    
    while (current != NULL && current->next != NULL) {
        if (current->free && current->next->free) {
            current->size += BLOCK_SIZE + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

void kfree(void* ptr) {
    if (ptr == NULL) {
        return;
    }
    
    memory_block_t* block = (memory_block_t*)((uint8_t*)ptr - BLOCK_SIZE);
    block->free = 1;
    
    merge_free_blocks();
}

size_t memory_get_free(void) {
    size_t free_size = 0;
    memory_block_t* current = free_list;
    
    while (current != NULL) {
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

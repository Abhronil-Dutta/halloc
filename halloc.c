#include "halloc.h"


static Header* heap_start = NULL;
static void* heap_end = NULL;

Header* find_free_block(size_t size){
    Header* current = heap_start;
    while (current != NULL){
        if (current->free == 1 && current->size >= size){
            return current;
        }
        current = (Header*)current->next;
    }
    return NULL;
}

Header* expand_heap(size_t size) {
    size_t needed = size + sizeof(Header);
    void* new_space = sbrk(needed);
    if (new_space == (void*)-1){
        return NULL;
    }

    Header* last = heap_start;
    while (last->next != NULL) {
        last = (Header*)last->next;
    }

    Header* new_block = (Header*)new_space;
    new_block->free = 1;
    new_block->size = needed - sizeof(Header);
    new_block->next = NULL;
    new_block->prev = last;
    
    last->next = new_block;
    heap_end = (char*)new_space + needed;
    
    return new_block;
}

void allocate_block(Header* block, size_t size){
    block->free = 0;

    size_t remaining = block->size - size;

    if (remaining > sizeof(Header)) {
        Header* new_block = (Header*)((char*)block + sizeof(Header) + size);
        
        new_block->free = 1;
        new_block->size = remaining - sizeof(Header);
        new_block->next = block->next;
        new_block->prev = (void*)block;

        if (block->next != NULL) {
            ((Header*)block->next)->prev = (void*)new_block;
        }
        block->next = (void*)new_block;
        block->size = size;
    }
}

void* halloc(size_t size){
    // CASE 1: First call innitial loop
    if (heap_start == NULL){
        void *innitial_heap = sbrk(1024);
        if (innitial_heap == (void*)-1){
            return NULL; // sbrk failed
        }

        heap_start = (Header*)innitial_heap;
        heap_end = (char*)innitial_heap + 1024;

        heap_start->free = 1;
        heap_start->size = 1024 - sizeof(Header);
        heap_start->next = NULL;
        heap_start->prev = NULL;
    }
    Header* block = find_free_block(size);
    if (block == NULL){
        block = expand_heap(size);
        if (block == NULL) return NULL; // sbrk failed
    }

    allocate_block(block, size);

    return (void*)((char*)block + sizeof(Header));
}



int hfree(void* addr){
    if (addr == NULL) return 0;

    Header* block = (Header*)addr - 1;

    if (block->free == 1){
        return 0; // block is already free
    }

    block->free = 1;

    if (block->next != NULL) {
        Header* next = (Header*)block->next;
        if (next->free == 1) {
            block->size += sizeof(Header) + next->size;
            block->next = next->next;
            if (next->next != NULL) {
                ((Header*)next->next)->prev = (void*)block;
            }
        }
    }

    if (block->prev != NULL) {
        Header* prev = (Header*)block->prev;
        if (prev->free == 1) {
            prev->size += sizeof(Header) + block->size;
            prev->next = block->next;
            if (block->next != NULL) {
                ((Header*)block->next)->prev = (void*)prev;
            }
        }
    }

    return 1;
}



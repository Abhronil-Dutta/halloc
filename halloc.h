#ifndef HALLOC_H
#define HALLOC_H

#include <unistd.h>
#include <stdio.h>

typedef struct {
    int free;     // 0 = not free; 1 = free
    size_t size;  // size of the usable block
    void* next;   // pointer to the next block
    void* prev;   // pointer to the previous block
} Header;  // 24 bytes

static Header* heap_start = NULL;
static void* heap_end = NULL;

void* halloc(size_t size);
int hfree(void* addr);


#endif


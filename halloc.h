#ifndef HALLOC_H
#define HALLOC_H

#include <unistd.h>
#include <stdio.h>

void* halloc(size_t size);
int hfree(void* addr);

typedef struct {
    int free;     // 0 = not free; 1 = free
    size_t size;  // size of the usable block
    void* next;   // pointer to the next block
} Header;  // 24 bytes

#endif


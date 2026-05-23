# halloc - Custom Memory Allocator

A simple heap allocator written in C that mimics malloc/free behavior.

## What it does

`halloc` allocates memory from the heap and `hfree` releases it back. It's built on a doubly-linked list of memory blocks, with automatic coalescing to prevent fragmentation.

## How it works

**First allocation:**
- Calls `sbrk(1024)` to grab 1KB from the OS
- Creates a single free block and sets up the linked list

**Normal allocation:**
- Walks the linked list looking for a free block big enough
- If found, marks it as used and splits off any extra space as a new free block
- If not found, calls `sbrk` again to expand the heap

**Freeing:**
- Marks a block as free
- Checks if adjacent blocks (next and previous) are also free
- If they are, merges them together to reduce fragmentation

## The header

Every allocated block has a 32-byte header before the actual memory:

```c
typedef struct {
    int free;      // 0 = allocated, 1 = free
    size_t size;   // usable size of the block
    void* next;    // pointer to next block
    void* prev;    // pointer to previous block
} Header;
```

When you call `halloc(100)`, you get back a pointer to the memory *after* the header. Internally, `hfree` finds the header by doing pointer arithmetic: `Header* block = (Header*)ptr - 1`.

## Usage

```c
#include "halloc.h"

int main() {
    void* ptr = halloc(256);
    if (ptr == NULL) {
        printf("allocation failed\n");
        return 1;
    }
    
    // use ptr...
    
    int result = hfree(ptr);
    if (result == 0) {
        printf("free failed\n");
        return 1;
    }
    
    return 0;
}
```

Compile with:
```bash
gcc -std=gnu11 -Wall -Wextra halloc.c your_code.c -o program
```

(Use `-std=gnu11` or add `-D_DEFAULT_SOURCE` to expose `sbrk`)

## Edge cases handled

- Double free returns 0 (fails gracefully)
- Free on NULL returns 0
- Allocating 0 bytes works (creates a 0-sized block)
- Consecutive free blocks automatically merge
- Heap expands automatically with `sbrk` when needed

## Limitations

- Single-threaded only (no locking)
- No bounds checking or corruption detection
- Header overhead (32 bytes per allocation) is significant for tiny allocations
- No `realloc` support yet
- First-fit allocation strategy (not optimized for fragmentation)

## Testing

Run the test suite:
```bash
make run
```

It covers basic allocation, freeing, fragmentation, coalescing, and heap expansion.

---

Built for learning. Not production-ready.
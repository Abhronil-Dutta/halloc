#include "halloc.h"

void* halloc(size_t size){
    printf("%p\n", sbrk(0));
    void* start = sbrk((int)size + sizeof(Header));
    printf("%p\n", sbrk(0));

    Header* header = (Header*)start;
    header->free = 0;
    header->size = size;
    header->next = NULL;

    void* user_ptr = (void*)((char*)start + sizeof(Header));
    return user_ptr;
}

int hfree(void* addr){
    Header* header = (Header*)((char*)addr - sizeof(Header));
    header->free = 1;
    return 0;
}
int main(){
    void* ptr1 = halloc(100);
    printf("User got pointer: %p\n", ptr1);
    
    char* buf = (char*)ptr1;
    buf[0] = 'A';
    buf[1] = 'B';
    printf("buf[0] = %c\n", buf[0]);
    hfree(ptr1);
    return 0;

}


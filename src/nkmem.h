#ifndef NINKASI_MEM_H
#define NINKASI_MEM_H

struct NKMemoryHeader
{
    uint32_t size;
    struct VM *vm;

    struct NKMemoryHeader *nextAllocation;
    struct NKMemoryHeader **prevAllocationPtr;
};

void *nkMalloc(struct VM *vm, uint32_t size);
void nkFree(struct VM *vm, void *data);
void *nkRealloc(struct VM *vm, void *data, uint32_t size);
char *nkStrdup(struct VM *vm, const char *str);

#endif

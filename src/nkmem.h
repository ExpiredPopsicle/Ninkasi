#ifndef NINKASI_MEM_H
#define NINKASI_MEM_H

struct NKMemoryHeader
{
    uint32_t size;
    struct VM *vm;

    struct NKMemoryHeader *nextAllocation;
    struct NKMemoryHeader **prevAllocationPtr;
};

void *nkiMalloc(struct VM *vm, uint32_t size);
void nkiFree(struct VM *vm, void *data);
void *nkiRealloc(struct VM *vm, void *data, uint32_t size);
char *nkStrdup(struct VM *vm, const char *str);

#endif // NINKASI_MEM_H

#ifndef NINKASI_MEM_H
#define NINKASI_MEM_H

struct NKMemoryHeader
{
    uint32_t size;
    struct NKVM *vm;

    struct NKMemoryHeader *nextAllocation;
    struct NKMemoryHeader **prevAllocationPtr;
};

void *nkiMalloc(struct NKVM *vm, uint32_t size);
void nkiFree(struct NKVM *vm, void *data);
void *nkiRealloc(struct NKVM *vm, void *data, uint32_t size);
char *nkiStrdup(struct NKVM *vm, const char *str);

#endif // NINKASI_MEM_H

#include "common.h"

void *nkMalloc(struct VM *vm, uint32_t size)
{
    // if(rand() % 128 == 0) return NULL;

    if(size != 0) {

        struct NKMemoryHeader *header = NULL;

        // Check size against memory limit.
        uint32_t newChunkSize = size + sizeof(struct NKMemoryHeader);
        if(vm->currentMemoryUsage > vm->limits.maxAllocatedMemory ||
            newChunkSize > vm->limits.maxAllocatedMemory - vm->currentMemoryUsage)
        {
            // VM allocation failure (hit user-set limit).
            errorStateSetAllocationFailFlag(&vm->errorState);
            NK_CATASTROPHE();
            return NULL;
        }

        header = malloc(newChunkSize);

        if(header) {

            header->size = size;
            header->vm = vm;
            vm->currentMemoryUsage += size + sizeof(struct NKMemoryHeader);

            if(vm->peakMemoryUsage < vm->currentMemoryUsage) {
                vm->peakMemoryUsage = vm->currentMemoryUsage;
            }

            // Add us to the allocation list.
            header->prevAllocationPtr = &vm->allocations;
            header->nextAllocation = vm->allocations;
            if(vm->allocations) {
                vm->allocations->prevAllocationPtr =
                    &header->nextAllocation;
            }
            vm->allocations = header;

            return header + 1;

        } else {

            // System allocation failure (possible address space
            // exhaustion).
            errorStateSetAllocationFailFlag(&vm->errorState);
            NK_CATASTROPHE();
        }
    }

    return NULL;
}

void nkFree(struct VM *vm, void *data)
{
    if(data) {
        struct NKMemoryHeader *header = (struct NKMemoryHeader*)data - 1;
        vm->currentMemoryUsage -= header->size + sizeof(struct NKMemoryHeader);

        // Snip us out of the allocations list.
        if(header->nextAllocation) {
            header->nextAllocation->prevAllocationPtr =
                header->prevAllocationPtr;
        }
        *header->prevAllocationPtr = header->nextAllocation;

        free(header);
    }
}

void *nkRealloc(struct VM *vm, void *data, uint32_t size)
{
    if(!data) {
        return nkMalloc(vm, size);
    } else {
        struct NKMemoryHeader *header = (struct NKMemoryHeader*)data - 1;
        uint32_t copySize = size < header->size ? size : header->size;
        void *newData = nkMalloc(vm, size);
        if(newData) {
            memcpy(newData, data, copySize);
        }
        nkFree(vm, data);
        return newData;
    }
    return NULL;
}

char *nkStrdup(struct VM *vm, const char *str)
{
    if(str) {
        uint32_t len = strlen(str) + 1;
        if(len) {
            char *copyData = nkMalloc(vm, len);
            if(copyData) {
                strcpy(copyData, str);
                return copyData;
            }
        }
    }
    return NULL;
}


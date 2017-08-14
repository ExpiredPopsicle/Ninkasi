#include "nkcommon.h"

// static nkuint32_t killCounter = 2;

void *nkiMalloc(struct NKVM *vm, nkuint32_t size)
{
    // if(rand() % 2048 == 0) {
    //     nkiErrorStateSetAllocationFailFlag(vm);
    //     NK_CATASTROPHE();
    //     return NULL;
    // }

    if(size != 0) {

        struct NKMemoryHeader *header = NULL;

        // Check size against memory limit.
        nkuint32_t newChunkSize = size + sizeof(struct NKMemoryHeader);
        if(vm->currentMemoryUsage > vm->limits.maxAllocatedMemory ||
            newChunkSize > vm->limits.maxAllocatedMemory - vm->currentMemoryUsage)
        {
            // VM allocation failure (hit user-set limit).
            nkiErrorStateSetAllocationFailFlag(vm);
            NK_CATASTROPHE();
            assert(0);
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
            nkiErrorStateSetAllocationFailFlag(vm);
            NK_CATASTROPHE();
            assert(0);
            return NULL;
        }
    }

    // Zero-size allocation.
    return NULL;
}

void nkiFree(struct NKVM *vm, void *data)
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

void *nkiRealloc(struct NKVM *vm, void *data, nkuint32_t size)
{
    if(!data) {
        return nkiMalloc(vm, size);
    } else {
        struct NKMemoryHeader *header = (struct NKMemoryHeader*)data - 1;
        nkuint32_t copySize = size < header->size ? size : header->size;
        void *newData = nkiMalloc(vm, size);
        if(newData) {
            memcpy(newData, data, copySize);
        }
        nkiFree(vm, data);
        return newData;
    }
    return NULL;
}

char *nkiStrdup(struct NKVM *vm, const char *str)
{
    if(str) {
        nkuint32_t len = strlen(str) + 1;
        if(len) {
            char *copyData = nkiMalloc(vm, len);
            if(copyData) {
                strcpy(copyData, str);
                return copyData;
            }
        }
    }
    return NULL;
}


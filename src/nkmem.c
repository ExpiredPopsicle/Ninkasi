// ----------------------------------------------------------------------
//
//        ▐ ▄ ▪   ▐ ▄ ▄ •▄  ▄▄▄· .▄▄ · ▪
//       •█▌▐███ •█▌▐██▌▄▌▪▐█ ▀█ ▐█ ▀. ██
//       ▐█▐▐▌▐█·▐█▐▐▌▐▀▀▄·▄█▀▀█ ▄▀▀▀█▄▐█·
//       ██▐█▌▐█▌██▐█▌▐█.█▌▐█ ▪▐▌▐█▄▪▐█▐█▌
//       ▀▀ █▪▀▀▀▀▀ █▪·▀  ▀ ▀  ▀  ▀▀▀▀ ▀▀▀
//
// ----------------------------------------------------------------------
//
//   Ninkasi 0.01
//
//   By Cliff "ExpiredPopsicle" Jolly
//     https://expiredpopsicle.com
//     https://intoxicoding.com
//     expiredpopsicle@gmail.com
//
// ----------------------------------------------------------------------
//
//   Copyright (c) 2017 Clifford Jolly
//
//   Permission is hereby granted, free of charge, to any person
//   obtaining a copy of this software and associated documentation files
//   (the "Software"), to deal in the Software without restriction,
//   including without limitation the rights to use, copy, modify, merge,
//   publish, distribute, sublicense, and/or sell copies of the Software,
//   and to permit persons to whom the Software is furnished to do so,
//   subject to the following conditions:
//
//   The above copyright notice and this permission notice shall be
//   included in all copies or substantial portions of the Software.
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
//   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
//   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//   SOFTWARE.
//
// -------------------------- END HEADER -------------------------------------

#include "nkcommon.h"

void *nkiMalloc(struct NKVM *vm, nkuint32_t size)
{
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

        header = vm->mallocReplacement(
            newChunkSize,
            vm->mallocAndFreeReplacementUserData);

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

        vm->freeReplacement(header, vm->mallocAndFreeReplacementUserData);
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


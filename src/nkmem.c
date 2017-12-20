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

#if __linux__
#include <malloc.h>
#include <execinfo.h>
#include <signal.h>
#endif

#if NK_EXTRA_FANCY_LEAK_TRACKING_LINUX
// Note: Uses malloc directly!
static void nkiMemAppendHeapString(char **base, const char *appender)
{
    char *s = malloc(strlen(*base) + strlen(appender) + 1);
    s[0] = 0;
    strcat(s, *base);
    strcat(s, appender);
    free(*base);
    *base = s;
}
#endif

// FIXME: Remove these!
nkuint32_t nkiMemFailRate = 0;
nkuint32_t nkiNumAllocs = 0;

nkuint32_t nkiMemGetAllocCount(struct NKVM *vm)
{
    nkuint32_t i = 0;
    struct NKMemoryHeader *header = vm->allocations;
    while(header) {
        i++;
        header = header->nextAllocation;
    }
    return i;
}

void *nkiMalloc_real(
    const char *filename, const char *function,
    int lineNumber, struct NKVM *vm, nkuint32_t size)
{
    // FIXME: Remove this!
    if(nkiMemFailRate) {
        nkiNumAllocs++;
        if(nkiNumAllocs >= nkiMemFailRate) {
            nkiNumAllocs = 0;
            NK_CATASTROPHE();
            assert(0);
            return NULL;
        }
    }

    // This MUST be set if we ever have even a chance of reaching this
    // function!
    assert(vm->catastrophicFailureJmpBuf);

    // Thanks, AFL! Allocations large enough to overflow 32-bit uints
    // are bad.
    if(size > ~(nkuint32_t)0 - sizeof(struct NKMemoryHeader)) {
        nkiErrorStateSetAllocationFailFlag(vm);
        NK_CATASTROPHE();
        assert(0);
        return NULL;
    }

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

        // FIXME: Remove this.
        // printf("MALLOC: %p " NK_PRINTF_UINT32 "\n", header+1, nkiMemGetAllocCount(vm));

        if(header) {

            // FIXME: Remove this.
            vm->allocationCount++;

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

#if NK_EXTRA_FANCY_LEAK_TRACKING_LINUX
            {
                const int stackLength = 256;
                void *stackArray[stackLength];
                char **symbols;
                size_t size;
                char *stackText = malloc(1);
                char tmp[256];

                stackText[0] = 0;

                size = backtrace(stackArray, stackLength);
                symbols = backtrace_symbols(stackArray, size);

                snprintf(tmp, sizeof(tmp) - 1, "Location: %s:%s:%d\n", filename, function, lineNumber);
                nkiMemAppendHeapString(&stackText, tmp);

                for(unsigned int i = 0; i < size && symbols; i++) {
                    nkiMemAppendHeapString(&stackText, symbols[i]);
                    nkiMemAppendHeapString(&stackText, "\n");
                }

                header->stackTrace = stackText;
                free(symbols);
            }
#endif
            // FIXME: Remove this.
            assert(vm->allocationCount == nkiMemGetAllocCount(vm));

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

    // FIXME: Remove this.
    assert(vm->allocationCount == nkiMemGetAllocCount(vm));

    // Zero-size allocation.
    return NULL;
}

void nkiFree(struct NKVM *vm, void *data)
{
    // // FIXME: Remove this.
    // if(data) {
    //     printf("FREE: %p " NK_PRINTF_UINT32 "\n", data, nkiMemGetAllocCount(vm));
    // }

    if(data) {
        struct NKMemoryHeader *header = (struct NKMemoryHeader*)data - 1;
        vm->currentMemoryUsage -= header->size + sizeof(struct NKMemoryHeader);

        // FIXME: Remove this.
        vm->allocationCount--;

        // Snip us out of the allocations list.
        if(header->nextAllocation) {
            header->nextAllocation->prevAllocationPtr =
                header->prevAllocationPtr;
        }
        *header->prevAllocationPtr = header->nextAllocation;

#if NK_EXTRA_FANCY_LEAK_TRACKING_LINUX
        free(header->stackTrace);
#endif

        vm->freeReplacement(header, vm->mallocAndFreeReplacementUserData);
    }

    // FIXME: Remove this.
    assert(vm->allocationCount == nkiMemGetAllocCount(vm));
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

// Thanks AFL! There were so many bugs caused by integer overflows
// when we tried to allocate some number of objects that I made this
// function to wrap all of them safely.
void *nkiMallocArray(struct NKVM *vm, nkuint32_t size, nkuint32_t count)
{
    if(count >= ~(nkuint32_t)0 / size) {
        nkiErrorStateSetAllocationFailFlag(vm);
        NK_CATASTROPHE();
        assert(0);
        return NULL;
    }

    return nkiMalloc(vm, size * count);
}

// Thanks AFL! Looks like we needed one for realloc, too.
void *nkiReallocArray(struct NKVM *vm, void *data, nkuint32_t size, nkuint32_t count)
{
    if(count >= ~(nkuint32_t)0 / size) {
        nkiErrorStateSetAllocationFailFlag(vm);
        NK_CATASTROPHE();
        assert(0);
        return NULL;
    }

    return nkiRealloc(vm, data, size * count);
}

void nkiDumpLeakData(struct NKVM *vm)
{
    struct NKMemoryHeader *header = vm->allocations;
    while(header) {
        fprintf(stderr, "Leak detected! %p\n", (header+1));
#if NK_EXTRA_FANCY_LEAK_TRACKING_LINUX
        fprintf(stderr, "%s\n", header->stackTrace);
#endif
        header = header->nextAllocation;
    }
    assert(!vm->allocations);
}


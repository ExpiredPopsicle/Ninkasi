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

// ----------------------------------------------------------------------
// Memory (malloc/free) wrappers.
// ----------------------------------------------------------------------
//
// This is the memory system that lets us track memory usage inside
// the VM, and throw an error when the hosting-application-defined
// memory limit OR system memory limit has been exhausted.
//
// Calls to user-defined malloc/free functions are handled inside this
// system, too.

#ifndef NINKASI_MEM_H
#define NINKASI_MEM_H

#define NK_EXTRA_FANCY_LEAK_TRACKING_LINUX 1

struct NKMemoryHeader
{
    nkuint32_t size;
    struct NKVM *vm;

    struct NKMemoryHeader *nextAllocation;
    struct NKMemoryHeader **prevAllocationPtr;

#if NK_EXTRA_FANCY_LEAK_TRACKING_LINUX
    char *stackTrace;
#endif
};

#if NK_EXTRA_FANCY_LEAK_TRACKING_LINUX
#define nkiMalloc(vm, size) nkiMalloc_real(__FILE__, __FUNCTION__, __LINE__, vm, size)
#else
#define nkiMalloc(vm, size) nkiMalloc_real(NULL, NULL, 0, vm, size)
#endif

void *nkiMalloc_real(const char *filename, const char *function, int lineNumber, struct NKVM *vm, nkuint32_t size);

void *nkiMallocArray(struct NKVM *vm, nkuint32_t size, nkuint32_t count);
void nkiFree(struct NKVM *vm, void *data);
void *nkiRealloc(struct NKVM *vm, void *data, nkuint32_t size);
void *nkiReallocArray(struct NKVM *vm, void *data, nkuint32_t size, nkuint32_t count);
char *nkiStrdup(struct NKVM *vm, const char *str);

void nkiDumpLeakData(struct NKVM *vm);

#endif // NINKASI_MEM_H

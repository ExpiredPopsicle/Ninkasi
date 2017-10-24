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

#define DEBUG_SPAM 0

// static nkint32_t nkiDbgIndentLevel = 0;

int nkiDbgWriteLine(const char *fmt, ...)
{
  #if DEBUG_SPAM
    va_list args;
    int ret;
    va_start(args, fmt);
    {
        nkint32_t i;
        for(i = 0; i < nkiDbgIndentLevel; i++) {
            printf("  ");
        }
        printf("\033[2m");
        ret = vprintf(fmt, args);
        printf("\033[0m");
        printf("\n");
    }
    va_end(args);
    return ret;
  #else
    return 0;
  #endif
}

void nkiDbgPush_real(const char *func)
{
    // nkiDbgIndentLevel++;
}

void nkiDbgPop_real(const char *func)
{
    // assert(nkiDbgIndentLevel > 0);
    // nkiDbgIndentLevel--;
}

// ----------------------------------------------------------------------
// State dump stuff.

void nkiDbgDumpRaw(FILE *stream, void *data, nkuint32_t len)
{
    nkuint32_t i;
    const char *lookup = "0123456789abcdef";
    const char *ptr = (const char *)data;
    for(i = 0; i < len; i++) {
        fputc(lookup[(ptr[i] & 0xf0) >> 4], stream);
        fputc(lookup[(ptr[i] & 0xf)], stream);
    }
}

void nkiDbgDumpState(struct NKVM *vm, FILE *stream)
{
    nkuint32_t i;

    fprintf(stream, "Errors:\n");
    {
        struct NKError *err = vm->errorState.firstError;
        while(err) {
            fprintf(stream, "  %s\n", err->errorText);
            err = err->next;
        }
    }

    fprintf(stream, "Stack:\n");
    fprintf(stream, "  Capacity:  %u\n", vm->stack.capacity);
    fprintf(stream, "  Size:      %u\n", vm->stack.size);
    fprintf(stream, "  IndexMask: %u\n", vm->stack.indexMask);
    fprintf(stream, "  Elements:\n");
    for(i = 0; i < vm->stack.size; i++) {
        fprintf(stream, "    %4x: ", i);
        nkiDbgDumpRaw(stream, &vm->stack.values[i], sizeof(vm->stack.values[i]));
        fprintf(stream, "\n");
    }

    fprintf(stream, "Statics:\n");
    fprintf(stream, "  staticAddressMask: %u\n", vm->staticAddressMask);
    fprintf(stream, "  Elements:\n");
    for(i = 0; i <= vm->staticAddressMask; i++) {
        fprintf(stream, "    %4x: ", i);
        nkiDbgDumpRaw(stream, &vm->staticSpace[i], sizeof(vm->staticSpace[i]));
        fprintf(stream, "\n");
    }

    fprintf(stream, "Instructions:\n");
    fprintf(stream, "  instructionAddressMask: %u\n", vm->instructionAddressMask);
    fprintf(stream, "  instructionPointer:     %u\n", vm->instructionPointer);
    for(i = 0; i <= vm->instructionAddressMask; i++) {
        fprintf(stream, "    %4x: ", i);
        nkiDbgDumpRaw(stream, &vm->instructions[i], sizeof(vm->instructions[i]));
        fprintf(stream, "\n");
    }

    fprintf(stream, "String table:\n");
    fprintf(stream, "  capacity: %u\n", vm->stringTable.stringTableCapacity);
    fprintf(stream, "  holes:\n");
    {
        char *holeTracker = nkiMalloc(vm, vm->stringTable.stringTableCapacity);
        struct NKVMStringTableHole *hole = vm->stringTable.tableHoles;

        memset(holeTracker, 0, vm->stringTable.stringTableCapacity);

        while(hole) {
            assert(!holeTracker[hole->index]);
            holeTracker[hole->index] = 1;
            hole = hole->next;
        }

        for(i = 0; i < vm->stringTable.stringTableCapacity; i++) {
            if(holeTracker[i]) {
                fprintf(stream, "    %4x\n", i);
            }
        }

        nkiFree(vm, holeTracker);
    }
    {
        nkuint32_t *bucketTracker = nkiMalloc(vm, vm->stringTable.stringTableCapacity * 4);
        for(i = 0; i < nkiVmStringHashTableSize; i++) {
            struct NKVMString *str = vm->stringTable.stringsByHash[i];
            while(str) {
                bucketTracker[str->stringTableIndex] = i;
                str = str->nextInHashBucket;
            }
        }
        nkiFree(vm, bucketTracker);
    }
    fprintf(stream, "  strings:\n");
    for(i = 0; i < vm->stringTable.stringTableCapacity; i++) {
        if(vm->stringTable.stringTable[i]) {
            fprintf(stream, "    string %4x:\n", i);
            fprintf(stream, "      stringTableIndex: %u\n", vm->stringTable.stringTable[i]->stringTableIndex);
            fprintf(stream, "      dontGC:           %u\n", vm->stringTable.stringTable[i]->dontGC);
            fprintf(stream, "      hash:             %u\n", vm->stringTable.stringTable[i]->hash);
            fprintf(stream, "      data:             ");
            nkiDbgDumpRaw(stream, vm->stringTable.stringTable[i]->str, strlen(vm->stringTable.stringTable[i]->str));
            fprintf(stream, "\n");
        }
    }

    fprintf(stream, "objectTable:\n");
    fprintf(stream, "  objectTableCapacity: %u\n", vm->objectTable.objectTableCapacity);
    fprintf(stream, "  holes:\n");
    {
        char *holeTracker = nkiMalloc(vm, vm->objectTable.objectTableCapacity);
        struct NKVMObjectTableHole *hole = vm->objectTable.tableHoles;

        memset(holeTracker, 0, vm->objectTable.objectTableCapacity);

        while(hole) {
            assert(!holeTracker[hole->index]);
            holeTracker[hole->index] = 1;
            hole = hole->next;
        }

        for(i = 0; i < vm->objectTable.objectTableCapacity; i++) {
            if(holeTracker[i]) {
                fprintf(stream, "    %4x\n", i);
            }
        }

        nkiFree(vm, holeTracker);
    }

    // TODO: Finish object table.

    // TODO: GC stuff? I dunno if we should include that in the
    // serialized state, but we can. Also, we should dump it here
    // regardless of serialization.

    // TODO: Functions.

    // TODO: External functions.

    // TODO: Global variables.

    // TODO: Memory limits (even if not serialized).

    // TODO: Check allocations? Same number?

    // TODO: External types.

}


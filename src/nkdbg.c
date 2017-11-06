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
        fprintf(stream, "    %u: ", i);
        nkiDbgDumpRaw(stream, &vm->stack.values[i], sizeof(vm->stack.values[i]));
        fprintf(stream, "\n");
    }

    fprintf(stream, "Statics:\n");
    fprintf(stream, "  staticAddressMask: %u\n", vm->staticAddressMask);
    fprintf(stream, "  Elements:\n");
    for(i = 0; i <= vm->staticAddressMask; i++) {
        fprintf(stream, "    %u: ", i);
        nkiDbgDumpRaw(stream, &vm->staticSpace[i], sizeof(vm->staticSpace[i]));
        fprintf(stream, "\n");
    }

    fprintf(stream, "Instructions:\n");
    fprintf(stream, "  instructionAddressMask: %u\n", vm->instructionAddressMask);
    fprintf(stream, "  instructionPointer:     %u\n", vm->instructionPointer);
    for(i = 0; i <= vm->instructionAddressMask; i++) {
        fprintf(stream, "    %u: ", i);
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
                fprintf(stream, "    %u\n", i);
            }
        }

        nkiFree(vm, holeTracker);
    }
    {
        // Thanks AFL! The size and count were swapped, so the size
        // would sometimes be zero and cause a divide by zero in the
        // checking.
        nkuint32_t *bucketTracker = nkiMallocArray(vm, sizeof(nkuint32_t), vm->stringTable.stringTableCapacity);
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
            fprintf(stream, "    string %u:\n", i);
            fprintf(stream, "      stringTableIndex: %u\n", vm->stringTable.stringTable[i]->stringTableIndex);
            fprintf(stream, "      dontGC:           %u\n", vm->stringTable.stringTable[i]->dontGC);
            fprintf(stream, "      hash:             %u\n", vm->stringTable.stringTable[i]->hash);
            fprintf(stream, "      data:             ");
            fprintf(stream, "      lastGCPass:       %u\n", vm->stringTable.stringTable[i]->lastGCPass);
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
                fprintf(stream, "    %u\n", i);
            }
        }

        nkiFree(vm, holeTracker);
    }
    fprintf(stream, "  objects:\n");
    for(i = 0; i < vm->objectTable.objectTableCapacity; i++) {
        if(vm->objectTable.objectTable[i]) {
            struct NKVMObject *ob = vm->objectTable.objectTable[i];
            fprintf(stream, "    %u\n", i);
            fprintf(stream, "      index: %u\n", ob->objectTableIndex);
            fprintf(stream, "      lastGCPass: %u\n", ob->lastGCPass);
            fprintf(stream, "      size: %u\n", ob->size);
            fprintf(stream, "      external handles: %u\n", ob->externalHandleCount);
            fprintf(stream, "      hashbuckets:\n");
            {
                nkuint32_t n;
                for(n = 0; n < nkiVMObjectHashBucketCount; n++) {
                    // FIXME: Output values here in a deterministic
                    // order, instead of just however they showed up
                    // in the list.
                    struct NKVMObjectElement *el = ob->hashBuckets[n];
                    if(el) {
                        fprintf(stream, "        %u:\n", n);
                        while(el) {
                            fprintf(stream, "          key: ");
                            nkiDbgDumpRaw(stream, &el->key, sizeof(el->key));
                            fprintf(stream, "\n          value: ");
                            nkiDbgDumpRaw(stream, &el->value, sizeof(el->value));
                            fprintf(stream, "\n");
                            el = el->next;
                        }
                    }
                }
            }
            fprintf(stream, "      gcCallback: %u\n", ob->gcCallback.id);
            fprintf(stream, "      serializationCallback: %u\n", ob->serializationCallback.id);
            fprintf(stream, "      externalDataType: %u\n", ob->externalDataType.id);
        }
    }

    // Functions.
    fprintf(stream, "functions: %u\n", vm->functionCount);
    for(i = 0; i < vm->functionCount; i++) {
        fprintf(stream, "  %u:\n", i);
        fprintf(stream, "    argumentCount: %u\n", vm->functionTable[i].argumentCount);
        fprintf(stream, "    firstInstructionIndex: %u\n", vm->functionTable[i].firstInstructionIndex);
        fprintf(stream, "    externalFunctionId: %u\n", vm->functionTable[i].externalFunctionId.id);
    }

    // External functions.
    fprintf(stream, "external functions: %u\n", vm->externalFunctionCount);
    for(i = 0; i < vm->externalFunctionCount; i++) {
        fprintf(stream, "  %u:\n", i);
        fprintf(stream, "    name: %s\n", vm->externalFunctionTable[i].name);
        fprintf(stream, "    CFunctionCallback: %p\n", vm->externalFunctionTable[i].CFunctionCallback);
        fprintf(stream, "    internalFunctionId: %u\n", vm->externalFunctionTable[i].internalFunctionId.id);
    }

    // Global variables.
    fprintf(stream, "globals: %u\n", vm->globalVariableCount);
    for(i = 0; i < vm->globalVariableCount; i++) {
        fprintf(stream, "  %u:\n", i);
        fprintf(stream, "    name: %s\n", vm->globalVariables[i].name);
        fprintf(stream, "    staticPosition: %u\n", vm->globalVariables[i].staticPosition);
    }

    // External types.
    fprintf(stream, "externalTypeNames: %u\n", vm->externalTypeCount);
    for(i = 0; i < vm->externalTypeCount; i++) {
        fprintf(stream, "  %u:\n", i);
        fprintf(stream, "    name: %s\n", vm->externalTypeNames[i]);
    }

    // GC stuff? I dunno if we should include that in the serialized
    // state, but we can. Also, we should dump it here regardless of
    // serialization.
    fprintf(stream, "Gc stuff:\n");
    fprintf(stream, "  lastGCPass: %u\n", vm->lastGCPass);
    fprintf(stream, "  gcInterval: %u\n", vm->gcInterval);
    fprintf(stream, "  gcCountdown: %u\n", vm->gcCountdown);
    fprintf(stream, "  gcNewObjectInterval: %u\n", vm->gcNewObjectInterval);
    fprintf(stream, "  gcNewObjectCountdown: %u\n", vm->gcNewObjectCountdown);

    // TODO: Memory limits (even if not serialized).

    // Check allocations? Same number?
    fprintf(stream, "Current memory usage: %u\n", vm->currentMemoryUsage);
}

void nkiCheckStringTableHoles(struct NKVM *vm)
{
    char *holeTracker = malloc(vm->stringTable.stringTableCapacity);
    struct NKVMStringTableHole *hole = vm->stringTable.tableHoles;

    memset(holeTracker, 0, vm->stringTable.stringTableCapacity);

    // printf("HOLE LIST...\n");
    while(hole) {
        // printf("HOLE: %d\n", hole->index);
        assert(hole->index < vm->stringTable.stringTableCapacity);
        assert(!vm->stringTable.stringTable[hole->index]);
        assert(!holeTracker[hole->index]);
        holeTracker[hole->index] = 1;
        hole = hole->next;
    }

    free(holeTracker);

    {
        nkuint32_t n;
        for(n = 0; n < vm->stringTable.stringTableCapacity; n++) {
            if(vm->stringTable.stringTable[n]) {
                assert(vm->stringTable.stringTable[n]->stringTableIndex == n);
            }
        }

        for(n = 0; n < nkiVmStringHashTableSize; n++) {

            struct NKVMString *str = vm->stringTable.stringsByHash[n];

            while(str) {
                assert(vm->stringTable.stringTable[str->stringTableIndex] == str);
                str = str->nextInHashBucket;
            }
        }
    }


}

void nkiVmStringTableDump(struct NKVMStringTable *table)
{
    nkuint32_t i;
    printf("String table dump...\n");

    printf("  Hash table...\n");
    for(i = 0; i < nkiVmStringHashTableSize  ; i++) {
        struct NKVMString *str = table->stringsByHash[i];
        printf("    %.2x\n", i);
        while(str) {
            printf("      %s\n", str->str);
            str = str->nextInHashBucket;
        }
    }

    printf("  Main table...\n");
    for(i = 0; i < table->stringTableCapacity; i++) {
        struct NKVMString *str = table->stringTable[i];
        printf("    %.2x\n", i);
        printf("      %s\n", str ? str->str : "<null>");
    }
}

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

nkuint32_t nkiStringHash(const char *in)
{
    nkuint32_t len = strlen(in);
    nkuint32_t a = 1;
    nkuint32_t b = 0;
    nkuint32_t i;

    for(i = 0; i < len; i++) {
        a = (a + in[i]) % (nkuint32_t)65521;
        b = (b + a) % (nkuint32_t)65521;
    }

    return (b << 16) | a;
}

void nkiVmStringTableInit(struct NKVM *vm)
{
    memset(&vm->stringsByHash, 0, sizeof(vm->stringsByHash));
    nkiTableInit(vm, &vm->stringTable);
}

void nkiVmStringTableDestroy(struct NKVM *vm)
{
    struct NKVMTable *table = &vm->stringTable;

    // Clear out the main table.
    nkuint32_t i;
    if(table && table->stringTable) {
        for(i = 0; i < table->capacity; i++) {
            nkiFree(vm, table->stringTable[i]);
            table->stringTable[i] = NULL;
        }
    }

    nkiTableDestroy(vm, table);

    // Zero-out the hash table, just for safety.
    memset(vm->stringsByHash, 0, sizeof(vm->stringsByHash));
}

struct NKVMString *nkiVmStringTableGetEntryById(
    struct NKVMTable *table, nkuint32_t index)
{
    if(index >= table->capacity) {
        return NULL;
    }

    return table->stringTable[index];
}

const char *nkiVmStringTableGetStringById(
    struct NKVMTable *table,
    nkuint32_t index)
{
    struct NKVMString *vmstr = nkiVmStringTableGetEntryById(table, index);
    const char *ret = vmstr ? vmstr->str : NULL;
    return ret;
}

nkuint32_t nkiVmStringTableFindOrAddString(
    struct NKVM *vm,
    const char *str)
{
    struct NKVMTable *table = &vm->stringTable;
    nkuint32_t hash = nkiStringHash(str);

    // See if we have this string already.
    struct NKVMString *hashBucket =
        vm->stringsByHash[hash & (nkiVmStringHashTableSize - 1)];
    struct NKVMString *cur = hashBucket;

    while(cur) {
        if(!strcmp(cur->str, str)) {
            return cur->stringTableIndex;
        }
        cur = cur->nextInHashBucket;
    }

    // If we've reach this point, then we don't have the string yet,
    // so we'll go ahead and make a new entry.
    {
        nkuint32_t len = strlen(str);

        struct NKVMString *newString =
            nkiMalloc(vm, sizeof(struct NKVMString) + len + 1);

        nkuint32_t index = nkiTableAddEntry(vm, table, newString);

        if(index == NK_INVALID_VALUE) {
            nkiFree(vm, newString);
            return NK_INVALID_VALUE;
        }

        newString->stringTableIndex = index;
        newString->lastGCPass = 0;
        newString->dontGC = nkfalse;
        newString->hash = nkiStringHash(str);
        strcpy(newString->str, str);
        newString->nextInHashBucket = hashBucket;
        vm->stringsByHash[hash & (nkiVmStringHashTableSize - 1)] = newString;

        return newString->stringTableIndex;
    }
}

// VM teardown function. Does not create holes. Use only during VM
// destruction.
void nkiVmStringTableCleanAllStrings(
    struct NKVM *vm)
{
    struct NKVMTable *table = &vm->stringTable;
    nkuint32_t i;

    for(i = 0; i < nkiVmStringHashTableSize; i++) {

        struct NKVMString *str = vm->stringsByHash[i];

        while(str) {
            struct NKVMString *next = str->nextInHashBucket;
            nkuint32_t index = str->stringTableIndex;
            nkiFree(vm, str);
            str = next;
            table->stringTable[index] = NULL;
        }
    }
}

void nkiVmStringTableCleanOldStrings(
    struct NKVM *vm,
    nkuint32_t lastGCPass)
{
    struct NKVMTable *table = &vm->stringTable;
    nkuint32_t i;

    for(i = 0; i < nkiVmStringHashTableSize; i++) {

        struct NKVMString **lastPtr = &vm->stringsByHash[i];
        struct NKVMString *str = vm->stringsByHash[i];

        while(str) {

            while(str && (lastGCPass != str->lastGCPass && !str->dontGC)) {

                nkuint32_t index = str->stringTableIndex;

                if(index == NK_INVALID_VALUE) {
                    nkiAddError(vm, -1, "Bad string index in garbage collector.");
                    return;
                }

                nkiTableEraseEntry(vm, table, index);

                *lastPtr = str->nextInHashBucket;
                nkiFree(vm, str);
                str = *lastPtr;
            }

            if(str) {
                lastPtr = &str->nextInHashBucket;
                str = str->nextInHashBucket;
            }
        }
    }
}

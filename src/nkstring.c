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
//   By Kiri "ExpiredPopsicle" Jolly
//     https://expiredpopsicle.com
//     https://intoxicoding.com
//     expiredpopsicle@gmail.com
//
// ----------------------------------------------------------------------
//
//   Copyright (c) 2017 Kiri Jolly
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
    nkuint32_t len = nkiStrlen(in);
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
    nkiMemset(&vm->stringsByHash, 0, sizeof(vm->stringsByHash));
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
    nkiMemset(vm->stringsByHash, 0, sizeof(vm->stringsByHash));
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

    {
        // See if we have this string already.
        struct NKVMString *hashBucket =
            vm->stringsByHash[hash & (nkiVmStringHashTableSize - 1)];
        struct NKVMString *cur = hashBucket;

        while(cur) {
            if(!nkiStrcmp(cur->str, str)) {
                return cur->stringTableIndex;
            }
            cur = cur->nextInHashBucket;
        }

        // If we've reached this point, then we don't have the string
        // yet, so we'll go ahead and make a new entry.
        {
            nkuint32_t len = nkiStrlen(str);

            struct NKVMString *newString =
                (struct NKVMString *)nkiMalloc(
                    vm, sizeof(struct NKVMString) + len + 1);

            nkuint32_t index = nkiTableAddEntry(vm, table, newString);

            if(index == NK_INVALID_VALUE) {
                nkiFree(vm, newString);
                return NK_INVALID_VALUE;
            }

            newString->stringTableIndex = index;
            newString->lastGCPass = 0;
            newString->dontGC = nkfalse;
            newString->hash = nkiStringHash(str);
            nkiStrcpy(newString->str, str);
            newString->nextInHashBucket = hashBucket;
            vm->stringsByHash[hash & (nkiVmStringHashTableSize - 1)] = newString;

            return newString->stringTableIndex;
        }
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

nkuint32_t nkiStrlen(const char *str)
{
    nkuint32_t i = 0;

    while(str[i]) {

        // Bail out early if we reach the max possible string length.
        if(i == ~(nkuint32_t)0 - 1) {
            return i;
        }

        i++;
    }

    return i;
}

nkint32_t nkiStrcmp(const char *a, const char *b)
{
    nkuint32_t i = 0;

    while(a[i] && b[i]) {

        // They're equal up to as far as we can see.
        if(i == ~(nkuint32_t)0 - 1) {
            return 0;
        }

        if(a[i] > b[i]) return 1;
        if(a[i] < b[i]) return -1;
        i++;
    }

    if(a[i] > b[i]) return 1;
    if(a[i] < b[i]) return -1;
    return 0;
}

void nkiStrcat(char *dst, const char *src)
{
    nkuint32_t i = nkiStrlen(dst);
    nkuint32_t start = i;

    while(src[i - start]) {

        // Bail out early if we hit the length limit.
        if(i == ~(nkuint32_t)0) {
            dst[i] = 0;
            return;
        }

        dst[i] = src[i - start];
        i++;
    }

    dst[i] = 0;
}

void nkiStrcpy(char *dst, const char *src)
{
    nkiStrcpy_s(dst, src, ~(nkuint32_t)0);
}

void nkiStrcpy_s(char *dst, const char *src, nkuint32_t len)
{
    nkuint32_t i = 0;

    while(src[i]) {

        dst[i] = src[i];

        i++;

        // Bail out early if we hit the length limit.
        if(i == ~(nkuint32_t)0) {
            dst[i] = 0;
            return;
        }
        if(i >= len) {
            dst[i] = 0;
            return;
        }
    }

    dst[i] = 0;
}

void nkiMemset(void *ptr, nkuint32_t c, nkuint32_t len)
{
    nkuint32_t i;
    for(i = 0; i < len; i++) {
        ((char*)ptr)[i] = c;
    }
}

void nkiMemcpy(void *dst, const void *src, nkuint32_t len)
{
    nkuint32_t i;
    for(i = 0; i < len; i++) {
        ((char*)dst)[i] = ((const char*)src)[i];
    }
}

nkint32_t nkiMemcmp(const void *a, const void *b, nkuint32_t len)
{
    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;
    nkuint32_t i;
    for(i = 0; i < len; i++) {
        if(pa[i] > pb[i]) return 1;
        if(pa[i] < pb[i]) return -1;
        i++;
    }
    return 0;
}


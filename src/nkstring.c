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
    for(i = 0; i < table->capacity; i++) {
        nkiFree(vm, table->stringTable[i]);
        table->stringTable[i] = NULL;
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
    nkuint32_t len = 0;

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

    // Check our length.
    len = strlen(str);
    if(len > vm->limits.maxStringLength) {
        nkiAddError(
            vm, -1, "Reached string length limit.");
        return NK_INVALID_VALUE;
    }

    // If we've reach this point, then we don't have the string yet,
    // so we'll go ahead and make a new entry.
    {
        struct NKVMString *newString =
            nkiMalloc(vm, sizeof(struct NKVMString) + len + 1);
        nkuint32_t index = 0;

        if(table->tableHoles) {

            // We can use an existing gap.

            struct NKVMTableHole *hole = table->tableHoles;
            table->tableHoles = hole->next;
            index = hole->index;
            nkiFree(vm, hole);

            // TODO: Remove.
            nkiDbgWriteLine("Filled a string table hole at index %d", index);

        } else {

            // Looks like we have to re-size the string table.

            nkuint32_t oldCapacity = table->capacity;
            nkuint32_t newCapacity = oldCapacity << 1;
            nkuint32_t i;

            // If we're going to have to allocate more space in our table, we
            // need to check against our VM's string limit.
            if((newCapacity > vm->limits.maxStrings || !newCapacity)) {
                nkiAddError(
                    vm, -1, "Reached string table capacity limit.");
                return NK_INVALID_VALUE;
            }

            table->stringTable = nkiReallocArray(
                vm,
                table->stringTable,
                sizeof(struct NKVMString *), newCapacity);

            // Create hole objects for all our empty new space. Not
            // that we don't create one on the border between the old
            // and new space because that's where our new entry will
            // be going.
            for(i = oldCapacity + 1; i < newCapacity; i++) {
                struct NKVMTableHole *hole =
                    nkiMalloc(vm, sizeof(struct NKVMTableHole));
                hole->index = i;
                hole->next = table->tableHoles;
                table->tableHoles = hole;

                table->stringTable[i] = NULL;
            }

            table->capacity = newCapacity;
            index = oldCapacity;

            // TODO: Remove.
            nkiDbgWriteLine("Expanded string table to make room for index %d", index);
        }

        newString->stringTableIndex = index;
        newString->lastGCPass = 0;
        newString->dontGC = nkfalse;
        newString->hash = nkiStringHash(str);
        strcpy(newString->str, str);
        newString->nextInHashBucket = hashBucket;
        vm->stringsByHash[hash & (nkiVmStringHashTableSize - 1)] = newString;
        table->stringTable[index] = newString;

        return newString->stringTableIndex;
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

                *lastPtr = str->nextInHashBucket;
                table->stringTable[str->stringTableIndex] = NULL;
                nkiFree(vm, str);
                str = *lastPtr;

                // Create a table hole for our new gap.
                nkiTableCreateHole(vm, &vm->stringTable, index);
            }

            if(str) {
                lastPtr = &str->nextInHashBucket;
                str = str->nextInHashBucket;
            }
        }
    }
}

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
    struct NKVMStringTable *table = &vm->stringTable;

    memset(&table->stringsByHash, 0, sizeof(table->stringsByHash));
    table->tableHoles = NULL;

    // Create a table with a capacity of a single string.
    table->stringTable = nkiMalloc(vm, sizeof(struct NKVMString*));
    table->stringTableCapacity = 1;
    table->stringTable[0] = NULL;

    // Create a "hole" object indicating that we have space in the
    // table.
    table->tableHoles = nkiMalloc(vm, sizeof(struct NKVMStringTableHole));
    table->tableHoles->index = 0;
    table->tableHoles->next = NULL;
}

void nkiVmStringTableDestroy(struct NKVM *vm)
{
    struct NKVMStringTable *table = &vm->stringTable;

    // Clear out the main table.
    nkuint32_t i;
    for(i = 0; i < table->stringTableCapacity; i++) {
        nkiFree(vm, table->stringTable[i]);
        table->stringTable[i] = NULL;
    }
    nkiFree(vm, table->stringTable);
    table->stringTableCapacity = 0;

    // Clear out the holes list.
    {
        struct NKVMStringTableHole *th = table->tableHoles;
        while(th) {
            struct NKVMStringTableHole *next = th->next;
            nkiFree(vm, th);
            th = next;
        }
        table->tableHoles = NULL;
    }

    // Zero-out the hash table, just for safety.
    memset(table->stringsByHash, 0, sizeof(table->stringsByHash));
}

struct NKVMString *nkiVmStringTableGetEntryById(
    struct NKVMStringTable *table, nkuint32_t index)
{
    if(index >= table->stringTableCapacity) {
        return NULL;
    }

    return table->stringTable[index];
}

const char *nkiVmStringTableGetStringById(
    struct NKVMStringTable *table,
    nkuint32_t index)
{
    struct NKVMString *vmstr = nkiVmStringTableGetEntryById(table, index);
    return vmstr ? vmstr->str : NULL;
}

nkuint32_t nkiVmStringTableFindOrAddString(
    struct NKVM *vm,
    const char *str)
{
    struct NKVMStringTable *table = &vm->stringTable;
    nkuint32_t hash = nkiStringHash(str);
    nkuint32_t len = 0;

    // See if we have this string already.
    struct NKVMString *hashBucket =
        table->stringsByHash[hash & (nkiVmStringHashTableSize - 1)];
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

            struct NKVMStringTableHole *hole = table->tableHoles;
            table->tableHoles = hole->next;
            index = hole->index;
            nkiFree(vm, hole);

            // TODO: Remove.
            nkiDbgWriteLine("Filled a string table hole at index %d", index);

        } else {

            // Looks like we have to re-size the string table.

            nkuint32_t oldCapacity = table->stringTableCapacity;
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
                struct NKVMStringTableHole *hole =
                    nkiMalloc(vm, sizeof(struct NKVMStringTableHole));
                hole->index = i;
                hole->next = table->tableHoles;
                table->tableHoles = hole;

                table->stringTable[i] = NULL;
            }

            table->stringTableCapacity = newCapacity;
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
        table->stringsByHash[hash & (nkiVmStringHashTableSize - 1)] = newString;
        table->stringTable[index] = newString;

        return newString->stringTableIndex;
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

void nkiVmStringTableCleanOldStrings(
    struct NKVM *vm,
    nkuint32_t lastGCPass)
{
    struct NKVMStringTable *table = &vm->stringTable;
    nkuint32_t i;

    nkiDbgWriteLine("Purging unused strings...");
    nkiDbgPush();

    for(i = 0; i < nkiVmStringHashTableSize; i++) {

        struct NKVMString **lastPtr = &table->stringsByHash[i];
        struct NKVMString *str = table->stringsByHash[i];

        while(str) {

            while(str && (lastGCPass != str->lastGCPass && !str->dontGC)) {

                nkuint32_t index = str->stringTableIndex;
                struct NKVMStringTableHole *hole =
                    nkiMalloc(vm, sizeof(struct NKVMStringTableHole));

                // TODO: Remove this.
                nkiDbgWriteLine("Purging unused string: %s", str->str);

                *lastPtr = str->nextInHashBucket;
                table->stringTable[str->stringTableIndex] = NULL;
                nkiFree(vm, str);
                str = *lastPtr;

                // Create a table hole for our new gap.
                memset(hole, 0, sizeof(*hole));
                hole->index = index;
                hole->next = table->tableHoles;
                table->tableHoles = hole;
            }

            // FIXME: Remove this.
            if(str) {
                nkiDbgWriteLine("NOT purging string: %s", str->str);
            }

            if(str) {
                lastPtr = &str->nextInHashBucket;
                str = str->nextInHashBucket;
            }
        }
    }

    nkiDbgPop();
}

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

void nkiTableInit(struct NKVM *vm, struct NKVMTable *table)
{
    // Create a table of one empty entry.
    table->data = nkiMalloc(vm, sizeof(void*));
    table->capacity = 1;
    table->data[0] = NULL;

    // Create the hole object that goes with the empty space.
    table->tableHoles = nkiMalloc(vm, sizeof(struct NKVMTableHole));
    table->tableHoles->index = 0;
    table->tableHoles->next = NULL;
}

void nkiTableDestroy(struct NKVM *vm, struct NKVMTable *table)
{
    nkuint32_t i;

    // Maybe remove this?
    for(i = 0; i < table->capacity; i++) {
        assert(!table->data[i]);
    }

    nkiFree(vm, table->data);
    table->data = NULL;
    table->capacity = 0;

    // Clear out the holes list.
    {
        struct NKVMTableHole *th = table->tableHoles;
        while(th) {
            struct NKVMTableHole *next = th->next;
            nkiFree(vm, th);
            th = next;
        }
        table->tableHoles = NULL;
    }
}

void nkiTableCreateHole(struct NKVM *vm, struct NKVMTable *table, nkuint32_t holeIndex)
{
    struct NKVMTableHole *hole =
        nkiMalloc(vm, sizeof(struct NKVMTableHole));

    assert(holeIndex < table->capacity);
    assert(table->data[holeIndex] == NULL);

    hole->index = holeIndex;
    hole->next = table->tableHoles;
    table->tableHoles = hole;
}

void nkiTableEraseEntry(struct NKVM *vm, struct NKVMTable *table, nkuint32_t index)
{
    assert(index < table->capacity);
    assert(table->data[index]);
    table->data[index] = NULL;
    nkiTableCreateHole(vm, table, index);
}

void nkiTableResetHoles(struct NKVM *vm, struct NKVMTable *table)
{
    nkuint32_t i;

    // Free all object table holes.
    while(table->tableHoles) {
        struct NKVMTableHole *hole = table->tableHoles;
        table->tableHoles = hole->next;
        nkiFree(vm, hole);
    }

    // Recreate all object table holes.
    for(i = table->capacity - 1; i != NK_UINT_MAX; i--) {
        if(!table->data[i]) {
            nkiTableCreateHole(vm, table, i);
        }
    }
}

void nkiTableShrink(struct NKVM *vm, struct NKVMTable *table)
{
    nkuint32_t i;
    nkuint32_t highestObject = 0;
    nkuint32_t newCapacity = table->capacity;

    // Find out what the highest active index is.
    for(i = 0; i < table->capacity; i++) {
        if(table->objectTable[i]) {
            highestObject = i;
        }
    }

    // See how far we can reduce the object table while still
    // containing the highest object.
    while((newCapacity >> 1) > highestObject) {
        newCapacity >>= 1;
    }

    // We always need a minimum capacity.
    if(newCapacity < 1) {
        newCapacity = 1;
    }

    // Reallocate.
    if(newCapacity != table->capacity) {

        // FIXME: Remove debug spam.
        printf("SHRINK: Reducing table: " NK_PRINTF_UINT32 " to " NK_PRINTF_UINT32 "\n",
            table->capacity, newCapacity);

        table->objectTable = nkiReallocArray(
            vm, table->objectTable,
            sizeof(struct NKVMObject *), newCapacity);
        table->capacity = newCapacity;

    }

    // Deal with the fact that our holes list is completely wrong.
    nkiTableResetHoles(vm, table);
}

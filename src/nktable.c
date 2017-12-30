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

    // Tolerance for half-setup VMs.
    if(!table) {
        return;
    }

    // Maybe remove this?
    if(table->data) {
        for(i = 0; i < table->capacity; i++) {
            assert(!table->data[i]);
        }
        nkiFree(vm, table->data);
        table->data = NULL;
    }

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

    // Thanks AFL! The minimum capacity check was causing
    // uninitialized data to leak in here. (We'd realloc to a bigger
    // array and then not fill it with anything.)

    // Reallocate.
    if(newCapacity != table->capacity) {
        table->objectTable = nkiReallocArray(
            vm, table->objectTable,
            sizeof(struct NKVMObject *), newCapacity);
        table->capacity = newCapacity;
    }

    // Deal with the fact that our holes list is completely wrong.
    nkiTableResetHoles(vm, table);
}

nkuint32_t nkiTableAddEntry(struct NKVM *vm, struct NKVMTable *table, void *entryData)
{
    nkuint32_t index = 0;

    if(table->tableHoles) {

        // Holes exist. Use one of those.
        struct NKVMTableHole *hole = table->tableHoles;
        table->tableHoles = hole->next;
        index = hole->index;
        nkiFree(vm, hole);

    } else {

        // No holes exist. Expand the table.

        nkuint32_t oldCapacity = table->capacity;
        nkuint32_t newCapacity = oldCapacity << 1;
        nkuint32_t i;

        // Handle 32-bit address space exhaustion. Well, I suppose
        // it's more like 31-bit in this case, because we never reach
        // 0xffffffff, only 0x80000000, before overflowing to zero.
        if(!newCapacity) {
            nkiAddError(
                vm, -1, "Address space exhaustion when adding item to table.");
            return NK_INVALID_VALUE;
        }

        // Here's something really important to note. Because of the
        // allocation failure longjmp(), this assignment will *never*
        // complete in the case of realloc failure. In fact, it won't
        // even deallocate the old array. If this nkiReallocArray()
        // call fails, then the table, and recorded capacity will
        // remain unchanged.
        //
        // This is important, because it means that the object and
        // string table contents and ranges will remain sane in case
        // of catastrophic failure, allowing save VM and external data
        // cleanup.
        table->data = nkiReallocArray(
            vm,
            table->stringTable,
            sizeof(void *), newCapacity);

        // Set table capacity AFTER we successfully realloc, so the
        // allocation and ranges don't ever mismatch.
        table->capacity = newCapacity;
        index = oldCapacity;

        // Create hole objects for all our empty new space. Note that
        // we don't create one on the border between the old and new
        // space because that's where our new entry will be going. Add
        // holes back-to-front so we end up with holes roughly in
        // order, and will mostly allocate from the front of memory.
        //
        // Note: Our new object is going to go into the slot at
        // oldCapacity, but we're filling it with NULL here because
        // the hole creation could fail and leave us with a dangling
        // pointer.
        for(i = newCapacity - 1; i >= oldCapacity; i--) {
            table->data[i] = NULL;
        }

        // We do this as a separate pass because hole creation
        // allocates memory and is therefor susceptible to
        // out-of-memory failures. We want to make sure the new table
        // is fully zeroed-out so that we can still do cleanup
        // operations if we suffer a catastrophic failure.
        for(i = newCapacity - 1; i >= oldCapacity + 1; i--) {
            nkiTableCreateHole(vm, table, i);
        }
    }

    table->data[index] = entryData;

    return index;
}



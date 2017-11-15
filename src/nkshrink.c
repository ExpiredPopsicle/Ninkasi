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

void nkiVmMoveObject(struct NKVM *vm, nkuint32_t oldSlot, nkuint32_t newSlot)
{
    assert(vm->objectTable.capacity > newSlot);
    assert(vm->objectTable.capacity > oldSlot);
    assert(!vm->objectTable.objectTable[newSlot]);
    assert(vm->objectTable.objectTable[oldSlot]);

    vm->objectTable.objectTable[oldSlot]->objectTableIndex = newSlot;
    vm->objectTable.objectTable[newSlot] = vm->objectTable.objectTable[oldSlot];
    vm->objectTable.objectTable[oldSlot] = NULL;

    {
        nkuint32_t i;

        // Rewrite all stack references.
        for(i = 0; i < vm->stack.size; i++) {
            if(vm->stack.values[i].type == NK_VALUETYPE_OBJECTID) {
                if(vm->stack.values[i].objectId == oldSlot) {
                    vm->stack.values[i].objectId = newSlot;
                }
            }
        }

        // Rewrite all static references.
        for(i = 0; i <= vm->staticAddressMask; i++) {
            if(vm->staticSpace[i].type == NK_VALUETYPE_OBJECTID) {
                if(vm->staticSpace[i].objectId == oldSlot) {
                    vm->staticSpace[i].objectId = newSlot;
                }
            }
        }

        // Rewrite all object references in any keys or values.
        for(i = 0; i < vm->objectTable.capacity; i++) {
            struct NKVMObject *ob = vm->objectTable.objectTable[i];
            if(ob) {
                nkuint32_t k;
                for(k = 0; k < nkiVMObjectHashBucketCount; k++) {
                    struct NKVMObjectElement *el;
                    for(el = ob->hashBuckets[k]; el; el = el->next) {
                        if(el->key.type == NK_VALUETYPE_OBJECTID && el->key.objectId == oldSlot) {
                            el->key.objectId = newSlot;
                        }
                        if(el->value.type == NK_VALUETYPE_OBJECTID && el->value.objectId == oldSlot) {
                            el->value.objectId = newSlot;
                        }
                    }
                }
            }
        }
    }
}

void nkiVmMoveString(struct NKVM *vm, nkuint32_t oldSlot, nkuint32_t newSlot)
{
    nkuint32_t i;

    assert(vm->stringTable.capacity > newSlot);
    assert(vm->stringTable.capacity > oldSlot);
    assert(!vm->stringTable.stringTable[newSlot]);
    assert(vm->stringTable.stringTable[oldSlot]);

    // Move the string.
    vm->stringTable.stringTable[oldSlot]->stringTableIndex = newSlot;
    vm->stringTable.stringTable[newSlot] = vm->stringTable.stringTable[oldSlot];
    vm->stringTable.stringTable[oldSlot] = NULL;

    // Rewrite all stack references.
    for(i = 0; i < vm->stack.size; i++) {
        if(vm->stack.values[i].type == NK_VALUETYPE_STRING) {
            if(vm->stack.values[i].stringTableEntry == oldSlot) {
                vm->stack.values[i].stringTableEntry = newSlot;
            }
        }
    }

    // Rewrite all static references.
    for(i = 0; i <= vm->staticAddressMask; i++) {
        if(vm->staticSpace[i].type == NK_VALUETYPE_STRING) {
            if(vm->staticSpace[i].stringTableEntry == oldSlot) {
                vm->staticSpace[i].stringTableEntry = newSlot;
            }
        }
    }

    // Rewrite all object references in any keys or values.
    for(i = 0; i < vm->objectTable.capacity; i++) {
        struct NKVMObject *ob = vm->objectTable.objectTable[i];
        if(ob) {
            nkuint32_t k;
            for(k = 0; k < nkiVMObjectHashBucketCount; k++) {
                struct NKVMObjectElement *el;
                for(el = ob->hashBuckets[k]; el; el = el->next) {
                    if(el->key.type == NK_VALUETYPE_STRING && el->key.stringTableEntry == oldSlot) {
                        el->key.stringTableEntry = newSlot;
                    }
                    if(el->value.type == NK_VALUETYPE_STRING && el->value.stringTableEntry == oldSlot) {
                        el->value.stringTableEntry = newSlot;
                    }
                }
            }
        }
    }
}

void nkiVmRecreateObjectAndStringHoles(struct NKVM *vm)
{
    nkuint32_t i;

    // Free all object table holes.
    while(vm->objectTable.tableHoles) {
        struct NKVMTableHole *hole = vm->objectTable.tableHoles;
        vm->objectTable.tableHoles = hole->next;
        nkiFree(vm, hole);
    }

    // Recreate all object table holes.
    for(i = vm->objectTable.capacity - 1; i != NK_UINT_MAX; i--) {
        if(!vm->objectTable.objectTable[i]) {
            nkiVmObjectTableCreateHole(vm, i);
        }
    }

    // Free all string table holes.
    while(vm->stringTable.tableHoles) {
        struct NKVMTableHole *hole = vm->stringTable.tableHoles;
        vm->stringTable.tableHoles = hole->next;
        nkiFree(vm, hole);
    }

    // Recreate all string table holes.
    for(i = vm->stringTable.capacity - 1; i != NK_UINT_MAX; i--) {
        if(!vm->stringTable.stringTable[i]) {
            nkiVmStringTableCreateHole(vm, i);
        }
    }
}

void nkiVmShrink(struct NKVM *vm)
{
    nkuint32_t emptyHoleSearch = 0;
    nkuint32_t objectSearch = 0;

    while(emptyHoleSearch < vm->objectTable.capacity) {
        if(!vm->objectTable.objectTable[emptyHoleSearch]) {

            // Skip up to the empty hole so we only look at moving
            // something from after it into this slot.
            if(objectSearch < emptyHoleSearch) {
                objectSearch = emptyHoleSearch + 1;
            }

            // Find an object to move into this slot.
            while(objectSearch < vm->objectTable.capacity) {
                if(vm->objectTable.objectTable[objectSearch]) {
                    if(!vm->objectTable.objectTable[objectSearch]->externalHandleCount) {
                        // Found something for this hole!
                        nkiVmMoveObject(vm, objectSearch, emptyHoleSearch);
                        break;
                    }
                }
                objectSearch++;
            }
        }
        emptyHoleSearch++;
    }

    emptyHoleSearch = 0;
    objectSearch = 0;

    while(emptyHoleSearch < vm->stringTable.capacity) {
        if(!vm->stringTable.stringTable[emptyHoleSearch]) {

            // Skip up to the empty hole so we only look at moving
            // something from after it into this slot.
            if(objectSearch < emptyHoleSearch) {
                objectSearch = emptyHoleSearch + 1;
            }

            // Find a string to move into this slot.
            while(objectSearch < vm->stringTable.capacity) {
                if(vm->stringTable.stringTable[objectSearch]) {
                    if(!vm->stringTable.stringTable[objectSearch]->dontGC) {
                        // Found something for this hole!
                        nkiVmMoveString(vm, objectSearch, emptyHoleSearch);
                        break;
                    }
                }
                objectSearch++;
            }
        }
        emptyHoleSearch++;
    }

    {
        nkuint32_t i;
        nkuint32_t highestObject = 0;
        nkuint32_t newCapacity = vm->objectTable.capacity;

        // Find out what the highest active index is.
        for(i = 0; i < vm->objectTable.capacity; i++) {
            if(vm->objectTable.objectTable[i]) {
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
        if(newCapacity != vm->objectTable.capacity) {

            // FIXME: Remove debug spam.
            printf("SHRINK: Reducing object table: " NK_PRINTF_UINT32 " to " NK_PRINTF_UINT32 "\n",
                vm->objectTable.capacity, newCapacity);

            vm->objectTable.objectTable = nkiReallocArray(
                vm, vm->objectTable.objectTable,
                sizeof(struct NKVMObject *), newCapacity);
            vm->objectTable.capacity = newCapacity;

        }
    }

    {
        nkuint32_t i;
        nkuint32_t highestString = 0;
        nkuint32_t newCapacity = vm->stringTable.capacity;

        // Find out what the highest active index is.
        for(i = 0; i < vm->stringTable.capacity; i++) {
            if(vm->stringTable.stringTable[i]) {
                highestString = i;
            }
        }

        // See how far we can reduce the string table while still
        // containing the highest string.
        while((newCapacity >> 1) > highestString) {
            newCapacity >>= 1;
        }

        // We always need a minimum capacity.
        if(newCapacity < 1) {
            newCapacity = 1;
        }

        // Reallocate.
        if(newCapacity != vm->stringTable.capacity) {

            vm->stringTable.stringTable = nkiReallocArray(
                vm, vm->stringTable.stringTable,
                sizeof(struct NKVMString *), newCapacity);
            vm->stringTable.capacity = newCapacity;

        }

    }

    nkiVmRecreateObjectAndStringHoles(vm);

    // Reduce stack size.
    {
        nkuint32_t newStackCapacity = vm->stack.capacity;
        while((newStackCapacity >> 1) > vm->stack.size) {
            newStackCapacity >>= 1;
        }

        if(newStackCapacity < 1) {
            newStackCapacity = 1;
        }

        if(newStackCapacity != vm->stack.capacity) {

            vm->stack.values = nkiReallocArray(
                vm, vm->stack.values,
                sizeof(struct NKValue*),
                newStackCapacity);

            vm->stack.capacity = newStackCapacity;
        }
    }

}

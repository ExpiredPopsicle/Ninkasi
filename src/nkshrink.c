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

// FIXME (COROUTINES): Make this stuff support coroutines. Will
// probably have to iterate over every coroutine object and shrink its
// stack.

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
        for(i = 0; i < vm->currentExecutionContext->stack.size; i++) {
            if(vm->currentExecutionContext->stack.values[i].type == NK_VALUETYPE_OBJECTID) {
                if(vm->currentExecutionContext->stack.values[i].objectId == oldSlot) {
                    vm->currentExecutionContext->stack.values[i].objectId = newSlot;
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
    for(i = 0; i < vm->currentExecutionContext->stack.size; i++) {
        if(vm->currentExecutionContext->stack.values[i].type == NK_VALUETYPE_STRING) {
            if(vm->currentExecutionContext->stack.values[i].stringTableEntry == oldSlot) {
                vm->currentExecutionContext->stack.values[i].stringTableEntry = newSlot;
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

void nkiVmShrinkStack(
    struct NKVM *vm,
    struct NKVMStack *stack)
{
    nkuint32_t newStackCapacity = stack->capacity;
    while((newStackCapacity >> 1) > stack->size) {
        newStackCapacity >>= 1;
    }

    if(newStackCapacity < 1) {
        newStackCapacity = 1;
    }

    if(newStackCapacity != stack->capacity) {
        stack->values =
            (struct NKValue *)nkiReallocArray(
                vm, stack->values,
                sizeof(struct NKValue),
                newStackCapacity);

        stack->capacity = newStackCapacity;

        // Thanks AFL!
        stack->indexMask = stack->capacity - 1;
    }
}

void nkiVmShrink(struct NKVM *vm)
{
    nkuint32_t emptyHoleSearch = 0;
    nkuint32_t objectSearch = 0;

    // VMs with errors may be in an unpredictable or inconsistent
    // state. Bail out here, just in case.
    if(nkiVmHasErrors(vm)) {
        return;
    }

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

    nkiTableShrink(vm, &vm->objectTable);
    nkiTableShrink(vm, &vm->stringTable);

    // Shrink the stack capacity in the root context.
    nkiVmShrinkStack(vm, &vm->rootExecutionContext.stack);

    // Iterate through all objects, find the coroutines, and shrink
    // their stacks too.
    for(nkuint32_t i = 0; i < vm->objectTable.capacity; i++) {
        struct NKVMObject *ob = vm->objectTable.objectTable[i];
        if(ob && ob->externalDataType.id ==
            vm->internalObjectTypes.coroutine.id)
        {
            struct NKVMExecutionContext *context =
                (struct NKVMExecutionContext *)ob->externalData;
            nkiVmShrinkStack(vm, &context->stack);
        }
    }
}

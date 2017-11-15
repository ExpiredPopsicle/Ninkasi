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

void nkiVmObjectTableInit(struct NKVM *vm)
{
    struct NKVMTable *table = &vm->objectTable;

    table->tableHoles = NULL;

    // Create a table of one empty entry.
    table->objectTable = nkiMalloc(vm, sizeof(struct NKVMObject*));
    table->capacity = 1;
    table->objectTable[0] = NULL;

    // Create the hole object that goes with the empty space.
    table->tableHoles = nkiMalloc(vm, sizeof(struct NKVMTableHole));
    table->tableHoles->index = 0;
    table->tableHoles->next = NULL;

    vm->objectsWithExternalHandles = NULL;
}

void nkiVmObjectDelete(struct NKVM *vm, struct NKVMObject *ob)
{
    nkuint32_t i;
    for(i = 0; i < nkiVMObjectHashBucketCount; i++) {
        struct NKVMObjectElement *el = ob->hashBuckets[i];
        while(el) {
            struct NKVMObjectElement *next = el->next;
            nkiFree(vm, el);
            el = next;
        }
    }
    nkiFree(vm, ob);
}

void nkiVmObjectTableDestroy(struct NKVM *vm)
{
    struct NKVMTable *table = &vm->objectTable;

    // Clear out the main table.
    nkuint32_t i;
    for(i = 0; i < table->capacity; i++) {
        if(table->objectTable[i]) {
            nkiVmObjectDelete(vm, table->objectTable[i]);
        }
        table->objectTable[i] = NULL;
    }
    nkiFree(vm, table->objectTable);
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

struct NKVMObject *nkiVmObjectTableGetEntryById(
    struct NKVMTable *table,
    nkuint32_t index)
{
    if(index >= table->capacity) {
        return NULL;
    }

    return table->objectTable[index];
}

void nkiVmObjectTableCreateHole(struct NKVM *vm, nkuint32_t holeIndex)
{
    struct NKVMTable *table = &vm->objectTable;
    struct NKVMTableHole *hole =
        nkiMalloc(vm, sizeof(struct NKVMTableHole));
    hole->index = holeIndex;
    hole->next = table->tableHoles;
    table->tableHoles = hole;
}

nkuint32_t nkiVmObjectTableCreateObject(
    struct NKVM *vm)
{
    struct NKVMTable *table = &vm->objectTable;
    nkuint32_t index = ~0;
    struct NKVMObject *newObject = nkiMalloc(vm, sizeof(struct NKVMObject));
    memset(newObject, 0, sizeof(*newObject));

    if(table->tableHoles) {

        // We can use an existing gap.

        struct NKVMTableHole *hole = table->tableHoles;
        table->tableHoles = hole->next;
        index = hole->index;
        nkiFree(vm, hole);

    } else {

        // FIXME: Limit object table size?

        // Looks like we have to re-size the object table.

        nkuint32_t oldCapacity = table->capacity;
        nkuint32_t newCapacity = oldCapacity << 1;
        nkuint32_t i;

        // If we're going to have to allocate more space in our table, we
        // need to check against our VM's string limit.
        if((newCapacity > vm->limits.maxObjects || !newCapacity)) {
            nkiAddError(
                vm, -1, "Reached object table capacity limit.");
            return NK_INVALID_VALUE;
        }

        table->objectTable = nkiReallocArray(
            vm,
            table->objectTable,
            sizeof(struct NKVMObject *), newCapacity);

        // Create hole objects for all our empty new space. Not that
        // we don't create one on the border between the old and new
        // space because that's where our new entry will be going. Add
        // holes back-to-front so we end up with holes roughly in
        // order, and will mostly allocate from the front of memory.
        for(i = newCapacity - 1; i >= oldCapacity + 1; i--) {
            nkiVmObjectTableCreateHole(vm, i);
            table->objectTable[i] = NULL;
        }

        table->capacity = newCapacity;
        index = oldCapacity;
    }

    newObject->objectTableIndex = index;
    newObject->lastGCPass = 0;
    newObject->gcCallback.id = NK_INVALID_VALUE;
    newObject->serializationCallback.id = NK_INVALID_VALUE;
    newObject->externalDataType.id = NK_INVALID_VALUE;
    newObject->externalData = NULL;
    table->objectTable[index] = newObject;

    // Tick the garbage collector so we eventually do another GC pass
    // when we have enough new objects.
    if(vm->gcInfo.gcNewObjectCountdown) {
        vm->gcInfo.gcNewObjectCountdown--;
    }

    return newObject->objectTableIndex;
}

void nkiVmObjectTableCleanOldObjects(
    struct NKVM *vm,
    nkuint32_t lastGCPass)
{
    struct NKVMTable *table = &vm->objectTable;
    nkuint32_t i;

    for(i = 0; i < table->capacity; i++) {

        struct NKVMObject *ob = table->objectTable[i];

        if(ob) {

            if(lastGCPass != ob->lastGCPass) {

                // struct NKVMTableHole *hole = NULL;

                // Run any external garbage collection callbacks.
                if(ob->gcCallback.id != NK_INVALID_VALUE) {
                    if(ob->gcCallback.id < vm->functionCount) {
                        struct NKVMFunction *func = &vm->functionTable[ob->gcCallback.id];
                        if(func->externalFunctionId.id != NK_INVALID_VALUE) {
                            if(func->externalFunctionId.id < vm->externalFunctionCount) {
                                struct NKValue funcValue;
                                struct NKValue argValue;
                                memset(&funcValue, 0, sizeof(funcValue));
                                funcValue.type = NK_VALUETYPE_FUNCTIONID;
                                funcValue.functionId = ob->gcCallback;
                                memset(&argValue, 0, sizeof(argValue));
                                argValue.type = NK_VALUETYPE_OBJECTID;
                                argValue.objectId = i;
                                nkiVmCallFunction(vm, &funcValue, 1, &argValue, NULL);
                            }
                        }
                    }
                }

                table->objectTable[i] = NULL;
                nkiVmObjectDelete(vm, ob);

                // Create a table hole for our new gap.
                nkiVmObjectTableCreateHole(vm, i);
            }
        }
    }
}

void nkiVmObjectClearEntry(
    struct NKVM *vm,
    struct NKVMObject *ob,
    struct NKValue *key)
{
    struct NKVMObjectElement **obList =
        &ob->hashBuckets[nkiValueHash(vm, key) & (nkiVMObjectHashBucketCount - 1)];

    struct NKVMObjectElement **elPtr = obList;
    while(*elPtr) {
        if(nkiValueCompare(vm, key, &(*elPtr)->key, nktrue) == 0) {
            break;
        }
        elPtr = &(*elPtr)->next;
    }

    if(*elPtr) {
        struct NKVMObjectElement *el = *elPtr;
        *elPtr = (*elPtr)->next;
        nkiFree(vm, el);

        assert(ob->size);
        ob->size--;
    }
}

struct NKValue *nkiVmObjectFindOrAddEntry(
    struct NKVM *vm,
    struct NKVMObject *ob,
    struct NKValue *key,
    nkbool noAdd)
{
    struct NKVMObjectElement **obList =
        &ob->hashBuckets[nkiValueHash(vm, key) & (nkiVMObjectHashBucketCount - 1)];

    struct NKVMObjectElement *el = *obList;
    while(el) {
        if(nkiValueCompare(vm, key, &el->key, nktrue) == 0) {
            break;
        }
        el = el->next;
    }

    // FIXME!!! We want non-existant things to return nil, and not be
    // added to the list if we're just reading!

    // If we can't find the entry, make a new one.
    if(!el) {

        // But only add if we're supposed to.
        if(noAdd) {
            return NULL;
        }

        ob->size++;
        if(!ob->size || ob->size > vm->limits.maxFieldsPerObject) {
            ob->size--;
            nkiAddError(
                vm, -1,
                "Reached object field count limit.");
            return NULL;
        }

        el = nkiMalloc(vm, sizeof(struct NKVMObjectElement));
        memset(el, 0, sizeof(struct NKVMObjectElement));
        el->next = *obList;
        el->key = *key;
        *obList = el;
    }

    return &el->value;
}

// ----------------------------------------------------------------------
// Object interface

struct NKVMObject *nkiVmGetObjectFromValue(struct NKVM *vm, struct NKValue *value)
{
    if(value->type == NK_VALUETYPE_OBJECTID) {
        struct NKVMObject *ob = nkiVmObjectTableGetEntryById(
            &vm->objectTable, value->objectId);
        return ob;
    }

    return NULL;
}

void nkiVmObjectAcquireHandle(struct NKVM *vm, struct NKValue *value)
{
    struct NKVMObject *ob = nkiVmGetObjectFromValue(vm, value);

    // Make sure we actually got an object.
    if(!ob) {
        nkiAddError(
            vm, -1, "Bad object ID in nkiVmObjectAcquireHandle.");
        return;
    }

    // FIXME: Check for integer overflow.
    ob->externalHandleCount++;

    // If we already have a handle count, it means the object is
    // in the linked list of handles, so we'll just increment the
    // counter and return.
    if(ob->externalHandleCount > 1) {
        return;
    }

    assert(!ob->nextObjectWithExternalHandles);
    assert(!ob->previousExternalHandleListPtr);

    // Add us to the linked list.
    ob->nextObjectWithExternalHandles =
        vm->objectsWithExternalHandles;
    ob->previousExternalHandleListPtr =
        &vm->objectsWithExternalHandles;
    if(vm->objectsWithExternalHandles) {
        vm->objectsWithExternalHandles->previousExternalHandleListPtr =
            &ob->nextObjectWithExternalHandles;
    }
    vm->objectsWithExternalHandles = ob;
}

void nkiVmObjectReleaseHandle(struct NKVM *vm, struct NKValue *value)
{
    struct NKVMObject *ob = nkiVmGetObjectFromValue(vm, value);

    // Make sure we actually got an object.
    if(!ob) {
        nkiAddError(
            vm, -1, "Bad object ID in nkiVmObjectAcquireHandle.");
        return;
    }

    if(ob->externalHandleCount == 0) {
        nkiAddError(
            vm, -1, "Tried to release handle for object with no external handles.");
    }

    ob->externalHandleCount--;

    // If there are handles left, then we're done.
    if(ob->externalHandleCount > 0) {
        return;
    }

    // Cut us out of the linked list.
    assert(ob);
    assert(ob->previousExternalHandleListPtr);
    *ob->previousExternalHandleListPtr = ob->nextObjectWithExternalHandles;
    ob->previousExternalHandleListPtr = NULL;
    ob->nextObjectWithExternalHandles = NULL;
}

void nkiVmObjectSetGarbageCollectionCallback(
    struct NKVM *vm,
    struct NKValue *object,
    NKVMExternalFunctionID callbackFunction)
{
    NKVMInternalFunctionID internalFunctionId = { NK_INVALID_VALUE };
    struct NKVMObject *ob;

    if(callbackFunction.id >= vm->externalFunctionCount) {
        nkiAddError(
            vm, -1, "Bad native function ID in nkiVmObjectSetGarbageCollectionCallback.");
    }

    internalFunctionId =
        nkiVmGetOrCreateInternalFunctionForExternalFunction(vm, callbackFunction);

    if(internalFunctionId.id >= vm->functionCount) {
        nkiAddError(
            vm, -1, "Bad function ID in nkiVmObjectSetGarbageCollectionCallback.");
        return;
    }

    ob = nkiVmGetObjectFromValue(vm, object);

    // Make sure we actually got an object.
    if(!ob) {
        nkiAddError(
            vm, -1, "Bad object ID in nkiVmObjectSetGarbageCollectionCallback.");
        return;
    }

    ob->gcCallback = internalFunctionId;
}

// FIXME: Consolidate this with code from
// nkiVmObjectSetGarbagecollectioncallback.
void nkiVmObjectSetSerializationCallback(
    struct NKVM *vm,
    struct NKValue *object,
    NKVMExternalFunctionID callbackFunction)
{
    NKVMInternalFunctionID internalFunctionId = { NK_INVALID_VALUE };
    struct NKVMObject *ob;

    if(callbackFunction.id >= vm->externalFunctionCount) {
        nkiAddError(
            vm, -1, "Bad native function ID in nkiVmObjectSetSerializationCallback.");
    }

    internalFunctionId =
        nkiVmGetOrCreateInternalFunctionForExternalFunction(vm, callbackFunction);

    if(internalFunctionId.id >= vm->functionCount) {
        nkiAddError(
            vm, -1, "Bad function ID in nkiVmObjectSetSerializationCallback.");
        return;
    }

    ob = nkiVmGetObjectFromValue(vm, object);

    // Make sure we actually got an object.
    if(!ob) {
        nkiAddError(
            vm, -1, "Bad object ID in nkiVmObjectSetSerializationCallback.");
        return;
    }

    ob->serializationCallback = internalFunctionId;
}

void nkiVmObjectSetExternalType(
    struct NKVM *vm,
    struct NKValue *object,
    NKVMExternalDataTypeID externalType)
{
    struct NKVMObject *ob = nkiVmGetObjectFromValue(vm, object);
    if(ob) {
        ob->externalDataType = externalType;
    } else {
        nkiAddError(
            vm, -1, "Bad object ID in nkiVmObjectSetExternalType.");
    }
}

NKVMExternalDataTypeID nkiVmObjectGetExternalType(
    struct NKVM *vm,
    struct NKValue *object)
{
    NKVMExternalDataTypeID ret = { NK_INVALID_VALUE };
    struct NKVMObject *ob = nkiVmGetObjectFromValue(vm, object);
    if(ob) {
        ret = ob->externalDataType;
    } else {
        nkiAddError(
            vm, -1, "Bad object ID in nkiVmObjectGetExternalType.");
    }
    return ret;
}

void nkiVmObjectSetExternalData(
    struct NKVM *vm,
    struct NKValue *object,
    void *data)
{
    struct NKVMObject *ob = nkiVmGetObjectFromValue(vm, object);
    if(ob) {
        ob->externalData = data;
    } else {
        nkiAddError(
            vm, -1, "Bad object ID in nkiVmObjectSetExternalData.");
    }
}

void *nkiVmObjectGetExternalData(
    struct NKVM *vm,
    struct NKValue *object)
{
    void *ret = NULL;
    struct NKVMObject *ob = nkiVmGetObjectFromValue(vm, object);
    if(ob) {
        ret = ob->externalData;
    } else {
        nkiAddError(
            vm, -1, "Bad object ID in nkiVmObjectGetExternalData.");
    }
    return ret;
}

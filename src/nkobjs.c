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

void nkiVmObjectTableInit(struct NKVM *vm)
{
    nkiTableInit(vm, &vm->objectTable);
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

    nkiTableDestroy(vm, table);
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

nkuint32_t nkiVmObjectTableCreateObject(
    struct NKVM *vm)
{
    struct NKVMTable *table = &vm->objectTable;
    nkuint32_t index = NK_INVALID_VALUE;
    struct NKVMObject *newObject = (struct NKVMObject *)nkiMalloc(
        vm, sizeof(struct NKVMObject));
    memset(newObject, 0, sizeof(*newObject));

    index = nkiTableAddEntry(vm, table, newObject);

    // Thanks AFL! This check/error might not be needed anymore, but
    // only because we removed the part where we would assign it to
    // the table slot ourselves (which would be disasterous if that
    // index was NK_INVALID_VALUE).
    if(index == NK_INVALID_VALUE) {
        nkiFree(vm, newObject);
        return NK_INVALID_VALUE;
    }

    newObject->objectTableIndex = index;
    newObject->lastGCPass = 0;
    newObject->externalDataType.id = NK_INVALID_VALUE;
    newObject->externalData = NULL;

    // Tick the garbage collector so we eventually do another GC pass
    // when we have enough new objects.
    if(vm->gcInfo.gcNewObjectCountdown) {
        vm->gcInfo.gcNewObjectCountdown--;
    }

    return newObject->objectTableIndex;
}

void nkiVmObjectTableCleanupObject(
    struct NKVM *vm,
    nkuint32_t objectTableIndex)
{
    struct NKVMTable *table = &vm->objectTable;
    struct NKVMObject *ob = table->objectTable[objectTableIndex];

    if(ob) {

        // Run any external data cleanup routines.
        if(ob->externalDataType.id != NK_INVALID_VALUE) {

            if(ob->externalDataType.id < vm->externalTypeCount) {

                NKVMExternalObjectCleanupCallback cleanupCallback =
                    vm->externalTypes[ob->externalDataType.id].cleanupCallback;

                if(cleanupCallback) {
                    struct NKValue val;
                    memset(&val, 0, sizeof(val));
                    val.type = NK_VALUETYPE_OBJECTID;
                    val.objectId = objectTableIndex;

                    cleanupCallback(vm, &val, ob->externalData);
                    ob->externalData = NULL;
                }

            } else {
                nkiAddError(vm, -1, "External type value out of range.");
            }
        }

        // Destroy the object itself.
        nkiVmObjectDelete(vm, ob);
    }

}

void nkiVmObjectTableCleanAllObjects(
    struct NKVM *vm)
{
    struct NKVMTable *table = &vm->objectTable;
    nkuint32_t i;

    for(i = 0; i < table->capacity; i++) {

        nkiVmObjectTableCleanupObject(vm, i);
        table->objectTable[i] = NULL;
    }
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
                nkiVmObjectTableCleanupObject(vm, i);
                nkiTableEraseEntry(vm, table, i);
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

    // If we can't find the entry, make a new one.
    if(!el) {

        // But only add if we're supposed to.
        if(noAdd) {
            return NULL;
        }

        el = (struct NKVMObjectElement *)nkiMalloc(
            vm, sizeof(struct NKVMObjectElement));
        memset(el, 0, sizeof(struct NKVMObjectElement));
        el->next = *obList;
        el->key = *key;
        *obList = el;

        ob->size++;
        if(!ob->size || ob->size > vm->limits.maxFieldsPerObject) {
            ob->size--;

            nkiAddError(
                vm, -1,
                "Reached object field count limit.");

            return NULL;
        }
    }

    return &el->value;
}

// ----------------------------------------------------------------------
// Object interface

// Note: No error additions or allocations in here, because this must
// be a cleanup-safe call.
struct NKVMObject *nkiVmGetObjectFromValue(struct NKVM *vm, struct NKValue *value)
{
    if(!value) {
        nkiAddError(vm, -1, "Bad value in nkiVmGetObjectFromValue.");
        return NULL;
    }

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

    ob->externalHandleCount++;

    // If we already have a handle count, it means the object is
    // in the linked list of handles, so we'll just increment the
    // counter and return.
    if(ob->externalHandleCount > 1) {
        return;
    }

    // Check for integer overflow.
    if(ob->externalHandleCount == 0) {
        ob->externalHandleCount = NK_UINT_MAX;
        nkiAddError(vm, -1, "Integer overflow in handle count.");
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

// Note: No error additions or allocations in here, because this must
// be a cleanup-safe call.
void *nkiVmObjectGetExternalData(
    struct NKVM *vm,
    struct NKValue *object)
{
    void *ret = NULL;
    struct NKVMObject *ob = nkiVmGetObjectFromValue(vm, object);
    if(ob) {
        ret = ob->externalData;
    }
    return ret;
}

void nkiVmObjectClearEntry_public(
    struct NKVM *vm,
    struct NKValue *objectId,
    struct NKValue *key)
{
    struct NKVMObject *ob = nkiVmGetObjectFromValue(vm, objectId);
    if(ob) {
        nkiVmObjectClearEntry(vm, ob, key);
    } else {
        nkiAddError(
            vm, -1, "Bad object ID in nkiVmObjectClearEntry.");
    }
}

struct NKValue *nkiVmObjectFindOrAddEntry_public(
    struct NKVM *vm,
    struct NKValue *objectId,
    struct NKValue *key,
    nkbool noAdd)
{
    struct NKVMObject *ob = nkiVmGetObjectFromValue(vm, objectId);
    if(ob) {
        return nkiVmObjectFindOrAddEntry(vm, ob, key, noAdd);
    } else {
        nkiAddError(
            vm, -1, "Bad object ID in nkiVmObjectFindOrAddEntry.");
    }
    return NULL;
}

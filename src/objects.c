#include "common.h"

void vmObjectTableInit(struct NKVM *vm)
{
    struct NKVMObjectTable *table = &vm->objectTable;

    table->tableHoles = NULL;

    // Create a table of one empty entry.
    table->objectTable = nkiMalloc(vm, sizeof(struct NKVMObject*));
    table->objectTableCapacity = 1;
    table->objectTable[0] = NULL;

    // Create the hole object that goes with the empty space.
    table->tableHoles = nkiMalloc(vm, sizeof(struct NKVMObjectTableHole));
    table->tableHoles->index = 0;
    table->tableHoles->next = NULL;

    table->objectsWithExternalHandles = NULL;
}

void vmObjectDelete(struct NKVM *vm, struct NKVMObject *ob)
{
    uint32_t i;
    for(i = 0; i < VMObjectHashBucketCount; i++) {
        struct NKVMObjectElement *el = ob->hashBuckets[i];
        while(el) {
            struct NKVMObjectElement *next = el->next;
            nkiFree(vm, el);
            el = next;
        }
    }
    nkiFree(vm, ob);
}

void vmObjectTableDestroy(struct NKVM *vm)
{
    struct NKVMObjectTable *table = &vm->objectTable;

    // Clear out the main table.
    uint32_t i;
    for(i = 0; i < table->objectTableCapacity; i++) {
        if(table->objectTable[i]) {
            vmObjectDelete(vm, table->objectTable[i]);
        }
        table->objectTable[i] = NULL;
    }
    nkiFree(vm, table->objectTable);
    table->objectTableCapacity = 0;

    // Clear out the holes list.
    {
        struct NKVMObjectTableHole *th = table->tableHoles;
        while(th) {
            struct NKVMObjectTableHole *next = th->next;
            nkiFree(vm, th);
            th = next;
        }
        table->tableHoles = NULL;
    }
}

struct NKVMObject *vmObjectTableGetEntryById(
    struct NKVMObjectTable *table,
    uint32_t index)
{
    if(index >= table->objectTableCapacity) {
        return NULL;
    }

    return table->objectTable[index];
}

uint32_t vmObjectTableCreateObject(
    struct NKVM *vm)
{
    struct NKVMObjectTable *table = &vm->objectTable;
    uint32_t index = ~0;
    struct NKVMObject *newObject = nkiMalloc(vm, sizeof(struct NKVMObject));
    memset(newObject, 0, sizeof(*newObject));

    if(table->tableHoles) {

        // We can use an existing gap.

        struct NKVMObjectTableHole *hole = table->tableHoles;
        table->tableHoles = hole->next;
        index = hole->index;
        nkiFree(vm, hole);

    } else {

        // FIXME: Limit object table size?

        // Looks like we have to re-size the object table.

        uint32_t oldCapacity = table->objectTableCapacity;
        uint32_t newCapacity = oldCapacity << 1;
        uint32_t i;

        // If we're going to have to allocate more space in our table, we
        // need to check against our VM's string limit.
        if((newCapacity > vm->limits.maxObjects || !newCapacity)) {
            nkiAddError(
                vm, -1, "Reached object table capacity limit.");
            return ~(uint32_t)0;
        }

        table->objectTable = nkiRealloc(
            vm,
            table->objectTable,
            sizeof(struct NKVMObject *) * newCapacity);

        // Create hole objects for all our empty new space. Not
        // that we don't create one on the border between the old
        // and new space because that's where our new entry will
        // be going.
        for(i = oldCapacity + 1; i < newCapacity; i++) {
            struct NKVMObjectTableHole *hole =
                nkiMalloc(vm, sizeof(struct NKVMObjectTableHole));
            hole->index = i;
            hole->next = table->tableHoles;
            table->tableHoles = hole;

            table->objectTable[i] = NULL;
        }

        table->objectTableCapacity = newCapacity;
        index = oldCapacity;
    }

    newObject->objectTableIndex = index;
    newObject->lastGCPass = 0;
    table->objectTable[index] = newObject;

    // Tick the garbage collector so we eventually do another GC pass
    // when we have enough new objects.
    if(vm->gcNewObjectCountdown) {
        vm->gcNewObjectCountdown--;
    }

    return newObject->objectTableIndex;
}

void vmObjectTableCleanOldObjects(
    struct NKVM *vm,
    uint32_t lastGCPass)
{
    struct NKVMObjectTable *table = &vm->objectTable;
    uint32_t i;

    dbgWriteLine("Purging unused objects...");
    dbgPush();

    for(i = 0; i < table->objectTableCapacity; i++) {

        struct NKVMObject *ob = table->objectTable[i];

        if(ob) {

            if(lastGCPass != ob->lastGCPass) {

                struct NKVMObjectTableHole *hole =
                    nkiMalloc(vm, sizeof(struct NKVMObjectTableHole));

                dbgWriteLine("Purging object at index %u", i);

                table->objectTable[i] = NULL;
                vmObjectDelete(vm, ob);

                // Create a table hole for our new gap.
                memset(hole, 0, sizeof(*hole));
                hole->index = i;
                hole->next = table->tableHoles;
                table->tableHoles = hole;
            }
        }
    }

    dbgPop();
}

void vmObjectClearEntry(
    struct NKVM *vm,
    struct NKVMObject *ob,
    struct NKValue *key)
{
    struct NKVMObjectElement **obList =
        &ob->hashBuckets[valueHash(vm, key) & (VMObjectHashBucketCount - 1)];

    struct NKVMObjectElement **elPtr = obList;
    while(*elPtr) {
        if(value_compare(vm, key, &(*elPtr)->key, true) == 0) {
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

struct NKValue *vmObjectFindOrAddEntry(
    struct NKVM *vm,
    struct NKVMObject *ob,
    struct NKValue *key,
    bool noAdd)
{
    struct NKVMObjectElement **obList =
        &ob->hashBuckets[valueHash(vm, key) & (VMObjectHashBucketCount - 1)];

    struct NKVMObjectElement *el = *obList;
    while(el) {
        if(value_compare(vm, key, &el->key, true) == 0) {
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

void vmObjectTableDump(struct NKVM *vm)
{
    uint32_t index;
    printf("Object table dump...\n");
    for(index = 0; index < vm->objectTable.objectTableCapacity; index++) {
        if(vm->objectTable.objectTable[index]) {

            struct NKVMObject *ob = vm->objectTable.objectTable[index];
            uint32_t bucket;

            printf("%4u ", index);
            printf("Object\n");

            for(bucket = 0; bucket < VMObjectHashBucketCount; bucket++) {
                printf("      Hash bucket %u\n", bucket);
                struct NKVMObjectElement *el = ob->hashBuckets[bucket];
                while(el) {
                    printf("        ");
                    value_dump(vm, &el->key);
                    printf(" = ");
                    value_dump(vm, &el->value);
                    printf("\n");
                    el = el->next;
                }
            }
            printf("\n");
        }
    }
}

void nkiVmObjectAcquireHandle(struct NKVM *vm, struct NKValue *value)
{
    if(value->type == NK_VALUETYPE_OBJECTID) {

        struct NKVMObject *ob = vmObjectTableGetEntryById(
            &vm->objectTable, value->objectId);

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
            vm->objectTable.objectsWithExternalHandles;
        ob->previousExternalHandleListPtr =
            &vm->objectTable.objectsWithExternalHandles;
        if(vm->objectTable.objectsWithExternalHandles) {
            vm->objectTable.objectsWithExternalHandles->previousExternalHandleListPtr =
                &ob->nextObjectWithExternalHandles;
        }
        vm->objectTable.objectsWithExternalHandles = ob;

    } else {

        nkiAddError(
            vm, -1, "Tried to acquire handle for non-object.");
    }
}

void nkiVmObjectReleaseHandle(struct NKVM *vm, struct NKValue *value)
{
    if(value->type == NK_VALUETYPE_OBJECTID) {

        struct NKVMObject *ob = vmObjectTableGetEntryById(
            &vm->objectTable, value->objectId);

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
        *ob->previousExternalHandleListPtr = ob->nextObjectWithExternalHandles;
        ob->previousExternalHandleListPtr = NULL;
        ob->nextObjectWithExternalHandles = NULL;

    } else {

        nkiAddError(
            vm, -1, "Tried to release handle for non-object.");
    }
}


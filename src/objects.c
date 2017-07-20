#include "common.h"

void vmObjectTableInit(struct VM *vm)
{
    struct VMObjectTable *table = &vm->objectTable;

    table->tableHoles = NULL;

    // Create a table of one empty entry.
    table->objectTable = nkMalloc(vm, sizeof(struct VMObject*));
    if(table->objectTable) {
        table->objectTableCapacity = 1;
        table->objectTable[0] = NULL;
    }

    // Create the hole object that goes with the empty space.
    table->tableHoles = nkMalloc(vm, sizeof(struct VMObjectTableHole));
    if(table->tableHoles) {
        table->tableHoles->index = 0;
        table->tableHoles->next = NULL;
    }

    table->objectsWithExternalHandles = NULL;
}

void vmObjectDelete(struct VM *vm, struct VMObject *ob)
{
    uint32_t i;
    for(i = 0; i < VMObjectHashBucketCount; i++) {
        struct VMObjectElement *el = ob->hashBuckets[i];
        while(el) {
            struct VMObjectElement *next = el->next;
            nkFree(vm, el);
            el = next;
        }
    }
    nkFree(vm, ob);
}

void vmObjectTableDestroy(struct VM *vm)
{
    struct VMObjectTable *table = &vm->objectTable;

    // Clear out the main table.
    uint32_t i;
    for(i = 0; i < table->objectTableCapacity; i++) {
        if(table->objectTable[i]) {
            vmObjectDelete(vm, table->objectTable[i]);
        }
        table->objectTable[i] = NULL;
    }
    nkFree(vm, table->objectTable);
    table->objectTableCapacity = 0;

    // Clear out the holes list.
    {
        struct VMObjectTableHole *th = table->tableHoles;
        while(th) {
            struct VMObjectTableHole *next = th->next;
            nkFree(vm, th);
            th = next;
        }
        table->tableHoles = NULL;
    }
}

struct VMObject *vmObjectTableGetEntryById(
    struct VMObjectTable *table,
    uint32_t index)
{
    if(index >= table->objectTableCapacity) {
        return NULL;
    }

    return table->objectTable[index];
}

uint32_t vmObjectTableCreateObject(
    struct VM *vm)
{
    struct VMObjectTable *table = &vm->objectTable;
    uint32_t index = ~0;
    struct VMObject *newObject = nkMalloc(vm, sizeof(struct VMObject));

    if(!newObject) {
        return index;
    }

    memset(newObject, 0, sizeof(*newObject));

    if(table->tableHoles) {

        // We can use an existing gap.

        struct VMObjectTableHole *hole = table->tableHoles;
        table->tableHoles = hole->next;
        index = hole->index;
        nkFree(vm, hole);

    } else {

        // FIXME: Limit object table size?

        // Looks like we have to re-size the object table.

        uint32_t oldCapacity = table->objectTableCapacity;
        uint32_t newCapacity = oldCapacity << 1;
        uint32_t i;

        // If we're going to have to allocate more space in our table, we
        // need to check against our VM's string limit.
        if((newCapacity > vm->limits.maxObjects || !newCapacity)) {
            errorStateAddError(
                &vm->errorState,
                -1, "Reached object table capacity limit.");
            return ~(uint32_t)0;
        }

        table->objectTable = nkRealloc(
            vm,
            table->objectTable,
            sizeof(struct VMObject *) * newCapacity);

        if(!table->objectTable) {
            free(newObject);
            return ~0;
        }

        // Create hole objects for all our empty new space. Not
        // that we don't create one on the border between the old
        // and new space because that's where our new entry will
        // be going.
        for(i = oldCapacity + 1; i < newCapacity; i++) {

            struct VMObjectTableHole *hole =
                nkMalloc(vm, sizeof(struct VMObjectTableHole));

            if(!hole) {
                free(newObject);
                return ~0;
            }

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
    return newObject->objectTableIndex;
}

void vmObjectTableCleanOldObjects(
    struct VM *vm,
    uint32_t lastGCPass)
{
    struct VMObjectTable *table = &vm->objectTable;
    uint32_t i;

    dbgWriteLine("Purging unused objects...");
    dbgPush();

    for(i = 0; i < table->objectTableCapacity; i++) {

        struct VMObject *ob = table->objectTable[i];

        if(ob) {

            if(lastGCPass != ob->lastGCPass) {

                struct VMObjectTableHole *hole =
                    nkMalloc(vm, sizeof(struct VMObjectTableHole));

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
    struct VM *vm,
    struct VMObject *ob,
    struct Value *key)
{
    struct VMObjectElement **obList =
        &ob->hashBuckets[valueHash(vm, key) & (VMObjectHashBucketCount - 1)];

    struct VMObjectElement **elPtr = obList;
    while(*elPtr) {
        if(value_compare(vm, key, &(*elPtr)->key, true) == 0) {
            break;
        }
        elPtr = &(*elPtr)->next;
    }

    if(*elPtr) {
        struct VMObjectElement *el = *elPtr;
        *elPtr = (*elPtr)->next;
        nkFree(vm, el);

        assert(ob->size);
        ob->size--;
    }
}

struct Value *vmObjectFindOrAddEntry(
    struct VM *vm,
    struct VMObject *ob,
    struct Value *key)
{
    struct VMObjectElement **obList =
        &ob->hashBuckets[valueHash(vm, key) & (VMObjectHashBucketCount - 1)];

    struct VMObjectElement *el = *obList;
    while(el) {
        if(value_compare(vm, key, &el->key, true) == 0) {
            break;
        }
        el = el->next;
    }

    // If we can't find the entry, make a new one.
    if(!el) {

        ob->size++;
        if(!ob->size || ob->size > vm->limits.maxFieldsPerObject) {
            ob->size--;
            errorStateAddError(
                &vm->errorState, -1,
                "Reached object field count limit.");
            return NULL;
        }

        el = nkMalloc(vm, sizeof(struct VMObjectElement));
        if(!el) {
            return NULL;
        }
        memset(el, 0, sizeof(struct VMObjectElement));
        el->next = *obList;
        el->key = *key;
        *obList = el;
    }

    return &el->value;

    return NULL;
}

void vmObjectTableDump(struct VM *vm)
{
    uint32_t index;
    printf("Object table dump...\n");
    for(index = 0; index < vm->objectTable.objectTableCapacity; index++) {
        if(vm->objectTable.objectTable[index]) {

            struct VMObject *ob = vm->objectTable.objectTable[index];
            uint32_t bucket;

            printf("%4u ", index);
            printf("Object\n");

            for(bucket = 0; bucket < VMObjectHashBucketCount; bucket++) {
                printf("      Hash bucket %u\n", bucket);
                struct VMObjectElement *el = ob->hashBuckets[bucket];
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

void vmObjectAcquireHandle(struct VM *vm, struct Value *value)
{
    if(value->type == VALUETYPE_OBJECTID) {

        struct VMObject *ob = vmObjectTableGetEntryById(
            &vm->objectTable, value->objectId);

        // Make sure we actually got an object.
        if(!ob) {
            errorStateAddError(
                &vm->errorState,
                -1, "Bad object ID in vmObjectAcquireHandle.");
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

        errorStateAddError(
            &vm->errorState,
            -1, "Tried to acquire handle for non-object.");
    }
}

void vmObjectReleaseHandle(struct VM *vm, struct Value *value)
{
    if(value->type == VALUETYPE_OBJECTID) {

        struct VMObject *ob = vmObjectTableGetEntryById(
            &vm->objectTable, value->objectId);

        // Make sure we actually got an object.
        if(!ob) {
            errorStateAddError(
                &vm->errorState,
                -1, "Bad object ID in vmObjectAcquireHandle.");
            return;
        }

        if(ob->externalHandleCount == 0) {
            errorStateAddError(
                &vm->errorState,
                -1, "Tried to release handle for object with no external handles.");
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

        errorStateAddError(
            &vm->errorState,
            -1, "Tried to release handle for non-object.");
    }
}


#include "common.h"

void vmObjectTableInit(struct VMObjectTable *table)
{
    table->tableHoles = NULL;

    // Create a table of one empty entry.
    table->objectTable = malloc(sizeof(struct VMObject*));
    table->objectTableCapacity = 1;
    table->objectTable[0] = NULL;

    // Create the hole object that goes with the empty space.
    table->tableHoles = malloc(sizeof(struct VMObjectTableHole));
    table->tableHoles->index = 0;
    table->tableHoles->next = NULL;
}

void vmObjectDelete(struct VMObject *ob)
{
    uint32_t i;
    for(i = 0; i < VMObjectHashBucketCount; i++) {
        struct VMObjectElement *el = ob->hashBuckets[i];
        while(el) {
            struct VMObjectElement *next = el->next;
            free(el);
            el = next;
        }
    }
    free(ob);
}

void vmObjectTableDestroy(struct VMObjectTable *table)
{
    // Clear out the main table.
    uint32_t i;
    for(i = 0; i < table->objectTableCapacity; i++) {
        if(table->objectTable[i]) {
            vmObjectDelete(table->objectTable[i]);
        }
        table->objectTable[i] = NULL;
    }
    free(table->objectTable);
    table->objectTableCapacity = 0;

    // Clear out the holes list.
    {
        struct VMObjectTableHole *th = table->tableHoles;
        while(th) {
            struct VMObjectTableHole *next = th->next;
            free(th);
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
    struct VMObjectTable *table)
{
    uint32_t index = ~0;
    struct VMObject *newObject = malloc(sizeof(struct VMObject));
    memset(newObject, 0, sizeof(*newObject));

    if(table->tableHoles) {

        // We can use an existing gap.

        struct VMObjectTableHole *hole = table->tableHoles;
        table->tableHoles = hole->next;
        index = hole->index;
        free(hole);

    } else {

        // FIXME: Limit object table size?

        // Looks like we have to re-size the object table.

        uint32_t oldCapacity = table->objectTableCapacity;
        uint32_t newCapacity = oldCapacity << 1;
        uint32_t i;

        table->objectTable = realloc(
            table->objectTable,
            sizeof(struct VMObject *) * newCapacity);

        // Create hole objects for all our empty new space. Not
        // that we don't create one on the border between the old
        // and new space because that's where our new entry will
        // be going.
        for(i = oldCapacity + 1; i < newCapacity; i++) {
            struct VMObjectTableHole *hole =
                malloc(sizeof(struct VMObjectTableHole));
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
    struct VMObjectTable *table, uint32_t lastGCPass)
{
    uint32_t i;

    dbgWriteLine("Purging unused objects...");
    dbgPush();

    for(i = 0; i < table->objectTableCapacity; i++) {

        struct VMObject *ob = table->objectTable[i];

        if(ob) {

            if(lastGCPass != ob->lastGCPass) {

                struct VMObjectTableHole *hole =
                    malloc(sizeof(struct VMObjectTableHole));

                dbgWriteLine("Purging object at index %u", i);

                table->objectTable[i] = NULL;
                vmObjectDelete(ob);

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
        free(el);

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
        el = malloc(sizeof(struct VMObjectElement));
        memset(el, 0, sizeof(struct VMObjectElement));
        el->next = *obList;
        el->key = *key;
        *obList = el;
        ob->size++;
        // FIXME: Check integer overflow on size.
        // FIXME: Check for size > object size limit.
    }

    return &el->value;

    return NULL;
}

void vmObjectTableDump(struct VM *vm)
{
    uint32_t index;
    printf("Object table dump...\n");
    for(index = 0; index < vm->objectTable.objectTableCapacity; index++) {
        printf("%4u ", index);
        if(vm->objectTable.objectTable[index]) {

            struct VMObject *ob = vm->objectTable.objectTable[index];
            uint32_t bucket;

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
        } else {
            printf("No object");
        }
        printf("\n");
    }
}


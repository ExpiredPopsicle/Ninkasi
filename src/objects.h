#ifndef OBJECTS_H
#define OBJECTS_H

// Dumb linked-list for object data. We should replace this some day.
struct VMObjectElement
{
    struct Value key;
    struct Value value;
    struct VMObjectElement *next;
};

#define VMObjectHashBucketCount 16

struct VMObject
{
    uint32_t objectTableIndex;
    uint32_t lastGCPass;

    // Cached count of number of entries.
    uint32_t size;

    // TODO: External handle count.

    // struct VMObjectElement *data;
    struct VMObjectElement *hashBuckets[VMObjectHashBucketCount];
};

struct VMObjectTableHole
{
    struct VMObjectTableHole *next;
    uint32_t index;
};

struct VMObjectTable
{
    struct VMObjectTableHole *tableHoles;
    struct VMObject **objectTable;
    uint32_t objectTableCapacity;
};

void vmObjectTableInit(struct VMObjectTable *table);
void vmObjectTableDestroy(struct VMObjectTable *table);

struct VMObject *vmObjectTableGetEntryById(
    struct VMObjectTable *table,
    uint32_t index);

uint32_t vmObjectTableCreateObject(
    struct VMObjectTable *table);

void vmObjectTableCleanOldObjects(
    struct VMObjectTable *table, uint32_t lastGCPass);

struct Value *vmObjectFindOrAddEntry(
    struct VM *vm,
    struct VMObject *ob,
    struct Value *key);

void vmObjectClearEntry(
    struct VM *vm,
    struct VMObject *ob,
    struct Value *key);

void vmObjectTableDump(struct VM *vm);

#endif

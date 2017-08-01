#ifndef NINKASI_OBJECTS_H
#define NINKASI_OBJECTS_H

// Dumb linked-list for object data. We should replace this some day.
struct NKVMObjectElement
{
    struct NKValue key;
    struct NKValue value;
    struct NKVMObjectElement *next;
};

#define VMObjectHashBucketCount 16

struct NKVMObject
{
    uint32_t objectTableIndex;
    uint32_t lastGCPass;

    // Cached count of number of entries.
    uint32_t size;

    struct NKVMObjectElement *hashBuckets[VMObjectHashBucketCount];

    // External handle stuff.
    struct NKVMObject *nextObjectWithExternalHandles;
    struct NKVMObject **previousExternalHandleListPtr;
    uint32_t externalHandleCount;
};

struct NKVMObjectTableHole
{
    struct NKVMObjectTableHole *next;
    uint32_t index;
};

struct NKVMObjectTable
{
    struct NKVMObjectTableHole *tableHoles;
    struct NKVMObject **objectTable;
    uint32_t objectTableCapacity;

    struct NKVMObject *objectsWithExternalHandles;
};

void vmObjectTableInit(struct NKVM *vm);
void vmObjectTableDestroy(struct NKVM *vm);

struct NKVMObject *vmObjectTableGetEntryById(
    struct NKVMObjectTable *table,
    uint32_t index);

uint32_t vmObjectTableCreateObject(
    struct NKVM *vm);

void vmObjectTableCleanOldObjects(
    struct NKVM *vm,
    uint32_t lastGCPass);

// FIXME: Make a public version of this that takes Value* instead of
// VMObject*.
struct NKValue *vmObjectFindOrAddEntry(
    struct NKVM *vm,
    struct NKVMObject *ob,
    struct NKValue *key,
    bool noAdd);

// FIXME: Make a public version of this that takes Value* instead of
// VMObject*.
void vmObjectClearEntry(
    struct NKVM *vm,
    struct NKVMObject *ob,
    struct NKValue *key);

void vmObjectTableDump(struct NKVM *vm);

// Internal version of nkxVmObjectAcquireHandle.
void nkiVmObjectAcquireHandle(struct NKVM *vm, struct NKValue *value);

// Internal version of nkxVmObjectReleaseHandle.
void nkiVmObjectReleaseHandle(struct NKVM *vm, struct NKValue *value);

#endif // NINKASI_OBJECTS_H

#ifndef NINKASI_OBJECTS_H
#define NINKASI_OBJECTS_H

/// Dumb linked-list for key/value pairs inside of an object.
struct NKVMObjectElement
{
    struct NKValue key;
    struct NKValue value;
    struct NKVMObjectElement *next;
};

#define nkiVMObjectHashBucketCount 16

struct NKVMObject
{
    nkuint32_t objectTableIndex;
    nkuint32_t lastGCPass;

    // Cached count of number of entries.
    nkuint32_t size;

    struct NKVMObjectElement *hashBuckets[nkiVMObjectHashBucketCount];

    // External handle stuff.
    struct NKVMObject *nextObjectWithExternalHandles;
    struct NKVMObject **previousExternalHandleListPtr;
    nkuint32_t externalHandleCount;
};

struct NKVMObjectTableHole
{
    struct NKVMObjectTableHole *next;
    nkuint32_t index;
};

struct NKVMObjectTable
{
    struct NKVMObjectTableHole *tableHoles;
    struct NKVMObject **objectTable;
    nkuint32_t objectTableCapacity;

    struct NKVMObject *objectsWithExternalHandles;
};

void nkiVmObjectTableInit(struct NKVM *vm);
void nkiVmObjectTableDestroy(struct NKVM *vm);

struct NKVMObject *nkiVmObjectTableGetEntryById(
    struct NKVMObjectTable *table,
    nkuint32_t index);

nkuint32_t nkiVmObjectTableCreateObject(
    struct NKVM *vm);

void nkiVmObjectTableCleanOldObjects(
    struct NKVM *vm,
    nkuint32_t lastGCPass);

// FIXME: Make a public version of this that takes Value* instead of
// VMObject*.
struct NKValue *nkiVmObjectFindOrAddEntry(
    struct NKVM *vm,
    struct NKVMObject *ob,
    struct NKValue *key,
    nkbool noAdd);

// FIXME: Make a public version of this that takes Value* instead of
// VMObject*.
void nkiVmObjectClearEntry(
    struct NKVM *vm,
    struct NKVMObject *ob,
    struct NKValue *key);

void nkiVmObjectTableDump(struct NKVM *vm);

// Internal version of nkxVmObjectAcquireHandle.
void nkiVmObjectAcquireHandle(struct NKVM *vm, struct NKValue *value);

// Internal version of nkxVmObjectReleaseHandle.
void nkiVmObjectReleaseHandle(struct NKVM *vm, struct NKValue *value);

#endif // NINKASI_OBJECTS_H

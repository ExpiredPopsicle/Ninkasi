#ifndef OBJECTS_H
#define OBJECTS_H

// ----------------------------------------------------------------------
// Public interface

// Note: This should mainly deal with struct Value pointers and not
// VMObject pointers directly.

/// Increment the reference count for an object. This keeps it (and
/// everything referenced from it) from being garbage collected.
void vmObjectAcquireHandle(struct VM *vm, struct Value *value);

/// Decrement the reference count for an object. Objects that reach
/// zero references and have no owning references inside the VM will
/// be deleted next garbage collection pass.
void vmObjectReleaseHandle(struct VM *vm, struct Value *value);

// ----------------------------------------------------------------------
// Internals

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

    struct VMObjectElement *hashBuckets[VMObjectHashBucketCount];

    // External handle stuff.
    struct VMObject *nextObjectWithExternalHandles;
    struct VMObject **previousExternalHandleListPtr;
    uint32_t externalHandleCount;
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

    struct VMObject *objectsWithExternalHandles;
};

void vmObjectTableInit(struct VMObjectTable *table);
void vmObjectTableDestroy(struct VMObjectTable *table);

struct VMObject *vmObjectTableGetEntryById(
    struct VMObjectTable *table,
    uint32_t index);

uint32_t vmObjectTableCreateObject(
    struct VM *vm);

void vmObjectTableCleanOldObjects(
    struct VMObjectTable *table, uint32_t lastGCPass);

// FIXME: Make a public version of this that takes Value* instead of
// VMObject*.
struct Value *vmObjectFindOrAddEntry(
    struct VM *vm,
    struct VMObject *ob,
    struct Value *key);

// FIXME: Make a public version of this that takes Value* instead of
// VMObject*.
void vmObjectClearEntry(
    struct VM *vm,
    struct VMObject *ob,
    struct Value *key);

void vmObjectTableDump(struct VM *vm);

#endif

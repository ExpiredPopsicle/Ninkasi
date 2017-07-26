#ifndef NINKASI_VMSTRING_H
#define NINKASI_VMSTRING_H

#include "basetype.h"

struct NKVMString
{
    struct NKVMString *nextInHashBucket;
    uint32_t stringTableIndex;
    uint32_t lastGCPass;
    bool dontGC;
    uint32_t hash;

    // Must be last. We're going to allocated VMStrings with enough
    // extra space that we can treat this array as an
    // arbitrarily-sized one, with the data extending off the end of
    // the structure.
    char str[1];
};

struct NKVMStringTableHole
{
    struct NKVMStringTableHole *next;
    uint32_t index;
};

#define nkVmStringHashTableSize 256

struct NKVMStringTable
{
    struct NKVMString *stringsByHash[nkVmStringHashTableSize];

    struct NKVMStringTableHole *tableHoles;

    struct NKVMString **stringTable;
    uint32_t stringTableCapacity;
};

void vmStringTableInit(struct VM *vm);
void vmStringTableDestroy(struct VM *vm);

struct NKVMString *vmStringTableGetEntryById(
    struct NKVMStringTable *table,
    uint32_t index);

const char *vmStringTableGetStringById(
    struct NKVMStringTable *table,
    uint32_t index);

uint32_t vmStringTableFindOrAddString(
    struct VM *vm,
    const char *str);

void vmStringTableDump(struct NKVMStringTable *table);

void vmStringTableCleanOldStrings(
    struct VM *vm,
    uint32_t lastGCPass);

#endif // NINKASI_VMSTRING_H


#ifndef NINKASI_VMSTRING_H
#define NINKASI_VMSTRING_H

#include "basetype.h"

struct VMString
{
    struct VMString *nextInHashBucket;
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

struct VMStringTableHole
{
    struct VMStringTableHole *next;
    uint32_t index;
};

#define vmStringTableHashTableSize 256

struct VMStringTable
{
    struct VMString *stringsByHash[vmStringTableHashTableSize];

    struct VMStringTableHole *tableHoles;

    struct VMString **stringTable;
    uint32_t stringTableCapacity;
};

void vmStringTableInit(struct VM *vm);
void vmStringTableDestroy(struct VM *vm);

struct VMString *vmStringTableGetEntryById(
    struct VMStringTable *table,
    uint32_t index);

const char *vmStringTableGetStringById(
    struct VMStringTable *table,
    uint32_t index);

uint32_t vmStringTableFindOrAddString(
    struct VM *vm,
    const char *str);

void vmStringTableDump(struct VMStringTable *table);

void vmStringTableCleanOldStrings(
    struct VM *vm,
    uint32_t lastGCPass);

#endif // NINKASI_VMSTRING_H


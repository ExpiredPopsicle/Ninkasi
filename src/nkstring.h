#ifndef NINKASI_VMSTRING_H
#define NINKASI_VMSTRING_H

#include "basetype.h"

struct NKVMString
{
    struct NKVMString *nextInHashBucket;
    nkuint32_t stringTableIndex;
    nkuint32_t lastGCPass;
    nkbool dontGC;
    nkuint32_t hash;

    // Must be last. We're going to allocated VMStrings with enough
    // extra space that we can treat this array as an
    // arbitrarily-sized one, with the data extending off the end of
    // the structure.
    char str[1];
};

struct NKVMStringTableHole
{
    struct NKVMStringTableHole *next;
    nkuint32_t index;
};

#define nkiVmStringHashTableSize 256

struct NKVMStringTable
{
    struct NKVMString *stringsByHash[nkiVmStringHashTableSize];

    struct NKVMStringTableHole *tableHoles;

    struct NKVMString **stringTable;
    nkuint32_t stringTableCapacity;
};

void nkiVmStringTableInit(struct NKVM *vm);
void nkiVmStringTableDestroy(struct NKVM *vm);

struct NKVMString *nkiVmStringTableGetEntryById(
    struct NKVMStringTable *table,
    nkuint32_t index);

const char *nkiVmStringTableGetStringById(
    struct NKVMStringTable *table,
    nkuint32_t index);

nkuint32_t nkiVmStringTableFindOrAddString(
    struct NKVM *vm,
    const char *str);

void nkiVmStringTableDump(struct NKVMStringTable *table);

void nkiVmStringTableCleanOldStrings(
    struct NKVM *vm,
    nkuint32_t lastGCPass);

#endif // NINKASI_VMSTRING_H


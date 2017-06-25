#ifndef VMSTRING_H
#define VMSTRING_H

#include "basetype.h"

struct VMString
{
    struct VMString *nextInHashBucket;
    uint32_t stringTableIndex;
    uint32_t lastGCPass;
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

void vmStringTableInit(struct VMStringTable *table);
void vmStringTableDestroy(struct VMStringTable *table);

struct VMString *vmStringTableGetEntryById(
    struct VMStringTable *table,
    uint32_t index);

const char *vmStringTableGetStringById(
    struct VMStringTable *table,
    uint32_t index);

uint32_t vmStringTableFindOrAddString(
    struct VMStringTable *table,
    const char *str);

void vmStringTableDump(struct VMStringTable *table);

void vmStringTableCleanOldStrings(
    struct VMStringTable *table, uint32_t lastGCPass);

#endif

#include "common.h"

static uint32_t stringHash(const char *in)
{
    uint32_t len = strlen(in);
    uint32_t a = 1;
    uint32_t b = 0;
    uint32_t i;

    for(i = 0; i < len; i++) {
        a = (a + in[i]) % 65521;
        b = (b + a) % 65521;
    }

    return (b << 16) | a;
}

void nkiVmStringTableInit(struct NKVM *vm)
{
    struct NKVMStringTable *table = &vm->stringTable;

    memset(&table->stringsByHash, 0, sizeof(table->stringsByHash));
    table->tableHoles = NULL;

    // Create a table with a capacity of a single string.
    table->stringTable = nkiMalloc(vm, sizeof(struct NKVMString*));
    table->stringTableCapacity = 1;
    table->stringTable[0] = NULL;

    // Create a "hole" object indicating that we have space in the
    // table.
    table->tableHoles = nkiMalloc(vm, sizeof(struct NKVMStringTableHole));
    table->tableHoles->index = 0;
    table->tableHoles->next = NULL;
}

void nkiVmStringTableDestroy(struct NKVM *vm)
{
    struct NKVMStringTable *table = &vm->stringTable;

    // Clear out the main table.
    uint32_t i;
    for(i = 0; i < table->stringTableCapacity; i++) {
        nkiFree(vm, table->stringTable[i]);
        table->stringTable[i] = NULL;
    }
    nkiFree(vm, table->stringTable);
    table->stringTableCapacity = 0;

    // Clear out the holes list.
    {
        struct NKVMStringTableHole *th = table->tableHoles;
        while(th) {
            struct NKVMStringTableHole *next = th->next;
            nkiFree(vm, th);
            th = next;
        }
        table->tableHoles = NULL;
    }

    // Zero-out the hash table, just for safety.
    memset(table->stringsByHash, 0, sizeof(table->stringsByHash));
}

struct NKVMString *vmStringTableGetEntryById(
    struct NKVMStringTable *table, uint32_t index)
{
    if(index >= table->stringTableCapacity) {
        return NULL;
    }

    return table->stringTable[index];
}

const char *vmStringTableGetStringById(
    struct NKVMStringTable *table,
    uint32_t index)
{
    struct NKVMString *vmstr = vmStringTableGetEntryById(table, index);
    return vmstr ? vmstr->str : NULL;
}

uint32_t vmStringTableFindOrAddString(
    struct NKVM *vm,
    const char *str)
{
    struct NKVMStringTable *table = &vm->stringTable;
    uint32_t hash = stringHash(str);

    // See if we have this string already.
    struct NKVMString *hashBucket =
        table->stringsByHash[hash & (nkiVmStringHashTableSize - 1)];
    struct NKVMString *cur = hashBucket;

    while(cur) {
        if(!strcmp(cur->str, str)) {
            return cur->stringTableIndex;
        }
        cur = cur->nextInHashBucket;
    }

    // Check our length.
    uint32_t len = strlen(str);
    if(len > vm->limits.maxStringLength) {
        nkiAddError(
            vm, -1, "Reached string length limit.");
        return ~(uint32_t)0;
    }

    // If we've reach this point, then we don't have the string yet,
    // so we'll go ahead and make a new entry.
    {
        struct NKVMString *newString =
            nkiMalloc(vm, sizeof(struct NKVMString) + len + 1);
        uint32_t index = 0;

        if(table->tableHoles) {

            // We can use an existing gap.

            struct NKVMStringTableHole *hole = table->tableHoles;
            table->tableHoles = hole->next;
            index = hole->index;
            nkiFree(vm, hole);

            // TODO: Remove.
            dbgWriteLine("Filled a string table hole at index %d", index);

        } else {

            // Looks like we have to re-size the string table.

            uint32_t oldCapacity = table->stringTableCapacity;
            uint32_t newCapacity = oldCapacity << 1;
            uint32_t i;

            // If we're going to have to allocate more space in our table, we
            // need to check against our VM's string limit.
            if((newCapacity > vm->limits.maxStrings || !newCapacity)) {
                nkiAddError(
                    vm, -1, "Reached string table capacity limit.");
                return ~(uint32_t)0;
            }

            table->stringTable = nkiRealloc(
                vm,
                table->stringTable,
                sizeof(struct NKVMString *) * newCapacity);

            // Create hole objects for all our empty new space. Not
            // that we don't create one on the border between the old
            // and new space because that's where our new entry will
            // be going.
            for(i = oldCapacity + 1; i < newCapacity; i++) {
                struct NKVMStringTableHole *hole =
                    nkiMalloc(vm, sizeof(struct NKVMStringTableHole));
                hole->index = i;
                hole->next = table->tableHoles;
                table->tableHoles = hole;

                table->stringTable[i] = NULL;
            }

            table->stringTableCapacity = newCapacity;
            index = oldCapacity;

            // TODO: Remove.
            dbgWriteLine("Expanded string table to make room for index %d", index);
        }

        newString->stringTableIndex = index;
        newString->lastGCPass = 0;
        newString->dontGC = false;
        newString->hash = stringHash(str);
        strcpy(newString->str, str);
        newString->nextInHashBucket = hashBucket;
        table->stringsByHash[hash & (nkiVmStringHashTableSize - 1)] = newString;
        table->stringTable[index] = newString;

        return newString->stringTableIndex;
    }
}

void vmStringTableDump(struct NKVMStringTable *table)
{
    uint32_t i;
    printf("String table dump...\n");

    printf("  Hash table...\n");
    for(i = 0; i < nkiVmStringHashTableSize  ; i++) {
        struct NKVMString *str = table->stringsByHash[i];
        printf("    %.2x\n", i);
        while(str) {
            printf("      %s\n", str->str);
            str = str->nextInHashBucket;
        }
    }

    printf("  Main table...\n");
    for(i = 0; i < table->stringTableCapacity; i++) {
        struct NKVMString *str = table->stringTable[i];
        printf("    %.2x\n", i);
        printf("      %s\n", str ? str->str : "<null>");
    }
}

void vmStringTableCleanOldStrings(
    struct NKVM *vm,
    uint32_t lastGCPass)
{
    struct NKVMStringTable *table = &vm->stringTable;
    uint32_t i;

    dbgWriteLine("Purging unused strings...");
    dbgPush();

    for(i = 0; i < nkiVmStringHashTableSize; i++) {

        struct NKVMString **lastPtr = &table->stringsByHash[i];
        struct NKVMString *str = table->stringsByHash[i];

        while(str) {

            while(str && (lastGCPass != str->lastGCPass && !str->dontGC)) {

                uint32_t index = str->stringTableIndex;
                struct NKVMStringTableHole *hole =
                    nkiMalloc(vm, sizeof(struct NKVMStringTableHole));

                // TODO: Remove this.
                dbgWriteLine("Purging unused string: %s", str->str);

                *lastPtr = str->nextInHashBucket;
                table->stringTable[str->stringTableIndex] = NULL;
                nkiFree(vm, str);
                str = *lastPtr;

                // Create a table hole for our new gap.
                memset(hole, 0, sizeof(*hole));
                hole->index = index;
                hole->next = table->tableHoles;
                table->tableHoles = hole;
            }

            // FIXME: Remove this.
            if(str) {
                dbgWriteLine("NOT purging string: %s", str->str);
            }

            if(str) {
                lastPtr = &str->nextInHashBucket;
                str = str->nextInHashBucket;
            }
        }
    }

    dbgPop();
}

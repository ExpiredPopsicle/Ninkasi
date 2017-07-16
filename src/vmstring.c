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

void vmStringTableInit(struct VMStringTable *table)
{
    memset(&table->stringsByHash, 0, sizeof(table->stringsByHash));
    table->tableHoles = NULL;

    // Create a table with a capacity of a single string.
    table->stringTable = malloc(sizeof(struct VMString*));
    table->stringTableCapacity = 1;
    table->stringTable[0] = NULL;

    // Create a "hole" object indicating that we have space in the
    // table.
    table->tableHoles = malloc(sizeof(struct VMStringTableHole));
    table->tableHoles->index = 0;
    table->tableHoles->next = NULL;
}

void vmStringTableDestroy(struct VMStringTable *table)
{
    // Clear out the main table.
    uint32_t i;
    for(i = 0; i < table->stringTableCapacity; i++) {
        free(table->stringTable[i]);
        table->stringTable[i] = NULL;
    }
    free(table->stringTable);
    table->stringTableCapacity = 0;

    // Clear out the holes list.
    {
        struct VMStringTableHole *th = table->tableHoles;
        while(th) {
            struct VMStringTableHole *next = th->next;
            free(th);
            th = next;
        }
        table->tableHoles = NULL;
    }

    // Zero-out the hash table, just for safety.
    memset(table->stringsByHash, 0, sizeof(table->stringsByHash));
}

struct VMString *vmStringTableGetEntryById(
    struct VMStringTable *table, uint32_t index)
{
    if(index >= table->stringTableCapacity) {
        return NULL;
    }

    return table->stringTable[index];
}

const char *vmStringTableGetStringById(
    struct VMStringTable *table,
    uint32_t index)
{
    struct VMString *vmstr = vmStringTableGetEntryById(table, index);
    return vmstr ? vmstr->str : NULL;
}

uint32_t vmStringTableFindOrAddString(
    struct VM *vm,
    const char *str)
{
    struct VMStringTable *table = &vm->stringTable;
    uint32_t hash = stringHash(str);

    // See if we have this string already.
    struct VMString *hashBucket =
        table->stringsByHash[hash & (vmStringTableHashTableSize - 1)];
    struct VMString *cur = hashBucket;

    while(cur) {
        if(!strcmp(cur->str, str)) {
            return cur->stringTableIndex;
        }
        cur = cur->nextInHashBucket;
    }

    // Check our length.
    uint32_t len = strlen(str);
    if(len > vm->limits.maxStringLength) {
        errorStateAddError(
            &vm->errorState,
            -1, "Reached string length limit.");
        return ~(uint32_t)0;
    }

    // If we've reach this point, then we don't have the string yet,
    // so we'll go ahead and make a new entry.
    {
        struct VMString *newString =
            malloc(sizeof(struct VMString) + len + 1);
        uint32_t index = 0;

        if(table->tableHoles) {

            // We can use an existing gap.

            struct VMStringTableHole *hole = table->tableHoles;
            table->tableHoles = hole->next;
            index = hole->index;
            free(hole);

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
                errorStateAddError(
                    &vm->errorState,
                    -1, "Reached string table capacity limit.");
                return ~(uint32_t)0;
            }

            table->stringTable = realloc(
                table->stringTable,
                sizeof(struct VMString *) * newCapacity);

            // Create hole objects for all our empty new space. Not
            // that we don't create one on the border between the old
            // and new space because that's where our new entry will
            // be going.
            for(i = oldCapacity + 1; i < newCapacity; i++) {
                struct VMStringTableHole *hole =
                    malloc(sizeof(struct VMStringTableHole));
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
        table->stringsByHash[hash & (vmStringTableHashTableSize - 1)] = newString;
        table->stringTable[index] = newString;

        return newString->stringTableIndex;
    }
}

void vmStringTableDump(struct VMStringTable *table)
{
    uint32_t i;
    printf("String table dump...\n");

    printf("  Hash table...\n");
    for(i = 0; i < vmStringTableHashTableSize; i++) {
        struct VMString *str = table->stringsByHash[i];
        printf("    %.2x\n", i);
        while(str) {
            printf("      %s\n", str->str);
            str = str->nextInHashBucket;
        }
    }

    printf("  Main table...\n");
    for(i = 0; i < table->stringTableCapacity; i++) {
        struct VMString *str = table->stringTable[i];
        printf("    %.2x\n", i);
        printf("      %s\n", str ? str->str : "<null>");
    }
}

void vmStringTableCleanOldStrings(
    struct VMStringTable *table, uint32_t lastGCPass)
{
    uint32_t i;

    dbgWriteLine("Purging unused strings...");
    dbgPush();

    for(i = 0; i < vmStringTableHashTableSize; i++) {

        struct VMString **lastPtr = &table->stringsByHash[i];
        struct VMString *str = table->stringsByHash[i];

        while(str) {

            while(str && (lastGCPass != str->lastGCPass && !str->dontGC)) {

                uint32_t index = str->stringTableIndex;
                struct VMStringTableHole *hole = malloc(sizeof(struct VMStringTableHole));

                // TODO: Remove this.
                dbgWriteLine("Purging unused string: %s", str->str);

                *lastPtr = str->nextInHashBucket;
                table->stringTable[str->stringTableIndex] = NULL;
                free(str);
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

#ifndef VALUE_H
#define VALUE_H

#include "enums.h"

struct VM;

struct Value
{
    enum NKValueType type;
    uint32_t lastGCPass;

    union
    {
        int32_t intData;
        float floatData;
        uint32_t stringTableEntry;
        uint32_t functionId;
        uint32_t objectId;

        // Used for ints, floats, functionIds, and objectIds.
        uint32_t basicHashValue;
    };
};

bool value_dump(struct VM *vm, struct Value *value);

const char *valueTypeGetName(enum NKValueType type);

int32_t valueToInt(struct VM *vm, struct Value *value);

float valueToFloat(struct VM *vm, struct Value *value);

// Returns a string for a value, possibly converting internally.
// Values are only guaranteed to be valid until the next garbage
// collection pass.
const char *valueToString(struct VM *vm, struct Value *value);

// The return of this value is like strcmp(). -1 for less, 0 for
// equal, 1 for greater-than. Set strictType to true to force a
// comparison failure when types differ. You MUST do this for things
// like binary trees to ensure things have a consistent order.
int32_t value_compare(
    struct VM *vm,
    struct Value *in1,
    struct Value *in2,
    bool strictType);

uint32_t valueHash(struct VM *vm, struct Value *value);

void vmValueSetInt(struct VM *vm, struct Value *value, int32_t intData);
void vmValueSetFloat(struct VM *vm, struct Value *value, float floatData);
void vmValueSetString(struct VM *vm, struct Value *value, const char *str);

#endif

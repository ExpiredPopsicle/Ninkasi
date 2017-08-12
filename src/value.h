#ifndef NINKASI_VALUE_H
#define NINKASI_VALUE_H

#include "nkenums.h"

struct NKVM;

struct NKValue
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

bool value_dump(struct NKVM *vm, struct NKValue *value);

const char *valueTypeGetName(enum NKValueType type);

int32_t valueToInt(struct NKVM *vm, struct NKValue *value);

float valueToFloat(struct NKVM *vm, struct NKValue *value);

// Returns a string for a value, possibly converting internally.
// Values are only guaranteed to be valid until the next garbage
// collection pass.
const char *valueToString(struct NKVM *vm, struct NKValue *value);

// The return of this value is like strcmp(). -1 for less, 0 for
// equal, 1 for greater-than. Set strictType to true to force a
// comparison failure when types differ. You MUST do this for things
// like binary trees to ensure things have a consistent order.
int32_t value_compare(
    struct NKVM *vm,
    struct NKValue *in1,
    struct NKValue *in2,
    bool strictType);

uint32_t valueHash(struct NKVM *vm, struct NKValue *value);

void vmValueSetInt(struct NKVM *vm, struct NKValue *value, int32_t intData);
void vmValueSetFloat(struct NKVM *vm, struct NKValue *value, float floatData);
void vmValueSetString(struct NKVM *vm, struct NKValue *value, const char *str);

#endif // NINKASI_VALUE_H


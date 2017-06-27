#ifndef VALUE_H
#define VALUE_H

#include "enums.h"

struct VM;

struct Value
{
    enum ValueType type;
    uint32_t lastGCPass;

    union
    {
        int32_t intData;
        uint32_t stringTableEntry;
    };
};

bool value_dump(struct VM *vm, struct Value *value);

const char *valueTypeGetName(enum ValueType type);

int32_t valueToInt(struct VM *vm, struct Value *value);

// Returns a string for a value, possibly converting internally.
// Values are only guaranteed to be valid until the next garbage
// collection pass.
const char *valueToString(struct VM *vm, struct Value *value);


#endif

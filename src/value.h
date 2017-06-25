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

bool value_dump(struct Value *value);

const char *valueTypeGetName(enum ValueType type);

int32_t valueToInt(struct VM *vm, struct Value *value);


#endif

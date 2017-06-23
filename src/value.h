#ifndef VALUE_H
#define VALUE_H

#include "enums.h"

struct VM;

struct Value
{
    enum ValueType type;

    union
    {
        int32_t intData;
    };
};

bool value_dump(struct Value *value);

const char *valueTypeGetName(enum ValueType type);

int32_t valueToInt(struct VM *vm, struct Value *value);


#endif

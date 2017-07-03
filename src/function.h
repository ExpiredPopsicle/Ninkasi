#ifndef FUNCTION_H
#define FUNCTION_H

#include "value.h"

struct VMFunctionCallbackData
{
    struct VM *vm;

    struct Value *arguments;
    uint32_t argumentCount;

    // Set this to something to return a value.
    struct Value returnValue;
};

typedef void (*VMFunctionCallback)(struct VMFunctionCallbackData *data);

struct VMFunction
{
    uint32_t argumentCount;
    uint32_t firstInstructionIndex;

    bool isCFunction;
    VMFunctionCallback CFunctionCallback;
};

#endif

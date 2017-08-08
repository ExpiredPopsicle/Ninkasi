#ifndef FUNCTION_H
#define FUNCTION_H

#include "value.h"
#include "public.h"
#include "nkx.h"

struct NKVMFunction
{
    uint32_t argumentCount;
    uint32_t firstInstructionIndex;

    bool isCFunction;
    VMFunctionCallback CFunctionCallback;
    void *CFunctionCallbackUserdata;
};

#endif

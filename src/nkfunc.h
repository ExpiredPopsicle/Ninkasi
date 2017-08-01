#ifndef FUNCTION_H
#define FUNCTION_H

#include "value.h"
#include "public.h"

struct NKVMFunction
{
    uint32_t argumentCount;
    uint32_t firstInstructionIndex;

    bool isCFunction;
    VMFunctionCallback CFunctionCallback;
    void *CFunctionCallbackUserdata;
};

/// Convenience function for C function callbacks. Check the argument
/// count that a function was called with. If it does not match, an
/// error will be added and this function will return false. Otherwise
/// it will return true.
bool vmFunctionCallbackCheckArgCount(
    struct NKVMFunctionCallbackData *data,
    uint32_t argCount,
    const char *functionName);

#endif

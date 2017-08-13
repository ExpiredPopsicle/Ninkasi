#ifndef NINKASI_FUNCTION_H
#define NINKASI_FUNCTION_H

#include "value.h"
#include "nkx.h"

struct NKVMFunction
{
    nkuint32_t argumentCount;
    nkuint32_t firstInstructionIndex;

    nkbool isCFunction;
    VMFunctionCallback CFunctionCallback;
    void *CFunctionCallbackUserdata;
};

#endif // NINKASI_FUNCTION_H


#include "common.h"

bool vmFunctionCallbackCheckArgCount(
    struct VMFunctionCallbackData *data,
    uint32_t argCount,
    const char *functionName)
{
    if(data->argumentCount != 1) {
        struct DynString *dynStr = dynStrCreate(
            data->vm, "Bad argument count in ");
        dynStrAppend(dynStr, functionName);
        errorStateAddError(&data->vm->errorState, -1, dynStr->data);
        dynStrDelete(dynStr);
        return false;
    }
    return true;
}


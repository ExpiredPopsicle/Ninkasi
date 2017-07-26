#include "common.h"

bool vmFunctionCallbackCheckArgCount(
    struct VMFunctionCallbackData *data,
    uint32_t argCount,
    const char *functionName)
{
    if(data->argumentCount != 1) {
        struct NKDynString *dynStr = dynStrCreate(
            data->vm, "Bad argument count in ");
        dynStrAppend(dynStr, functionName);
        nkiAddError(
            data->vm, -1, dynStr->data);
        dynStrDelete(dynStr);
        return false;
    }
    return true;
}


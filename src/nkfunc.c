#include "common.h"

bool vmFunctionCallbackCheckArgCount(
    struct VMFunctionCallbackData *data,
    uint32_t argCount,
    const char *functionName)
{
    if(data->argumentCount != 1) {
        struct NKDynString *dynStr = nkiDynStrCreate(
            data->vm, "Bad argument count in ");
        nkiDynStrAppend(dynStr, functionName);
        nkiAddError(
            data->vm, -1, dynStr->data);
        nkiDynStrDelete(dynStr);
        return false;
    }
    return true;
}


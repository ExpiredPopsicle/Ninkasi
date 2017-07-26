#include "common.h"

struct NKDynString *nkiDynStrCreate(struct VM *vm, const char *str)
{
    struct NKDynString *ret = nkMalloc(vm, sizeof(struct NKDynString) + 1);
    ret->vm = vm;
    ret->data = nkStrdup(vm, str ? str : "<null>");
    return ret;
}

void nkiDynStrDelete(struct NKDynString *dynStr)
{
    nkFree(dynStr->vm, dynStr->data);
    nkFree(dynStr->vm, dynStr);
}

void dynStrAppend(struct NKDynString *dynStr, const char *str)
{
    if(!str) {
        str = "<null>";
    }

    dynStr->data = nkRealloc(
        dynStr->vm,
        dynStr->data,
        strlen(dynStr->data) + strlen(str) + 1);

    strcat(dynStr->data, str);
}

void dynStrAppendInt32(struct NKDynString *dynStr, int32_t value)
{
    // +1 for terminator, +1 for '-'.
    char tmp[sizeof(int32_t) * 8 + 2];
    sprintf(tmp, "%d", value);
    dynStrAppend(dynStr, tmp);
}

void dynStrAppendFloat(struct NKDynString *dynStr, float value)
{
    // +1 for terminator, +1 for '-', +1 for '.'.
    // +a lot because float.
    char tmp[sizeof(float) * 16 + 3];
    sprintf(tmp, "%f", value);
    dynStrAppend(dynStr, tmp);
}

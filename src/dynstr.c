#include "common.h"

struct NKDynString *nkiDynStrCreate(struct NKVM *vm, const char *str)
{
    struct NKDynString *ret = nkiMalloc(vm, sizeof(struct NKDynString) + 1);
    ret->vm = vm;
    ret->data = nkiStrdup(vm, str ? str : "<null>");
    return ret;
}

void nkiDynStrDelete(struct NKDynString *dynStr)
{
    nkiFree(dynStr->vm, dynStr->data);
    nkiFree(dynStr->vm, dynStr);
}

void nkiDynStrAppend(struct NKDynString *dynStr, const char *str)
{
    if(!str) {
        str = "<null>";
    }

    dynStr->data = nkiRealloc(
        dynStr->vm,
        dynStr->data,
        strlen(dynStr->data) + strlen(str) + 1);

    strcat(dynStr->data, str);
}

void nkiDynStrAppendInt32(struct NKDynString *dynStr, int32_t value)
{
    // +1 for terminator, +1 for '-'.
    char tmp[sizeof(int32_t) * 8 + 2];
    sprintf(tmp, "%d", value);
    nkiDynStrAppend(dynStr, tmp);
}

void nkiDynStrAppendFloat(struct NKDynString *dynStr, float value)
{
    // +1 for terminator, +1 for '-', +1 for '.'.
    // +a lot because float.
    char tmp[sizeof(float) * 16 + 3];
    sprintf(tmp, "%f", value);
    nkiDynStrAppend(dynStr, tmp);
}

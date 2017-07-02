#include "common.h"

struct DynString *dynStrCreate(const char *str)
{
    struct DynString *ret = malloc(sizeof(struct DynString) + 1);
    ret->data = strdup(str);
    return ret;
}

void dynStrDelete(struct DynString *dynStr)
{
    free(dynStr->data);
    free(dynStr);
}

void dynStrAppend(struct DynString *dynStr, const char *str)
{
    if(!str) {
        str = "<null>";
    }

    dynStr->data = realloc(
        dynStr->data,
        strlen(dynStr->data) + strlen(str) + 1);

    strcat(dynStr->data, str);
}

void dynStrAppendInt32(struct DynString *dynStr, int32_t value)
{
    // +1 for terminator, +1 for '-'.
    char tmp[sizeof(int32_t) * 8 + 2];
    sprintf(tmp, "%d", value);
    dynStrAppend(dynStr, tmp);
}

void dynStrAppendFloat(struct DynString *dynStr, float value)
{
    // +1 for terminator, +1 for '-', +1 for '.'.
    // +a lot because float.
    char tmp[sizeof(float) * 16 + 3];
    sprintf(tmp, "%f", value);
    dynStrAppend(dynStr, tmp);
}

#ifndef DYNSTR_H
#define DYNSTR_H

#include "basetype.h"

struct DynString
{
    char *data;
    struct VM *vm;
};

struct DynString *dynStrCreate(struct VM *vm, const char *str);
void dynStrDelete(struct DynString *dynStr);

void dynStrAppend(struct DynString *dynStr, const char *str);
void dynStrAppendInt32(struct DynString *dynStr, int32_t value);
void dynStrAppendFloat(struct DynString *dynStr, float value);

#endif

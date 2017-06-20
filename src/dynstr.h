#ifndef DYNSTR_H
#define DYNSTR_H

#include "basetype.h"

struct DynString
{
    char *data;
};

struct DynString *dynStrCreate(const char *str);
void dynStrDelete(struct DynString *dynStr);

void dynStrAppend(struct DynString *dynStr, const char *str);
void dynStrAppendInt32(struct DynString *dynStr, int32_t value);

#endif

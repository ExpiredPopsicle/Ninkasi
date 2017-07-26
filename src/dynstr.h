#ifndef NINKASI_DYNSTR_H
#define NINKASI_DYNSTR_H

#include "basetype.h"

struct NKDynString
{
    char *data;
    struct VM *vm;
};

struct NKDynString *nkiDynStrCreate(struct VM *vm, const char *str);
void nkiDynStrDelete(struct NKDynString *dynStr);

void dynStrAppend(struct NKDynString *dynStr, const char *str);
void dynStrAppendInt32(struct NKDynString *dynStr, int32_t value);
void dynStrAppendFloat(struct NKDynString *dynStr, float value);

#endif // NINKASI_DYNSTR_H


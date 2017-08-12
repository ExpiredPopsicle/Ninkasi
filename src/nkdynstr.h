#ifndef NINKASI_DYNSTR_H
#define NINKASI_DYNSTR_H

#include "basetype.h"

struct NKDynString
{
    char *data;
    struct NKVM *vm;
};

struct NKDynString *nkiDynStrCreate(struct NKVM *vm, const char *str);
void nkiDynStrDelete(struct NKDynString *dynStr);

void nkiDynStrAppend(struct NKDynString *dynStr, const char *str);
void nkiDynStrAppendInt32(struct NKDynString *dynStr, nkint32_t value);
void nkiDynStrAppendFloat(struct NKDynString *dynStr, float value);

#endif // NINKASI_DYNSTR_H


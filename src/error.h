#ifndef ERROR_H
#define ERROR_H

#include "basetype.h"

struct Error
{
    char *errorText;
    struct Error *next;
};

struct ErrorState
{
    struct Error *firstError;
    struct Error *lastError;
};

void errorStateInit(struct ErrorState *es);
void errorStateDestroy(struct ErrorState *es);

void errorStateAddError(
    struct ErrorState *es,
    int32_t lineNumber,
    const char *str);

bool errorStateHasErrors(const struct ErrorState *es);

#endif

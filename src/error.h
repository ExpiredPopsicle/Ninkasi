#ifndef NINKASI_ERROR_H
#define NINKASI_ERROR_H

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

    // Allocation failures are handled separately, as their own flag,
    // because if we're in a situation where an allocation has failed,
    // we might not be in a situation where we can allocate memory for
    // any more error messages at all.
    bool allocationFailure;
};

void errorStateInit(struct ErrorState *es);
void errorStateDestroy(struct ErrorState *es);

void errorStateAddError(
    struct ErrorState *es,
    int32_t lineNumber,
    const char *str);

void errorStateSetAllocationFailFlag(
    struct ErrorState *es);

bool errorStateHasErrors(const struct ErrorState *es);

#endif

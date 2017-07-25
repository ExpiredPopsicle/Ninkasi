#ifndef NINKASI_ERROR_H
#define NINKASI_ERROR_H

#include "basetype.h"

struct VM;

struct NKError
{
    char *errorText;
    struct NKError *next;
};

struct NKErrorState
{
    struct NKError *firstError;
    struct NKError *lastError;

    // Allocation failures are handled separately, as their own flag,
    // because if we're in a situation where an allocation has failed,
    // we might not be in a situation where we can allocate memory for
    // any more error messages at all.
    bool allocationFailure;
};

void nkiErrorStateInit(struct VM *vm);
void nkiErrorStateDestroy(struct VM *vm);

void nkiAddError(
    struct VM *vm,
    int32_t lineNumber,
    const char *str);

void nkiErrorStateSetAllocationFailFlag(
    struct VM *vm);

/// Get whether or not an error has occurred (faster than
/// vmGetErrorCount).
bool nkiVmHasErrors(struct VM *vm);

#endif

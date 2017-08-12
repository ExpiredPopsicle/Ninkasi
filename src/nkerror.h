#ifndef NINKASI_ERROR_H
#define NINKASI_ERROR_H

#include "nktypes.h"

struct NKVM;

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
    nkbool allocationFailure;
};

void nkiErrorStateInit(struct NKVM *vm);
void nkiErrorStateDestroy(struct NKVM *vm);

void nkiAddError(
    struct NKVM *vm,
    nkint32_t lineNumber,
    const char *str);

void nkiErrorStateSetAllocationFailFlag(
    struct NKVM *vm);

/// Get whether or not an error has occurred (faster than
/// vmGetErrorCount).
nkbool nkiVmHasErrors(struct NKVM *vm);

#endif

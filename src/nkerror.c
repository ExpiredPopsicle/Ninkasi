#include "common.h"

void nkiErrorStateSetAllocationFailFlag(
    struct NKVM *vm)
{
    vm->errorState.allocationFailure = nktrue;
}

void nkiErrorStateInit(struct NKVM *vm)
{
    vm->errorState.firstError = NULL;
    vm->errorState.lastError = NULL;
    vm->errorState.allocationFailure = nkfalse;
}

void nkiErrorStateDestroy(struct NKVM *vm)
{
    struct NKError *e = vm->errorState.firstError;
    while(e) {
        struct NKError *next = e->next;
        nkiFree(vm, e->errorText);
        nkiFree(vm, e);
        e = next;
    }
    vm->errorState.firstError = NULL;
}

nkbool nkiVmHasErrors(struct NKVM *vm)
{
    return vm->errorState.firstError || vm->errorState.allocationFailure;
}

void nkiAddError(
    struct NKVM *vm,
    nkint32_t lineNumber,
    const char *str)
{
    struct NKError *newError = nkiMalloc(
        vm,
        sizeof(struct NKError));

    newError->errorText =
        nkiMalloc(vm, strlen(str) + 2 + sizeof(lineNumber) * 8 + 1);

    newError->errorText[0] = 0;
    sprintf(newError->errorText, "%d: %s", lineNumber, str);

    newError->next = NULL;

    // Add error to the error list.
    if(vm->errorState.lastError) {
        vm->errorState.lastError->next = newError;
    }
    vm->errorState.lastError = newError;
    if(!vm->errorState.firstError) {
        vm->errorState.firstError = newError;
    }
}

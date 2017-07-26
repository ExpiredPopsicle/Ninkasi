#include "common.h"

void nkiErrorStateSetAllocationFailFlag(
    struct VM *vm)
{
    vm->errorState.allocationFailure = true;
}

void nkiErrorStateInit(struct VM *vm)
{
    vm->errorState.firstError = NULL;
    vm->errorState.lastError = NULL;
    vm->errorState.allocationFailure = false;
}

void nkiErrorStateDestroy(struct VM *vm)
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

bool nkiVmHasErrors(struct VM *vm)
{
    return vm->errorState.firstError || vm->errorState.allocationFailure;
}

void nkiAddError(
    struct VM *vm,
    int32_t lineNumber,
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

#include "common.h"

void vmInit(struct VM *vm)
{
    errorStateInit(&vm->errorState);
    vmStackInit(&vm->stack);
}

void vmDestroy(struct VM *vm)
{
    vmStackDestroy(&vm->stack);
    errorStateDestroy(&vm->errorState);
}

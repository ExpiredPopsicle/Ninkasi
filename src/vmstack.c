#include "common.h"

void vmStackInit(struct NKVM *vm)
{
    struct NKVMStack *stack = &vm->stack;
    stack->values = nkiMalloc(vm, sizeof(struct NKValue));
    memset(stack->values, 0, sizeof(struct NKValue));
    stack->size = 0;
    stack->capacity = 1;
    stack->indexMask = 0;
}

void vmStackDestroy(struct NKVM *vm)
{
    struct NKVMStack *stack = &vm->stack;
    nkiFree(vm, stack->values);
    memset(stack, 0, sizeof(struct NKVMStack));
}

struct NKValue *vmStackPush_internal(struct NKVM *vm)
{
    struct NKVMStack *stack = &vm->stack;

    // Grow the stack if necessary.
    if(stack->size == stack->capacity) {

        stack->capacity <<= 1;

        // TODO: Add an adjustable stack size limit.
        if(stack->capacity > 0xffff) {

            nkiAddError(
                vm, -1,
                "Stack overflow.");

            return &stack->values[0];
        }

        // TODO: Make a more reasonable stack space limit than "we ran
        // out of 32-bit integer".
        // TODO: Make this a normal error. (Return NULL?)
        assert(stack->capacity);
        if(!stack->capacity) {
            nkiAddError(
                vm, -1,
                "Stack ran out of address space.");
            return &stack->values[0];
        }

        if(stack->capacity > vm->limits.maxStacksize) {
            nkiAddError(
                vm, -1,
                "Reached stack capacity limit.");
            return &stack->values[0];
        }

        stack->indexMask <<= 1;
        stack->indexMask |= 1;
        stack->values = nkiRealloc(
            vm,
            stack->values,
            stack->capacity * sizeof(struct NKValue));

        memset(
            &stack->values[stack->size], 0,
            sizeof(struct NKValue) * (stack->capacity - stack->size));
    }

    {
        struct NKValue *ret = &stack->values[stack->size];
        stack->size++;
        return ret;
    }
}

nkbool vmStackPushInt(struct NKVM *vm, nkint32_t value)
{
    struct NKValue *data = vmStackPush_internal(vm);
    if(data) {
        data->type = NK_VALUETYPE_INT;
        data->intData = value;
        return nktrue;
    }
    return nkfalse;
}

nkbool vmStackPushFloat(struct NKVM *vm, float value)
{
    struct NKValue *data = vmStackPush_internal(vm);
    if(data) {
        data->type = NK_VALUETYPE_FLOAT;
        data->floatData = value;
        return nktrue;
    }
    return nkfalse;
}

nkbool vmStackPushString(struct NKVM *vm, const char *str)
{
    struct NKValue *data = vmStackPush_internal(vm);
    if(data) {
        data->type = NK_VALUETYPE_STRING;
        data->stringTableEntry =
            nkiVmStringTableFindOrAddString(
                vm, str);
        return nktrue;
    }
    return nkfalse;
}

struct NKValue *vmStackPop(struct NKVM *vm)
{
    struct NKVMStack *stack = &vm->stack;

    // TODO: Shrink the stack if we can?

    if(stack->size == 0) {
        // Stack underflow. We'll return the bottom of the stack, just
        // so that whatever is expecting a valid piece of data here
        // won't explode, but the error will be visible next check.
        nkiAddError(vm, -1, "Stack underflow in pop.");
        return &stack->values[0];
    }

    stack->size--;
    return &stack->values[stack->size];
}

void vmStackPopN(struct NKVM *vm, nkuint32_t count)
{
    struct NKVMStack *stack = &vm->stack;

    // TODO: Shrink the stack if we can?

    dbgWriteLine("pop count: %u", count);

    if(stack->size < count) {
        // Stack underflow. We'll return the bottom of the stack, just
        // so that whatever is expecting a valid piece of data here
        // won't explode, but the error will be visible next check.
        nkiAddError(vm, -1, "Stack underflow in popN.");
        stack->size = 0;
        return;
    }

    stack->size -= count;
}

void vmStackDump(struct NKVM *vm)
{
    nkuint32_t i;
    struct NKVMStack *stack = &vm->stack;
    for(i = 0; i < stack->size; i++) {
        printf("%3d: ", i);
        value_dump(vm, vmStackPeek(vm, i));
        printf("\n");
    }
}

struct NKValue *vmStackPeek(struct NKVM *vm, nkuint32_t index)
{
    return &vm->stack.values[index & vm->stack.indexMask];
}


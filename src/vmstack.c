#include "common.h"

void vmStackInit(struct VMStack *stack)
{
    stack->values = calloc(1, sizeof(struct Value));
    stack->size = 0;
    stack->capacity = 1;
    stack->indexMask = 0;
}

void vmStackDestroy(struct VMStack *stack)
{
    free(stack->values);
    memset(stack, 0, sizeof(struct VMStack));
}

struct Value *vmStackPush_internal(struct VMStack *stack)
{
    // Grow the stack if necessary.
    if(stack->size == stack->capacity) {

        stack->capacity <<= 1;

        // TODO: Make a more reasonable stack space limit than "we ran
        // out of 32-bit integer".
        // TODO: Make this a normal error. (Return NULL?)
        assert(stack->capacity);
        if(!stack->capacity) {
            return NULL;
        }

        stack->indexMask <<= 1;
        stack->indexMask |= 1;
        stack->values = realloc(
            stack->values,
            stack->capacity * sizeof(struct Value));
    }

    {
        struct Value *ret = &stack->values[stack->size];
        stack->size++;
        return ret;
    }
}

bool vmStackPushInt(struct VMStack *stack, int32_t value)
{
    struct Value *data = vmStackPush_internal(stack);
    if(data) {
        data->type = VALUETYPE_INT;
        data->intData = value;
        return true;
    }
    return false;
}

bool vmStackPushString(struct VM *vm, const char *str)
{
    struct VMStack *stack = &vm->stack;
    struct Value *data = vmStackPush_internal(stack);
    if(data) {
        data->type = VALUETYPE_STRING;
        data->stringTableEntry =
            vmStringTableFindOrAddString(
                &vm->stringTable, str);
        return true;
    }
    return false;
}

struct Value *vmStackPop(struct VMStack *stack)
{
    // TODO: Shrink the stack if we can?

    // TODO: Make this a normal error. (Return NULL?)
    assert(stack->size);
    if(stack->size == 0) {
        return NULL;
    }

    stack->size--;
    return &stack->values[stack->size];
}

void vmStackDump(struct VM *vm)
{
    uint32_t i;
    struct VMStack *stack = &vm->stack;
    for(i = 0; i < stack->size; i++) {
        printf("%3d: ", i);
        value_dump(vm, vmStackPeek(stack, i));
        printf("\n");
    }
}

struct Value *vmStackPeek(struct VMStack *stack, uint32_t index)
{
    return &stack->values[index & stack->indexMask];
}


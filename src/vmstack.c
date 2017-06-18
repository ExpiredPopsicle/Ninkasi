#include "common.h"

void vmstack_init(struct VMStack *stack)
{
    stack->values = calloc(1, sizeof(struct Value));
    stack->size = 0;
    stack->capacity = 1;
    stack->indexMask = 0;
}

void vmstack_destroy(struct VMStack *stack)
{
    free(stack->values);
    memset(stack, 0, sizeof(struct VMStack));
}

struct Value *vmstack_push_internal(struct VMStack *stack)
{
    if(stack->size == stack->capacity) {

        stack->capacity <<= 1;

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

    struct Value *ret = &stack->values[stack->size];
    stack->size++;
    return ret;
}

bool vmstack_pushInt(struct VMStack *stack, int32_t value)
{
    struct Value *data = vmstack_push_internal(stack);
    if(data) {
        data->type = VALUETYPE_INT;
        data->intData = value;
        return true;
    }
    return false;
}

struct Value *vmstack_pop(struct VMStack *stack)
{
    // TODO: Make this a normal error. (Return NULL?)
    assert(stack->size);
    if(stack->size == 0) {
        return NULL;
    }

    stack->size--;
    return &stack->values[stack->size];
}

void vmstack_dump(struct VMStack *stack)
{
    uint32_t i;
    for(i = 0; i < stack->size; i++) {
        printf("%3d: ", i);
        value_dump(vmstack_peek(stack, i));
        printf("\n");
    }
}

struct Value *vmstack_peek(struct VMStack *stack, uint32_t index)
{
    return &stack->values[index & stack->indexMask];
}


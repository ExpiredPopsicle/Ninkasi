#include "common.h"

bool opcode_add(struct VMStack *stack)
{
    struct Value *in1  = vmstack_pop(stack);
    struct Value *in2  = vmstack_pop(stack);
    // struct Value *out1 = vmstack_push_internal(stack);

    enum ValueType type = in1->type;

    if(type != in2->type) {
        // TODO: Make normal error.
        assert(0);
        return false;
    }

    // TODO: Function pointer table here?
    switch(type) {
        case VALUETYPE_INT:
            vmstack_pushInt(
                stack,
                in2->intData +
                in1->intData);
            break;
        default:
            // TODO: Normal error.
            assert(0);
            return false;
    }
    return true;
}


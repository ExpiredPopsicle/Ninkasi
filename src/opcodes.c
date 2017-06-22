#include "common.h"

void opcode_add(struct VM *vm, struct Instruction *instruction)
{
    struct VMStack *stack = &vm->stack;
    struct Value *in1  = vmStackPop(stack);
    struct Value *in2  = vmStackPop(stack);
    // struct Value *out1 = vmstack_push_internal(stack);

    enum ValueType type = in1->type;

    if(type != in2->type) {
        // TODO: Make normal error.
        assert(0);
        return;
    }

    // TODO: Function pointer table here?
    switch(type) {
        case VALUETYPE_INT:
            vmStackPushInt(
                stack,
                in2->intData +
                in1->intData);
            break;
        default:
            // TODO: Normal error.
            assert(0);
            return;
    }
}

void opcode_pushLiteral(struct VM *vm, struct Instruction *instruction)
{
    struct Value *stackVal = vmStackPush_internal(&vm->stack);
    *stackVal = instruction->pushLiteralData.value;
}

void opcode_nop(struct VM *vm, struct Instruction *instruction)
{
}


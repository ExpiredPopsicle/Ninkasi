#include "common.h"

void opcode_add(struct VM *vm, struct Instruction *instruction)
{
    struct VMStack *stack = &vm->stack;
    struct Value *in2  = vmStackPop(stack);
    struct Value *in1  = vmStackPop(stack);

    enum ValueType type = in1->type;

    switch(type) {

        case VALUETYPE_INT:
            vmStackPushInt(
                stack,
                in1->intData +
                valueToInt(vm, in2));
            break;

            // TODO: Float support.

        case VALUETYPE_STRING: {
            struct DynString *dynStr =
                dynStrCreate(
                    vmStringTableGetStringById(
                        &vm->stringTable,
                        in1->stringTableEntry));
            dynStrAppend(dynStr, valueToString(vm, in2));
            vmStackPushString(
                vm, dynStr->data);
            dynStrDelete(dynStr);
        } break;

            // TODO: String concatenation support.

            // TODO: Array concatenation support.

        default: {
            struct DynString *ds =
                dynStrCreate("Addition unimplemented for type ");
            dynStrAppend(ds, valueTypeGetName(type));
            dynStrAppend(ds, ".");
            errorStateAddError(
                &vm->errorState, -1,
                ds->data);
            dynStrDelete(ds);
            return;
        }
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

void opcode_subtract(struct VM *vm, struct Instruction *instruction)
{
    struct VMStack *stack = &vm->stack;
    struct Value *in2 = vmStackPop(stack);
    struct Value *in1 = vmStackPop(stack);

    enum ValueType type = in1->type;

    switch(type) {

        case VALUETYPE_INT:
            vmStackPushInt(
                stack,
                in1->intData -
                valueToInt(vm, in2));
            break;

            // TODO: Float support.

        default: {
            struct DynString *ds =
                dynStrCreate("Subtraction unimplemented for type ");
            dynStrAppend(ds, valueTypeGetName(type));
            dynStrAppend(ds, ".");
            errorStateAddError(
                &vm->errorState, -1,
                ds->data);
            dynStrDelete(ds);
        } break;
    }
}

void opcode_multiply(struct VM *vm, struct Instruction *instruction)
{
    struct VMStack *stack = &vm->stack;
    struct Value *in2 = vmStackPop(stack);
    struct Value *in1 = vmStackPop(stack);

    enum ValueType type = in1->type;

    switch(type) {

        case VALUETYPE_INT:
            vmStackPushInt(
                stack,
                in1->intData *
                valueToInt(vm, in2));
            break;

            // TODO: Float support.

        default: {
            struct DynString *ds =
                dynStrCreate("Multiplication unimplemented for type ");
            dynStrAppend(ds, valueTypeGetName(type));
            dynStrAppend(ds, ".");
            errorStateAddError(
                &vm->errorState, -1,
                ds->data);
            dynStrDelete(ds);
        } break;
    }
}

void opcode_divide(struct VM *vm, struct Instruction *instruction)
{
    struct VMStack *stack = &vm->stack;
    struct Value *in2 = vmStackPop(stack);
    struct Value *in1 = vmStackPop(stack);

    enum ValueType type = in1->type;

    switch(type) {

        case VALUETYPE_INT: {
            int32_t val2 = valueToInt(vm, in2);
            if(val2 == 0) {
                errorStateAddError(
                    &vm->errorState, -1,
                    "Integer divide-by-zero.");
            } else {
                vmStackPushInt(
                    stack,
                    in1->intData /
                    val2);
            }
        } break;

            // TODO: Float support.

        default: {
            struct DynString *ds =
                dynStrCreate("Division unimplemented for type ");
            dynStrAppend(ds, valueTypeGetName(type));
            dynStrAppend(ds, ".");
            errorStateAddError(
                &vm->errorState, -1,
                ds->data);
            dynStrDelete(ds);
        } break;
    }
}

void opcode_negate(struct VM *vm, struct Instruction *instruction)
{
    struct VMStack *stack = &vm->stack;
    struct Value *in1 = vmStackPop(stack);

    enum ValueType type = in1->type;

    switch(type) {

        case VALUETYPE_INT: {
            vmStackPushInt(
                stack,
                -(in1->intData));
        } break;

            // TODO: Float support.

        default: {
            struct DynString *ds =
                dynStrCreate("Negation unimplemented for type ");
            dynStrAppend(ds, valueTypeGetName(type));
            dynStrAppend(ds, ".");
            errorStateAddError(
                &vm->errorState, -1,
                ds->data);
            dynStrDelete(ds);
        } break;
    }
}


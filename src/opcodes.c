#include "common.h"

void opcode_add(struct VM *vm, struct Instruction *instruction)
{
    struct Value *in2  = vmStackPop(vm);
    struct Value *in1  = vmStackPop(vm);

    enum ValueType type = in1->type;

    switch(type) {

        case VALUETYPE_INT:
            vmStackPushInt(
                vm,
                in1->intData +
                valueToInt(vm, in2));
            break;

        case VALUETYPE_FLOAT:
            vmStackPushFloat(
                vm,
                in1->floatData +
                valueToFloat(vm, in2));
            break;

        case VALUETYPE_STRING: {

            // Make a new string that is the concatenated values.
            // Start with a DynString of the first one.
            struct DynString *dynStr =
                dynStrCreate(
                    vmStringTableGetStringById(
                        &vm->stringTable,
                        in1->stringTableEntry));

            // Append the other one, after conversion to string if
            // necessary.
            dynStrAppend(dynStr, valueToString(vm, in2));

            // Push the result.
            vmStackPushString(
                vm, dynStr->data);

            // Clean up.
            dynStrDelete(dynStr);

        } break;

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

void opcode_pushLiteral_int(struct VM *vm, struct Instruction *instruction)
{
    struct Value *stackVal = vmStackPush_internal(vm);
    vm->instructionPointer++;

    stackVal->type = VALUETYPE_INT;
    stackVal->intData =
        vm->instructions[vm->instructionPointer & vm->instructionAddressMask].opData_int;
}

void opcode_pushLiteral_float(struct VM *vm, struct Instruction *instruction)
{
    struct Value *stackVal = vmStackPush_internal(vm);
    vm->instructionPointer++;

    stackVal->type = VALUETYPE_FLOAT;
    stackVal->floatData =
        vm->instructions[vm->instructionPointer & vm->instructionAddressMask].opData_float;
}

void opcode_pushLiteral_string(struct VM *vm, struct Instruction *instruction)
{
    struct Value *stackVal = vmStackPush_internal(vm);
    vm->instructionPointer++;

    stackVal->type = VALUETYPE_STRING;
    stackVal->stringTableEntry =
        vm->instructions[vm->instructionPointer & vm->instructionAddressMask].opData_string;

    printf("Pushing literal string: %d = %d\n", vm->instructionPointer, stackVal->stringTableEntry);

}

void opcode_nop(struct VM *vm, struct Instruction *instruction)
{
}

void opcode_subtract(struct VM *vm, struct Instruction *instruction)
{
    struct Value *in2 = vmStackPop(vm);
    struct Value *in1 = vmStackPop(vm);

    enum ValueType type = in1->type;

    switch(type) {

        case VALUETYPE_INT:
            vmStackPushInt(
                vm,
                in1->intData -
                valueToInt(vm, in2));
            break;

        case VALUETYPE_FLOAT:
            vmStackPushFloat(
                vm,
                in1->floatData -
                valueToFloat(vm, in2));
            break;

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
    struct Value *in2 = vmStackPop(vm);
    struct Value *in1 = vmStackPop(vm);

    enum ValueType type = in1->type;

    switch(type) {

        case VALUETYPE_INT:
            vmStackPushInt(
                vm,
                in1->intData *
                valueToInt(vm, in2));
            break;

        case VALUETYPE_FLOAT:
            vmStackPushFloat(
                vm,
                in1->floatData *
                valueToFloat(vm, in2));
            break;

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
    struct Value *in2 = vmStackPop(vm);
    struct Value *in1 = vmStackPop(vm);

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
                    vm,
                    in1->intData /
                    val2);
            }
        } break;

        case VALUETYPE_FLOAT:
            vmStackPushFloat(
                vm,
                in1->floatData /
                valueToFloat(vm, in2));
            break;

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
    struct Value *in1 = vmStackPop(vm);

    enum ValueType type = in1->type;

    switch(type) {

        case VALUETYPE_INT: {
            vmStackPushInt(
                vm,
                -(in1->intData));
        } break;

        case VALUETYPE_FLOAT: {
            vmStackPushFloat(
                vm,
                -(in1->floatData));
        } break;

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

void opcode_pop(struct VM *vm, struct Instruction *instruction)
{
    vmStackPop(vm);
}

void opcode_dump(struct VM *vm, struct Instruction *instruction)
{
    struct Value *v = vmStackPop(vm);
    printf("Debug dump: ");
    value_dump(vm, v);
    printf("\n");
}

void opcode_stackPeek(struct VM *vm, struct Instruction *instruction)
{
    // Read index.
    struct Value *v = vmStackPop(vm);
    if(v->type != VALUETYPE_INT) {
        errorStateAddError(&vm->errorState, -1,
            "Attempted to use a non-integer as a stack index.");
        return;
    }

    // Copy stack data over.
    if(v->intData >= 0) {

        // Absolute stack address. Probably a global variable.
        uint32_t stackAddress = v->intData;
        struct Value *vIn = vmStackPeek(vm, v->intData);
        struct Value *vOut = vmStackPush_internal(vm);
        *vOut = *vIn;

        printf("Fetched global value at stack position: %u\n", stackAddress);

    } else {

        printf("Fetching local value...\n");
        printf("  Stack size: %u\n", vm->stack.size);
        printf("  v->intData: %d\n", v->intData);

        {
            // Negative stack address. Probably a local variable.
            uint32_t stackAddress = vm->stack.size + v->intData;
            struct Value *vIn = vmStackPeek(
                vm, stackAddress);
            struct Value *vOut = vmStackPush_internal(vm);
            *vOut = *vIn;

            printf("Fetched local value at stack position: %u\n", stackAddress);
        }

    }
}

void opcode_stackPoke(struct VM *vm, struct Instruction *instruction)
{
    // Read index.
    struct Value *stackAddrValue = vmStackPop(vm);
    if(stackAddrValue->type != VALUETYPE_INT) {
        errorStateAddError(&vm->errorState, -1,
            "Attempted to use a non-integer as a stack index.");
        return;
    }

    // Copy to stack.
    if(stackAddrValue->intData >= 0) {

        // Absolute stack address. Probably a global variable.
        uint32_t stackAddress = stackAddrValue->intData;
        struct Value *vIn = vmStackPeek(vm, (vm->stack.size - 1));
        struct Value *vOut = vmStackPeek(vm, stackAddrValue->intData);
        *vOut = *vIn;

        printf("Set global value at stack position: %u\n", stackAddress);

    } else {

        printf("Setting local value...\n");
        printf("  Stack size: %u\n", vm->stack.size);
        printf("  v->intData: %d\n", stackAddrValue->intData);

        {
            // Negative stack address. Probably a local variable.
            uint32_t stackAddress = vm->stack.size + stackAddrValue->intData;
            struct Value *vIn = vmStackPeek(vm, (vm->stack.size - 1));
            struct Value *vOut = vmStackPeek(vm, stackAddress);
            *vOut = *vIn;

            printf("Set local value at stack position: %u\n", stackAddress);
        }

    }
}

void opcode_jumpRelative(struct VM *vm, struct Instruction *instruction)
{
    struct Value *offsetVal = vmStackPop(vm);
    vm->instructionPointer += valueToInt(vm, offsetVal);
}

void opcode_call(struct VM *vm, struct Instruction *instruction)
{
    // Expected stack state at start...
    //   function id
    //   _argumentCount
    //   <_argumentCount number of arguments>

    // TODO: Pop the top of the stack and save it. That's the function
    // id.

    // TODO: Look up the function in our not-yet-existing table of
    // function objects.

    // TODO: PEEK at the top of the stack. That's _argumentCount.

    // TODO: Compare _argumentCount to the stored function object's
    // _argumentCount. Throw an error if they mismatch.

    // TODO: Push the current instruction pointer (_returnPointer).

    // TODO: Set the instruction pointer to the saved function
    // object's instruction pointer. Maybe minus one.

    // Expected stack state at end...
    //   _returnPointer
    //   _argumentCount
    //   <_argumentCount number of arguments>
}

void opcode_return(struct VM *vm, struct Instruction *instruction)
{
    // Expected stack state at start...
    //   _returnValue
    //   context amount to throw away (contextCount) (generate from cs->context->stackFrameOffset - functionArgumentCount?)
    //   <contextCount number of slots>
    //   _returnPointer (from CALL)
    //   _argumentCount (from before CALL)
    //   <_argumentCount number of arguments> (from before CALL)

    // TODO: Pop a value off the stack. That's _returnValue.

    // TODO: Pop a value, contextCount, off the stack.

    // TODO: Pop contextCount more values off the stack and throw them
    // away. (Function context we're done with.)

    // TODO: Pop the _returnPointer off the stack and keep it.

    // TODO: Pop the _argumentCount off the stack and keep it.

    // TODO: Pop _argumentCount values off the stack and throw them
    // away. (Function arguments we're done with.)

    // TODO: Push _returnValue back onto the stack.

    // TODO: Set the instruction pointer to _returnPointer - 1.

    // Expected stack state at end...
    //   _returnValue
}


#include "common.h"

void opcode_add(struct VM *vm)
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

void opcode_pushLiteral_int(struct VM *vm)
{
    struct Value *stackVal = vmStackPush_internal(vm);
    vm->instructionPointer++;

    stackVal->type = VALUETYPE_INT;
    stackVal->intData =
        vm->instructions[vm->instructionPointer & vm->instructionAddressMask].opData_int;
}

void opcode_pushLiteral_float(struct VM *vm)
{
    struct Value *stackVal = vmStackPush_internal(vm);
    vm->instructionPointer++;

    stackVal->type = VALUETYPE_FLOAT;
    stackVal->floatData =
        vm->instructions[vm->instructionPointer & vm->instructionAddressMask].opData_float;
}

void opcode_pushLiteral_string(struct VM *vm)
{
    struct Value *stackVal = vmStackPush_internal(vm);
    vm->instructionPointer++;

    stackVal->type = VALUETYPE_STRING;
    stackVal->stringTableEntry =
        vm->instructions[vm->instructionPointer & vm->instructionAddressMask].opData_string;

    dbgWriteLine("Pushing literal string: %d = %d", vm->instructionPointer, stackVal->stringTableEntry);

}

void opcode_pushLiteral_functionId(struct VM *vm)
{
    struct Value *stackVal = vmStackPush_internal(vm);
    vm->instructionPointer++;

    stackVal->type = VALUETYPE_FUNCTIONID;
    stackVal->functionId =
        vm->instructions[vm->instructionPointer & vm->instructionAddressMask].opData_functionId;
}

void opcode_nop(struct VM *vm)
{
}

void opcode_subtract(struct VM *vm)
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

void opcode_multiply(struct VM *vm)
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

void opcode_divide(struct VM *vm)
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

void opcode_modulo(struct VM *vm)
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
                    in1->intData %
                    val2);
            }
        } break;

        default: {
            struct DynString *ds =
                dynStrCreate("Modulo unimplemented for type ");
            dynStrAppend(ds, valueTypeGetName(type));
            dynStrAppend(ds, ".");
            errorStateAddError(
                &vm->errorState, -1,
                ds->data);
            dynStrDelete(ds);
        } break;
    }
}

void opcode_negate(struct VM *vm)
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

void opcode_pop(struct VM *vm)
{
    vmStackPop(vm);
}

void opcode_popN(struct VM *vm)
{
    struct Value *v = vmStackPop(vm);
    vmStackPopN(vm, valueToInt(vm, v));
}

void opcode_dump(struct VM *vm)
{
    vmStackPop(vm);
    // struct Value *v = vmStackPop(vm);
    // printf("Debug dump: ");
    // value_dump(vm, v);
    // printf("\n");
}

void opcode_stackPeek(struct VM *vm)
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

        dbgWriteLine("Fetched global value at stack position: %u", stackAddress);

    } else {

        // Negative stack address. Probably a local variable.
        uint32_t stackAddress = vm->stack.size + v->intData;
        struct Value *vIn = vmStackPeek(
            vm, stackAddress);
        struct Value *vOut = vmStackPush_internal(vm);
        *vOut = *vIn;

        dbgWriteLine("Fetched local value at stack position: %u", stackAddress);

    }
}

void opcode_stackPoke(struct VM *vm)
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

        dbgWriteLine("Set global value at stack position: %u", stackAddress);

    } else {
        // Negative stack address. Probably a local variable.
        uint32_t stackAddress = vm->stack.size + stackAddrValue->intData;
        struct Value *vIn = vmStackPeek(vm, (vm->stack.size - 1));
        struct Value *vOut = vmStackPeek(vm, stackAddress);
        *vOut = *vIn;

        dbgWriteLine("Set local value at stack position: %u", stackAddress);
    }
}

void opcode_jumpRelative(struct VM *vm)
{
    struct Value *offsetVal = vmStackPop(vm);
    vm->instructionPointer += valueToInt(vm, offsetVal);
}

void opcode_call(struct VM *vm)
{
    // Expected stack state at start...
    //   _argumentCount
    //   <_argumentCount number of arguments>
    //   function id

    uint32_t argumentCount = 0;
    uint32_t functionId = 0;
    struct VMFunction *funcOb = NULL;

    // PEEK at the top of the stack. That's _argumentCount.
    argumentCount = valueToInt(vm, vmStackPeek(vm, (vm->stack.size - 1)));

    dbgWriteLine("Calling function with argument count: %u", argumentCount);

    // PEEK at the function id (stack top - _argumentCount). Save it.
    {
        struct Value *functionIdValue = vmStackPeek(
            vm, vm->stack.size - (argumentCount + 2));

        if(functionIdValue->type != VALUETYPE_FUNCTIONID) {
            errorStateAddError(
                &vm->errorState,
                -1,
                "Tried to call something that is not a function id.");
            return;
        }

        functionId = functionIdValue->functionId;
        dbgWriteLine("Calling function with id: %u", functionId);
    }

    // Look up the function in our table of function objects.
    if(functionId >= vm->functionCount) {
        errorStateAddError(
            &vm->errorState,
            -1,
            "Bad function id.");
        return;
    }
    funcOb = &vm->functionTable[functionId];

    // Compare _argumentCount to the stored function object's
    // _argumentCount. Throw an error if they mismatch.
    if(funcOb->argumentCount != ~(uint32_t)0 &&
        funcOb->argumentCount != argumentCount)
    {
        errorStateAddError(
            &vm->errorState,
            -1,
            "Incorrect argument count for function call.");

        dbgWriteLine("funcOb->argumentCount: %u", funcOb->argumentCount);
        dbgWriteLine("argumentCount:         %u", argumentCount);

        return;
    }

    // At this point the behavior changes depending on if it's a C
    // function or script function.
    if(funcOb->isCFunction) {

        // In this case we just want to call the C function, pop all
        // of our data off the stack, push the return value, and then
        // return from this function.

        struct VMFunctionCallbackData data;
        memset(&data, 0, sizeof(data));

        // Fill in important stuff here.
        data.vm = vm;
        data.argumentCount = argumentCount;
        data.arguments = malloc(argumentCount * sizeof(struct Value));

        // Note: We're not simply giving the function a stack pointer,
        // because then the called function would have to worry about
        // the stack index mask and all that crap.

        // This is one of the few places where we do allocations based
        // directly off of a value given to us from the program, so
        // let's make sure that worked.
        if(!data.arguments) {
            errorStateAddError(
                &vm->errorState,
                -1,
                "Failed to allocated arguments for function call.");
            return;
        }

        // Copy arguments over.
        {
            uint32_t i;
            for(i = 0; i < argumentCount; i++) {
                data.arguments[i] =
                    vm->stack.values[
                        vm->stack.indexMask &
                        ((vm->stack.size - argumentCount - 1) + i)];
            }
        }

        // Actual function call.
        funcOb->CFunctionCallback(&data);

        free(data.arguments);

        // Pop all the arguments.
        vmStackPopN(vm, argumentCount + 2);

        // Push return value.
        {
            struct Value *retVal = vmStackPush_internal(vm);
            *retVal = data.returnValue;
        }

    } else {

        // Push the current instruction pointer (_returnPointer).
        vmStackPushInt(vm, vm->instructionPointer);

        // Set the instruction pointer to the saved function object's
        // instruction pointer. Maybe minus one.
        vm->instructionPointer = funcOb->firstInstructionIndex - 1;

        // Expected stack state at end...
        //   _returnPointer
        //   _argumentCount
        //   <_argumentCount number of arguments>
        //   function id
    }
}

void opcode_return(struct VM *vm)
{
    // Expected stack state at start...
    //   context amount to throw away (contextCount)
    //       (generate from cs->context->stackFrameOffset - functionArgumentCount? - 1 or 2)
    //   _returnValue
    //   <contextCount number of slots>
    //   _returnPointer (from CALL)
    //   _argumentCount (from before CALL)
    //   <_argumentCount number of arguments> (from before CALL)
    //   function id (from before CALL)

    struct Value *returnValue = NULL;
    struct Value *contextCountValue = NULL;
    uint32_t returnAddress = 0;
    uint32_t argumentCount = 0;

    // Pop a value, contextCount, off the stack.
    contextCountValue = vmStackPop(vm);

    // Pop a value off the stack. That's _returnValue.
    returnValue = vmStackPop(vm);

    // Pop contextCount more values off the stack and throw them away.
    // (Function context we're done with.)
    vmStackPopN(vm, valueToInt(vm, contextCountValue));

    // Pop the _returnPointer off the stack and keep it.
    returnAddress = valueToInt(vm, vmStackPop(vm));

    // Pop the _argumentCount off the stack and keep it.
    argumentCount = valueToInt(vm, vmStackPop(vm));

    // Pop _argumentCount values off the stack and throw them away.
    // (Function arguments we're done with.)
    vmStackPopN(vm, argumentCount);

    // Pop function id off the stack.
    vmStackPop(vm);

    // Push _returnValue back onto the stack.
    {
        struct Value *returnValueWrite = vmStackPush_internal(vm);
        *returnValueWrite = *returnValue;
    }

    // Set the instruction pointer to _returnPointer - 1.
    vm->instructionPointer = returnAddress;

    // Expected stack state at end...
    //   _returnValue
}

void opcode_end(struct VM *vm)
{
    // This doesn't actually do anything. The iteration loops know to
    // check for it and stop, though.
}

void opcode_jz(struct VM *vm)
{
    struct Value *relativeOffsetValue = vmStackPop(vm);
    struct Value *testValue = vmStackPop(vm);

    dbgWriteLine("Testing branch value %d. Address now: %u", valueToInt(vm, testValue), vm->instructionPointer);
    if(valueToInt(vm, testValue) == 0) {
        vm->instructionPointer += valueToInt(vm, relativeOffsetValue);
        dbgWriteLine("Branch taken. Address now: %u", vm->instructionPointer);
    } else {
        dbgWriteLine("Branch NOT taken. Address now: %u", vm->instructionPointer);
    }
}

// This will return ~0 for invalid comparisons. Otherwise the return
// value is like strcmp(). Invalid comparisons can be treated as a
// "not equal", but should not be used for any greater/less than
// comparisons (always false).
int32_t opcode_internal_compare(struct VM *vm)
{
    struct Value *in2 = vmStackPop(vm);
    struct Value *in1 = vmStackPop(vm);

    enum ValueType type = in1->type;

    switch(type) {

        case VALUETYPE_INT: {
            int32_t other = valueToInt(vm, in2);
            if(in1->intData > other) {
                return 1;
            } else if(in1->intData == other) {
                return 0;
            } else {
                return -1;
            }
        } break;

        case VALUETYPE_FLOAT: {
            float other = valueToFloat(vm, in2);
            if(in1->floatData > other) {
                return 1;
            } else if(in1->floatData == other) {
                return 0;
            } else {
                return -1;
            }
        } break;

        case VALUETYPE_STRING: {
            const char *other = valueToString(vm, in2);
            const char *thisData = valueToString(vm, in1);

            // Shortcut it if we ended up with two of the same entry
            // in the string table.
            if(other == thisData) {
                return 0;
            }

            return strcmp(thisData, other);
        } break;

        case VALUETYPE_FUNCTIONID: {
            if(in2->type == VALUETYPE_FUNCTIONID &&
                in2->functionId == in1->functionId)
            {
                return 0;
            } else {
                return ~0;
            }
        }

        default: {
            struct DynString *ds =
                dynStrCreate("Comparison unimplemented for type ");
            dynStrAppend(ds, valueTypeGetName(type));
            dynStrAppend(ds, ".");
            errorStateAddError(
                &vm->errorState, -1,
                ds->data);
            dynStrDelete(ds);
        } break;
    }

    return ~0;
}

void opcode_gt(struct VM *vm)
{
    int32_t comparison = opcode_internal_compare(vm);
    vmStackPushInt(vm, comparison == 1);
}

void opcode_lt(struct VM *vm)
{
    int32_t comparison = opcode_internal_compare(vm);
    vmStackPushInt(vm, comparison == -1);
}

void opcode_ge(struct VM *vm)
{
    int32_t comparison = opcode_internal_compare(vm);
    vmStackPushInt(vm, comparison == 0 || comparison == 1);
}

void opcode_le(struct VM *vm)
{
    int32_t comparison = opcode_internal_compare(vm);
    vmStackPushInt(vm, comparison == 0 || comparison == -1);
}

void opcode_eq(struct VM *vm)
{
    int32_t comparison = opcode_internal_compare(vm);
    vmStackPushInt(vm, comparison == 0);
}

void opcode_ne(struct VM *vm)
{
    int32_t comparison = opcode_internal_compare(vm);
    vmStackPushInt(vm, comparison != 0);
}

void opcode_eqsametype(struct VM *vm)
{
    uint32_t stackSize = vm->stack.size;
    bool sameTypes =
        vmStackPeek(vm, stackSize - 1)->type ==
        vmStackPeek(vm, stackSize - 2)->type;

    if(sameTypes) {
        int32_t comparison = opcode_internal_compare(vm);
        vmStackPushInt(vm, comparison == 0);
    } else {
        vmStackPopN(vm, 2);
        vmStackPushInt(vm, 0);
    }
}

void opcode_not(struct VM *vm)
{
    vmStackPushInt(vm, !valueToInt(vm, vmStackPop(vm)));
}

void opcode_and(struct VM *vm)
{
    // Do NOT try to inline these function calls in this expression. C
    // shortcutting will ruin your day.
    bool in1 = valueToInt(vm, vmStackPop(vm));
    bool in2 = valueToInt(vm, vmStackPop(vm));
    vmStackPushInt(vm, in1 && in2);
}

void opcode_or(struct VM *vm)
{
    // Do NOT try to inline these function calls in this expression. C
    // shortcutting will ruin your day.
    bool in1 = valueToInt(vm, vmStackPop(vm));
    bool in2 = valueToInt(vm, vmStackPop(vm));
    vmStackPushInt(vm, in1 || in2);
}


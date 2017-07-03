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

    printf("Pushing literal string: %d = %d\n", vm->instructionPointer, stackVal->stringTableEntry);

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
    struct Value *v = vmStackPop(vm);
    printf("Debug dump: ");
    value_dump(vm, v);
    printf("\n");
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

    printf("Calling function with argument count: %u\n", argumentCount);

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
        printf("Calling function with id: %u\n", functionId);
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

        printf("funcOb->argumentCount: %u\n", funcOb->argumentCount);
        printf("argumentCount:         %u\n", argumentCount);

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

    printf("Testing branch value %d. Address now: %u\n", valueToInt(vm, testValue), vm->instructionPointer);
    if(valueToInt(vm, testValue) == 0) {
        vm->instructionPointer += valueToInt(vm, relativeOffsetValue);
        printf("Branch taken. Address now: %u\n", vm->instructionPointer);
    } else {
        printf("Branch NOT taken. Address now: %u\n", vm->instructionPointer);
    }
}

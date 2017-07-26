#include "common.h"

void opcode_add(struct VM *vm)
{
    struct Value *in2  = vmStackPop(vm);
    struct Value *in1  = vmStackPop(vm);

    enum NKValueType type = in1->type;

    switch(type) {

        case NK_VALUETYPE_INT:
            vmStackPushInt(
                vm,
                in1->intData +
                valueToInt(vm, in2));
            break;

        case NK_VALUETYPE_FLOAT:
            vmStackPushFloat(
                vm,
                in1->floatData +
                valueToFloat(vm, in2));
            break;

        case NK_VALUETYPE_STRING: {

            // Make a new string that is the concatenated values.
            // Start with a DynString of the first one.
            struct NKDynString *dynStr =
                nkiDynStrCreate(vm,
                    vmStringTableGetStringById(
                        &vm->stringTable,
                        in1->stringTableEntry));

            // Append the other one, after conversion to string if
            // necessary.
            nkiDynStrAppend(dynStr, valueToString(vm, in2));

            // Push the result.
            vmStackPushString(
                vm, dynStr->data);

            // Clean up.
            nkiDynStrDelete(dynStr);

        } break;

            // TODO: Array concatenation support.

        default: {
            struct NKDynString *ds =
                nkiDynStrCreate(vm, "Addition unimplemented for type ");
            nkiDynStrAppend(ds, valueTypeGetName(type));
            nkiDynStrAppend(ds, ".");
            nkiAddError(
                vm, -1,
                ds->data);
            nkiDynStrDelete(ds);
            return;
        }
    }
}

void opcode_pushLiteral_int(struct VM *vm)
{
    struct Value *stackVal = vmStackPush_internal(vm);
    vm->instructionPointer++;

    stackVal->type = NK_VALUETYPE_INT;
    stackVal->intData =
        vm->instructions[vm->instructionPointer & vm->instructionAddressMask].opData_int;
}

void opcode_pushLiteral_float(struct VM *vm)
{
    struct Value *stackVal = vmStackPush_internal(vm);
    vm->instructionPointer++;

    stackVal->type = NK_VALUETYPE_FLOAT;
    stackVal->floatData =
        vm->instructions[vm->instructionPointer & vm->instructionAddressMask].opData_float;
}

void opcode_pushLiteral_string(struct VM *vm)
{
    struct Value *stackVal = vmStackPush_internal(vm);
    vm->instructionPointer++;

    stackVal->type = NK_VALUETYPE_STRING;
    stackVal->stringTableEntry =
        vm->instructions[vm->instructionPointer & vm->instructionAddressMask].opData_string;

    dbgWriteLine("Pushing literal string: %d = %d", vm->instructionPointer, stackVal->stringTableEntry);

}

void opcode_pushLiteral_functionId(struct VM *vm)
{
    struct Value *stackVal = vmStackPush_internal(vm);
    vm->instructionPointer++;

    stackVal->type = NK_VALUETYPE_FUNCTIONID;
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

    enum NKValueType type = in1->type;

    switch(type) {

        case NK_VALUETYPE_INT:
            vmStackPushInt(
                vm,
                in1->intData -
                valueToInt(vm, in2));
            break;

        case NK_VALUETYPE_FLOAT:
            vmStackPushFloat(
                vm,
                in1->floatData -
                valueToFloat(vm, in2));
            break;

        default: {
            struct NKDynString *ds =
                nkiDynStrCreate(vm, "Subtraction unimplemented for type ");
            nkiDynStrAppend(ds, valueTypeGetName(type));
            nkiDynStrAppend(ds, ".");
            nkiAddError(
                vm, -1,
                ds->data);
            nkiDynStrDelete(ds);
        } break;
    }
}

void opcode_multiply(struct VM *vm)
{
    struct Value *in2 = vmStackPop(vm);
    struct Value *in1 = vmStackPop(vm);

    enum NKValueType type = in1->type;

    switch(type) {

        case NK_VALUETYPE_INT:
            vmStackPushInt(
                vm,
                in1->intData *
                valueToInt(vm, in2));
            break;

        case NK_VALUETYPE_FLOAT:
            vmStackPushFloat(
                vm,
                in1->floatData *
                valueToFloat(vm, in2));
            break;

        default: {
            struct NKDynString *ds =
                nkiDynStrCreate(vm, "Multiplication unimplemented for type ");
            nkiDynStrAppend(ds, valueTypeGetName(type));
            nkiDynStrAppend(ds, ".");
            nkiAddError(
                vm, -1,
                ds->data);
            nkiDynStrDelete(ds);
        } break;
    }
}

void opcode_divide(struct VM *vm)
{
    struct Value *in2 = vmStackPop(vm);
    struct Value *in1 = vmStackPop(vm);

    enum NKValueType type = in1->type;

    switch(type) {

        case NK_VALUETYPE_INT: {
            int32_t val2 = valueToInt(vm, in2);
            if(val2 == 0) {
                nkiAddError(
                    vm, -1,
                    "Integer divide-by-zero.");
            } else {
                vmStackPushInt(
                    vm,
                    in1->intData /
                    val2);
            }
        } break;

        case NK_VALUETYPE_FLOAT:
            vmStackPushFloat(
                vm,
                in1->floatData /
                valueToFloat(vm, in2));
            break;

        default: {
            struct NKDynString *ds =
                nkiDynStrCreate(vm, "Division unimplemented for type ");
            nkiDynStrAppend(ds, valueTypeGetName(type));
            nkiDynStrAppend(ds, ".");
            nkiAddError(
                vm, -1,
                ds->data);
            nkiDynStrDelete(ds);
        } break;
    }
}

void opcode_modulo(struct VM *vm)
{
    struct Value *in2 = vmStackPop(vm);
    struct Value *in1 = vmStackPop(vm);

    enum NKValueType type = in1->type;

    switch(type) {

        case NK_VALUETYPE_INT: {
            int32_t val2 = valueToInt(vm, in2);
            if(val2 == 0) {
                nkiAddError(
                    vm, -1,
                    "Integer divide-by-zero.");
            } else {
                vmStackPushInt(
                    vm,
                    in1->intData %
                    val2);
            }
        } break;

        default: {
            struct NKDynString *ds =
                nkiDynStrCreate(vm, "Modulo unimplemented for type ");
            nkiDynStrAppend(ds, valueTypeGetName(type));
            nkiDynStrAppend(ds, ".");
            nkiAddError(
                vm, -1,
                ds->data);
            nkiDynStrDelete(ds);
        } break;
    }
}

void opcode_negate(struct VM *vm)
{
    struct Value *in1 = vmStackPop(vm);

    enum NKValueType type = in1->type;

    switch(type) {

        case NK_VALUETYPE_INT: {
            vmStackPushInt(
                vm,
                -(in1->intData));
        } break;

        case NK_VALUETYPE_FLOAT: {
            vmStackPushFloat(
                vm,
                -(in1->floatData));
        } break;

        default: {
            struct NKDynString *ds =
                nkiDynStrCreate(vm, "Negation unimplemented for type ");
            nkiDynStrAppend(ds, valueTypeGetName(type));
            nkiDynStrAppend(ds, ".");
            nkiAddError(
                vm, -1,
                ds->data);
            nkiDynStrDelete(ds);
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
    if(v->type != NK_VALUETYPE_INT) {
        nkiAddError(vm, -1,
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
    if(stackAddrValue->type != NK_VALUETYPE_INT) {
        nkiAddError(vm, -1,
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

        if(functionIdValue->type != NK_VALUETYPE_FUNCTIONID) {
            nkiAddError(
                vm,
                -1,
                "Tried to call something that is not a function id.");
            return;
        }

        functionId = functionIdValue->functionId;
        dbgWriteLine("Calling function with id: %u", functionId);
    }

    // Look up the function in our table of function objects.
    if(functionId >= vm->functionCount) {
        nkiAddError(
            vm,
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
        nkiAddError(
            vm,
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
        data.arguments = nkiMalloc(vm, argumentCount * sizeof(struct Value));
        data.userData = funcOb->CFunctionCallbackUserdata;

        // Note: We're not simply giving the function a stack pointer,
        // because then the called function would have to worry about
        // the stack index mask and all that crap.

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

        // If the C function called some code back inside the VM, we
        // may have suffered a catastrophic failure in that code, so
        // returning up through the stack in this side would not be a
        // good idea, because the VM may be in a bad state. Chain
        // another catastrophe event to get us all the way back to the
        // last user call.
        if(NK_CHECK_CATASTROPHE()) {
            NK_CATASTROPHE();
        }

        nkFree(vm, data.arguments);

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

    // Okay, it does do one thing, and that is keeping itself on the
    // end instruction.
    vm->instructionPointer--;
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

int32_t opcode_internal_compare(struct VM *vm)
{
    struct Value *in2 = vmStackPop(vm);
    struct Value *in1 = vmStackPop(vm);

    return value_compare(vm, in1, in2, false);
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

void opcode_createObject(struct VM *vm)
{
    struct Value *v = vmStackPush_internal(vm);
    v->type = NK_VALUETYPE_OBJECTID;
    v->objectId = vmObjectTableCreateObject(vm);
}

void opcode_objectFieldGet_internal(struct VM *vm, bool popObject)
{
    struct Value *indexToGet = vmStackPop(vm);
    struct Value *objectToGet;
    struct Value *output;
    struct Value *objectValue;
    struct VMObject *ob;

    if(popObject) {
        objectToGet = vmStackPop(vm);
    } else {
        objectToGet = vmStackPeek(vm, vm->stack.size - 1);
    }

    if(objectToGet->type != NK_VALUETYPE_OBJECTID) {
        nkiAddError(
            vm,
            -1,
            "Attempted to get a field on a value that is not an object.");
        return;
    }

    ob = vmObjectTableGetEntryById(&vm->objectTable, objectToGet->objectId);

    if(!ob) {
        nkiAddError(
            vm,
            -1,
            "Bad object id in opcode_objectFieldGet.");
        return;
    }

    output = vmStackPush_internal(vm);

    objectValue =
        vmObjectFindOrAddEntry(vm, ob, indexToGet, true);

    if(objectValue) {
        *output = *objectValue;
    } else {
        output->type = NK_VALUETYPE_NIL;
        output->basicHashValue = 0;
    }
}

void opcode_objectFieldGet(struct VM *vm)
{
    opcode_objectFieldGet_internal(vm, true);
}

void opcode_objectFieldGet_noPop(struct VM *vm)
{
    opcode_objectFieldGet_internal(vm, false);
}

void opcode_objectFieldSet(struct VM *vm)
{
    struct Value *indexToSet  = vmStackPop(vm);
    struct Value *objectToSet = vmStackPop(vm);
    struct Value *valueToSet  = vmStackPop(vm);

    // TODO: Actually assign the value.
    struct Value *objectValue;
    struct VMObject *ob;

    if(objectToSet->type != NK_VALUETYPE_OBJECTID) {
        nkiAddError(
            vm,
            -1,
            "Attempted to set a field on a value that is not an object.");
        return;
    }

    ob = vmObjectTableGetEntryById(&vm->objectTable, objectToSet->objectId);

    if(!ob) {
        nkiAddError(
            vm,
            -1,
            "Bad object id in opcode_objectFieldSet.");
        return;
    }

    if(valueToSet->type == NK_VALUETYPE_NIL) {

        // For nil, we actually want to remove a field.

        vmObjectClearEntry(vm, ob, indexToSet);

    } else {

        // For non-nil values, set or create the field.

        objectValue =
            vmObjectFindOrAddEntry(vm, ob, indexToSet, false);

        if(objectValue) {
            *objectValue = *valueToSet;
        }
    }

    // Leave the assigned value on the stack.
    *vmStackPush_internal(vm) = *valueToSet;
}

void opcode_prepareSelfCall(struct VM *vm)
{
    struct Value *argumentCount = vmStackPeek(vm, vm->stack.size - 1);
    uint32_t stackOffset = valueToInt(vm, argumentCount);
    struct Value *function = vmStackPeek(vm, vm->stack.size - (stackOffset + 3));
    struct Value *object = vmStackPeek(vm, vm->stack.size - (stackOffset + 2));
    struct Value tmp;
    tmp = *function;
    *function = *object;
    *object = tmp;

    // Fixup argument count.
    argumentCount->type = NK_VALUETYPE_INT;
    argumentCount->intData = stackOffset + 1;
}

void opcode_pushNil(struct VM *vm)
{
    struct Value *v = vmStackPush_internal(vm);
    v->type = NK_VALUETYPE_NIL;
}

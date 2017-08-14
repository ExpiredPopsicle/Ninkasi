#include "common.h"

void nkiOpcode_add(struct NKVM *vm)
{
    struct NKValue *in2  = vmStackPop(vm);
    struct NKValue *in1  = vmStackPop(vm);

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
                    nkiVmStringTableGetStringById(
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

void nkiOpcode_pushLiteral_int(struct NKVM *vm)
{
    struct NKValue *stackVal = vmStackPush_internal(vm);
    vm->instructionPointer++;

    stackVal->type = NK_VALUETYPE_INT;
    stackVal->intData =
        vm->instructions[vm->instructionPointer & vm->instructionAddressMask].opData_int;
}

void nkiOpcode_pushLiteral_float(struct NKVM *vm)
{
    struct NKValue *stackVal = vmStackPush_internal(vm);
    vm->instructionPointer++;

    stackVal->type = NK_VALUETYPE_FLOAT;
    stackVal->floatData =
        vm->instructions[vm->instructionPointer & vm->instructionAddressMask].opData_float;
}

void nkiOpcode_pushLiteral_string(struct NKVM *vm)
{
    struct NKValue *stackVal = vmStackPush_internal(vm);
    vm->instructionPointer++;

    stackVal->type = NK_VALUETYPE_STRING;
    stackVal->stringTableEntry =
        vm->instructions[vm->instructionPointer & vm->instructionAddressMask].opData_string;

    dbgWriteLine("Pushing literal string: %d = %d", vm->instructionPointer, stackVal->stringTableEntry);

}

void nkiOpcode_pushLiteral_functionId(struct NKVM *vm)
{
    struct NKValue *stackVal = vmStackPush_internal(vm);
    vm->instructionPointer++;

    stackVal->type = NK_VALUETYPE_FUNCTIONID;
    stackVal->functionId =
        vm->instructions[vm->instructionPointer & vm->instructionAddressMask].opData_functionId;
}

void nkiOpcode_nop(struct NKVM *vm)
{
}

void nkiOpcode_subtract(struct NKVM *vm)
{
    struct NKValue *in2 = vmStackPop(vm);
    struct NKValue *in1 = vmStackPop(vm);

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

void nkiOpcode_multiply(struct NKVM *vm)
{
    struct NKValue *in2 = vmStackPop(vm);
    struct NKValue *in1 = vmStackPop(vm);

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

void nkiOpcode_divide(struct NKVM *vm)
{
    struct NKValue *in2 = vmStackPop(vm);
    struct NKValue *in1 = vmStackPop(vm);

    enum NKValueType type = in1->type;

    switch(type) {

        case NK_VALUETYPE_INT: {
            nkint32_t val2 = valueToInt(vm, in2);
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

void nkiOpcode_modulo(struct NKVM *vm)
{
    struct NKValue *in2 = vmStackPop(vm);
    struct NKValue *in1 = vmStackPop(vm);

    enum NKValueType type = in1->type;

    switch(type) {

        case NK_VALUETYPE_INT: {
            nkint32_t val2 = valueToInt(vm, in2);
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

void nkiOpcode_negate(struct NKVM *vm)
{
    struct NKValue *in1 = vmStackPop(vm);

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

void nkiOpcode_pop(struct NKVM *vm)
{
    vmStackPop(vm);
}

void nkiOpcode_popN(struct NKVM *vm)
{
    struct NKValue *v = vmStackPop(vm);
    vmStackPopN(vm, valueToInt(vm, v));
}

void nkiOpcode_dump(struct NKVM *vm)
{
    vmStackPop(vm);
    // struct NKValue *v = vmStackPop(vm);
    // printf("Debug dump: ");
    // value_dump(vm, v);
    // printf("\n");
}

void nkiOpcode_stackPeek(struct NKVM *vm)
{
    // Read index.
    struct NKValue *v = vmStackPop(vm);
    if(v->type != NK_VALUETYPE_INT) {
        nkiAddError(vm, -1,
            "Attempted to use a non-integer as a stack index.");
        return;
    }

    // Copy stack data over.
    if(v->intData >= 0) {

        // Absolute stack address. Probably a global variable.
        nkuint32_t stackAddress = v->intData;
        struct NKValue *vIn = vmStackPeek(vm, v->intData);
        struct NKValue *vOut = vmStackPush_internal(vm);
        *vOut = *vIn;

        dbgWriteLine("Fetched global value at stack position: %u", stackAddress);

    } else {

        // Negative stack address. Probably a local variable.
        nkuint32_t stackAddress = vm->stack.size + v->intData;
        struct NKValue *vIn = vmStackPeek(
            vm, stackAddress);
        struct NKValue *vOut = vmStackPush_internal(vm);
        *vOut = *vIn;

        dbgWriteLine("Fetched local value at stack position: %u", stackAddress);

    }
}

void nkiOpcode_stackPoke(struct NKVM *vm)
{
    // Read index.
    struct NKValue *stackAddrValue = vmStackPop(vm);
    if(stackAddrValue->type != NK_VALUETYPE_INT) {
        nkiAddError(vm, -1,
            "Attempted to use a non-integer as a stack index.");
        return;
    }

    // Copy to stack.
    if(stackAddrValue->intData >= 0) {

        // Absolute stack address. Probably a global variable.
        nkuint32_t stackAddress = stackAddrValue->intData;
        struct NKValue *vIn = vmStackPeek(vm, (vm->stack.size - 1));
        struct NKValue *vOut = vmStackPeek(vm, stackAddrValue->intData);
        *vOut = *vIn;

        dbgWriteLine("Set global value at stack position: %u", stackAddress);

    } else {
        // Negative stack address. Probably a local variable.
        nkuint32_t stackAddress = vm->stack.size + stackAddrValue->intData;
        struct NKValue *vIn = vmStackPeek(vm, (vm->stack.size - 1));
        struct NKValue *vOut = vmStackPeek(vm, stackAddress);
        *vOut = *vIn;

        dbgWriteLine("Set local value at stack position: %u", stackAddress);
    }
}

void nkiOpcode_jumpRelative(struct NKVM *vm)
{
    struct NKValue *offsetVal = vmStackPop(vm);
    vm->instructionPointer += valueToInt(vm, offsetVal);
}

void nkiOpcode_call(struct NKVM *vm)
{
    // Expected stack state at start...
    //   _argumentCount
    //   <_argumentCount number of arguments>
    //   function id

    nkuint32_t argumentCount = 0;
    nkuint32_t functionId = 0;
    struct NKVMFunction *funcOb = NULL;

    // PEEK at the top of the stack. That's _argumentCount.
    argumentCount = valueToInt(vm, vmStackPeek(vm, (vm->stack.size - 1)));

    dbgWriteLine("Calling function with argument count: %u", argumentCount);

    // PEEK at the function id (stack top - _argumentCount). Save it.
    {
        struct NKValue *functionIdValue = vmStackPeek(
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
    if(funcOb->argumentCount != ~(nkuint32_t)0 &&
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

        struct NKVMFunctionCallbackData data;
        memset(&data, 0, sizeof(data));

        // Fill in important stuff here.
        data.vm = vm;
        data.argumentCount = argumentCount;
        data.arguments = nkiMalloc(vm, argumentCount * sizeof(struct NKValue));
        data.userData = funcOb->CFunctionCallbackUserdata;

        // Note: We're not simply giving the function a stack pointer,
        // because then the called function would have to worry about
        // the stack index mask and all that crap.

        // Copy arguments over.
        {
            nkuint32_t i;
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

        nkiFree(vm, data.arguments);

        // Pop all the arguments.
        vmStackPopN(vm, argumentCount + 2);

        // Push return value.
        {
            struct NKValue *retVal = vmStackPush_internal(vm);
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

void nkiOpcode_return(struct NKVM *vm)
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

    struct NKValue *returnValue = NULL;
    struct NKValue *contextCountValue = NULL;
    nkuint32_t returnAddress = 0;
    nkuint32_t argumentCount = 0;

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
        struct NKValue *returnValueWrite = vmStackPush_internal(vm);
        *returnValueWrite = *returnValue;
    }

    // Set the instruction pointer to _returnPointer - 1.
    vm->instructionPointer = returnAddress;

    // Expected stack state at end...
    //   _returnValue
}

void nkiOpcode_end(struct NKVM *vm)
{
    // This doesn't actually do anything. The iteration loops know to
    // check for it and stop, though.

    // Okay, it does do one thing, and that is keeping itself on the
    // end instruction.
    vm->instructionPointer--;
}

void nkiOpcode_jz(struct NKVM *vm)
{
    struct NKValue *relativeOffsetValue = vmStackPop(vm);
    struct NKValue *testValue = vmStackPop(vm);

    dbgWriteLine("Testing branch value %d. Address now: %u", valueToInt(vm, testValue), vm->instructionPointer);
    if(valueToInt(vm, testValue) == 0) {
        vm->instructionPointer += valueToInt(vm, relativeOffsetValue);
        dbgWriteLine("Branch taken. Address now: %u", vm->instructionPointer);
    } else {
        dbgWriteLine("Branch NOT taken. Address now: %u", vm->instructionPointer);
    }
}

nkint32_t nkiOpcode_internal_compare(struct NKVM *vm)
{
    struct NKValue *in2 = vmStackPop(vm);
    struct NKValue *in1 = vmStackPop(vm);

    return value_compare(vm, in1, in2, nkfalse);
}

void nkiOpcode_gt(struct NKVM *vm)
{
    nkint32_t comparison = nkiOpcode_internal_compare(vm);
    vmStackPushInt(vm, comparison == 1);
}

void nkiOpcode_lt(struct NKVM *vm)
{
    nkint32_t comparison = nkiOpcode_internal_compare(vm);
    vmStackPushInt(vm, comparison == -1);
}

void nkiOpcode_ge(struct NKVM *vm)
{
    nkint32_t comparison = nkiOpcode_internal_compare(vm);
    vmStackPushInt(vm, comparison == 0 || comparison == 1);
}

void nkiOpcode_le(struct NKVM *vm)
{
    nkint32_t comparison = nkiOpcode_internal_compare(vm);
    vmStackPushInt(vm, comparison == 0 || comparison == -1);
}

void nkiOpcode_eq(struct NKVM *vm)
{
    nkint32_t comparison = nkiOpcode_internal_compare(vm);
    vmStackPushInt(vm, comparison == 0);
}

void nkiOpcode_ne(struct NKVM *vm)
{
    nkint32_t comparison = nkiOpcode_internal_compare(vm);
    vmStackPushInt(vm, comparison != 0);
}

void nkiOpcode_eqsametype(struct NKVM *vm)
{
    nkuint32_t stackSize = vm->stack.size;
    nkbool sameTypes =
        vmStackPeek(vm, stackSize - 1)->type ==
        vmStackPeek(vm, stackSize - 2)->type;

    if(sameTypes) {
        nkint32_t comparison = nkiOpcode_internal_compare(vm);
        vmStackPushInt(vm, comparison == 0);
    } else {
        vmStackPopN(vm, 2);
        vmStackPushInt(vm, 0);
    }
}

void nkiOpcode_not(struct NKVM *vm)
{
    vmStackPushInt(vm, !valueToInt(vm, vmStackPop(vm)));
}

void nkiOpcode_and(struct NKVM *vm)
{
    // Do NOT try to inline these function calls in this expression. C
    // shortcutting will ruin your day.
    nkbool in1 = valueToInt(vm, vmStackPop(vm));
    nkbool in2 = valueToInt(vm, vmStackPop(vm));
    vmStackPushInt(vm, in1 && in2);
}

void nkiOpcode_or(struct NKVM *vm)
{
    // Do NOT try to inline these function calls in this expression. C
    // shortcutting will ruin your day.
    nkbool in1 = valueToInt(vm, vmStackPop(vm));
    nkbool in2 = valueToInt(vm, vmStackPop(vm));
    vmStackPushInt(vm, in1 || in2);
}

void nkiOpcode_createObject(struct NKVM *vm)
{
    struct NKValue *v = vmStackPush_internal(vm);
    v->type = NK_VALUETYPE_OBJECTID;
    v->objectId = nkiVmObjectTableCreateObject(vm);
}

void nkiOpcode_objectFieldGet_internal(struct NKVM *vm, nkbool popObject)
{
    struct NKValue *indexToGet = vmStackPop(vm);
    struct NKValue *objectToGet;
    struct NKValue *output;
    struct NKValue *objectValue;
    struct NKVMObject *ob;

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

    ob = nkiVmObjectTableGetEntryById(&vm->objectTable, objectToGet->objectId);

    if(!ob) {
        nkiAddError(
            vm,
            -1,
            "Bad object id in nkiOpcode_objectFieldGet.");
        return;
    }

    output = vmStackPush_internal(vm);

    objectValue =
        nkiVmObjectFindOrAddEntry(vm, ob, indexToGet, nktrue);

    if(objectValue) {
        *output = *objectValue;
    } else {
        output->type = NK_VALUETYPE_NIL;
        output->basicHashValue = 0;
    }
}

void nkiOpcode_objectFieldGet(struct NKVM *vm)
{
    nkiOpcode_objectFieldGet_internal(vm, nktrue);
}

void nkiOpcode_objectFieldGet_noPop(struct NKVM *vm)
{
    nkiOpcode_objectFieldGet_internal(vm, nkfalse);
}

void nkiOpcode_objectFieldSet(struct NKVM *vm)
{
    struct NKValue *indexToSet  = vmStackPop(vm);
    struct NKValue *objectToSet = vmStackPop(vm);
    struct NKValue *valueToSet  = vmStackPop(vm);

    // TODO: Actually assign the value.
    struct NKValue *objectValue;
    struct NKVMObject *ob;

    if(objectToSet->type != NK_VALUETYPE_OBJECTID) {
        nkiAddError(
            vm,
            -1,
            "Attempted to set a field on a value that is not an object.");
        return;
    }

    ob = nkiVmObjectTableGetEntryById(&vm->objectTable, objectToSet->objectId);

    if(!ob) {
        nkiAddError(
            vm,
            -1,
            "Bad object id in nkiOpcode_objectFieldSet.");
        return;
    }

    if(valueToSet->type == NK_VALUETYPE_NIL) {

        // For nil, we actually want to remove a field.

        nkiVmObjectClearEntry(vm, ob, indexToSet);

    } else {

        // For non-nil values, set or create the field.

        objectValue =
            nkiVmObjectFindOrAddEntry(vm, ob, indexToSet, nkfalse);

        if(objectValue) {
            *objectValue = *valueToSet;
        }
    }

    // Leave the assigned value on the stack.
    *vmStackPush_internal(vm) = *valueToSet;
}

void nkiOpcode_prepareSelfCall(struct NKVM *vm)
{
    struct NKValue *argumentCount = vmStackPeek(vm, vm->stack.size - 1);
    nkuint32_t stackOffset = valueToInt(vm, argumentCount);
    struct NKValue *function = vmStackPeek(vm, vm->stack.size - (stackOffset + 3));
    struct NKValue *object = vmStackPeek(vm, vm->stack.size - (stackOffset + 2));
    struct NKValue tmp;
    tmp = *function;
    *function = *object;
    *object = tmp;

    // Fixup argument count.
    argumentCount->type = NK_VALUETYPE_INT;
    argumentCount->intData = stackOffset + 1;
}

void nkiOpcode_pushNil(struct NKVM *vm)
{
    struct NKValue *v = vmStackPush_internal(vm);
    v->type = NK_VALUETYPE_NIL;
}

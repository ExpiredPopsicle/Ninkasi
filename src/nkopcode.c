// ----------------------------------------------------------------------
//
//        ▐ ▄ ▪   ▐ ▄ ▄ •▄  ▄▄▄· .▄▄ · ▪
//       •█▌▐███ •█▌▐██▌▄▌▪▐█ ▀█ ▐█ ▀. ██
//       ▐█▐▐▌▐█·▐█▐▐▌▐▀▀▄·▄█▀▀█ ▄▀▀▀█▄▐█·
//       ██▐█▌▐█▌██▐█▌▐█.█▌▐█ ▪▐▌▐█▄▪▐█▐█▌
//       ▀▀ █▪▀▀▀▀▀ █▪·▀  ▀ ▀  ▀  ▀▀▀▀ ▀▀▀
//
// ----------------------------------------------------------------------
//
//   Ninkasi 0.01
//
//   By Kiri "ExpiredPopsicle" Jolly
//     https://expiredpopsicle.com
//     https://intoxicoding.com
//     expiredpopsicle@gmail.com
//
// ----------------------------------------------------------------------
//
//   Copyright (c) 2017 Kiri Jolly
//
//   Permission is hereby granted, free of charge, to any person
//   obtaining a copy of this software and associated documentation files
//   (the "Software"), to deal in the Software without restriction,
//   including without limitation the rights to use, copy, modify, merge,
//   publish, distribute, sublicense, and/or sell copies of the Software,
//   and to permit persons to whom the Software is furnished to do so,
//   subject to the following conditions:
//
//   The above copyright notice and this permission notice shall be
//   included in all copies or substantial portions of the Software.
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
//   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
//   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//   SOFTWARE.
//
// -------------------------- END HEADER -------------------------------------

#include "nkcommon.h"

void nkiOpcode_add(struct NKVM *vm)
{
    struct NKValue *in2  = nkiVmStackPop(vm);
    struct NKValue *in1  = nkiVmStackPop(vm);

    enum NKValueType type = in1->type;

    switch(type) {

        case NK_VALUETYPE_INT:

            nkiVmStackPushInt(
                vm,
                in1->intData +
                nkiValueToInt(vm, in2));
            break;

        case NK_VALUETYPE_FLOAT:

            nkiVmStackPushFloat(
                vm,
                in1->floatData +
                nkiValueToFloat(vm, in2));
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
            nkiDynStrAppend(dynStr, nkiValueToString(vm, in2));

            // Push the result.
            nkiVmStackPushString(
                vm, dynStr->data);

            // Clean up.
            nkiDynStrDelete(dynStr);

        } break;

            // TODO: Array concatenation support.

        default: {
            struct NKDynString *ds =
                nkiDynStrCreate(vm, "Addition unimplemented for type ");
            nkiDynStrAppend(ds, nkiValueTypeGetName(type));
            nkiDynStrAppend(ds, ".");
            nkiAddError(
                vm,
                ds->data);
            nkiDynStrDelete(ds);
            return;
        }
    }
}

void nkiOpcode_pushLiteral_int(struct NKVM *vm)
{
    struct NKValue *stackVal = nkiVmStackPush_internal(vm);
    vm->currentExecutionContext->instructionPointer++;

    stackVal->type = NK_VALUETYPE_INT;
    stackVal->intData =
        vm->instructions[vm->currentExecutionContext->instructionPointer & vm->instructionAddressMask].opData_int;
}

void nkiOpcode_pushLiteral_float(struct NKVM *vm)
{
    struct NKValue *stackVal = nkiVmStackPush_internal(vm);
    vm->currentExecutionContext->instructionPointer++;

    stackVal->type = NK_VALUETYPE_FLOAT;
    stackVal->floatData =
        vm->instructions[vm->currentExecutionContext->instructionPointer & vm->instructionAddressMask].opData_float;
}

void nkiOpcode_pushLiteral_string(struct NKVM *vm)
{
    struct NKValue *stackVal = nkiVmStackPush_internal(vm);
    vm->currentExecutionContext->instructionPointer++;

    stackVal->type = NK_VALUETYPE_STRING;
    stackVal->stringTableEntry =
        vm->instructions[vm->currentExecutionContext->instructionPointer & vm->instructionAddressMask].opData_string;

}

void nkiOpcode_pushLiteral_functionId(struct NKVM *vm)
{
    struct NKValue *stackVal = nkiVmStackPush_internal(vm);
    vm->currentExecutionContext->instructionPointer++;

    stackVal->type = NK_VALUETYPE_FUNCTIONID;
    stackVal->functionId =
        vm->instructions[vm->currentExecutionContext->instructionPointer & vm->instructionAddressMask].opData_functionId;
}

void nkiOpcode_nop(struct NKVM *vm)
{
}

void nkiOpcode_subtract(struct NKVM *vm)
{
    struct NKValue *in2 = nkiVmStackPop(vm);
    struct NKValue *in1 = nkiVmStackPop(vm);

    enum NKValueType type = in1->type;

    switch(type) {

        case NK_VALUETYPE_INT:
            nkiVmStackPushInt(
                vm,
                in1->intData -
                nkiValueToInt(vm, in2));
            break;

        case NK_VALUETYPE_FLOAT:
            nkiVmStackPushFloat(
                vm,
                in1->floatData -
                nkiValueToFloat(vm, in2));
            break;

        default: {
            struct NKDynString *ds =
                nkiDynStrCreate(vm, "Subtraction unimplemented for type ");
            nkiDynStrAppend(ds, nkiValueTypeGetName(type));
            nkiDynStrAppend(ds, ".");
            nkiAddError(
                vm,
                ds->data);
            nkiDynStrDelete(ds);
        } break;
    }
}

void nkiOpcode_multiply(struct NKVM *vm)
{
    struct NKValue *in2 = nkiVmStackPop(vm);
    struct NKValue *in1 = nkiVmStackPop(vm);

    enum NKValueType type = in1->type;

    switch(type) {

        case NK_VALUETYPE_INT:
            nkiVmStackPushInt(
                vm,
                in1->intData *
                nkiValueToInt(vm, in2));
            break;

        case NK_VALUETYPE_FLOAT:
            nkiVmStackPushFloat(
                vm,
                in1->floatData *
                nkiValueToFloat(vm, in2));
            break;

        default: {
            struct NKDynString *ds =
                nkiDynStrCreate(vm, "Multiplication unimplemented for type ");
            nkiDynStrAppend(ds, nkiValueTypeGetName(type));
            nkiDynStrAppend(ds, ".");
            nkiAddError(
                vm,
                ds->data);
            nkiDynStrDelete(ds);
        } break;
    }
}

void nkiOpcode_divide(struct NKVM *vm)
{
    struct NKValue *in2 = nkiVmStackPop(vm);
    struct NKValue *in1 = nkiVmStackPop(vm);

    enum NKValueType type = in1->type;

    switch(type) {

        case NK_VALUETYPE_INT: {

            nkint32_t val2 = nkiValueToInt(vm, in2);

            // TIL there's more than one way to cause a SIGFPE with
            // integer division. Note: The constant is weird because
            // we can't represent -2147483648L on some compilers.
            if(val2 == 0 ||
                (val2 == -1 && in1->intData == -2147483647L - 1))
            {
                nkiAddError(
                    vm,
                    "Integer divide-by-zero or division overflow.");
            } else {
                nkiVmStackPushInt(
                    vm,
                    in1->intData /
                    val2);
            }
        } break;

        case NK_VALUETYPE_FLOAT:
            nkiVmStackPushFloat(
                vm,
                in1->floatData /
                nkiValueToFloat(vm, in2));
            break;

        default: {
            struct NKDynString *ds =
                nkiDynStrCreate(vm, "Division unimplemented for type ");
            nkiDynStrAppend(ds, nkiValueTypeGetName(type));
            nkiDynStrAppend(ds, ".");
            nkiAddError(
                vm,
                ds->data);
            nkiDynStrDelete(ds);
        } break;
    }
}

void nkiOpcode_modulo(struct NKVM *vm)
{
    struct NKValue *in2 = nkiVmStackPop(vm);
    struct NKValue *in1 = nkiVmStackPop(vm);

    enum NKValueType type = in1->type;

    switch(type) {

        case NK_VALUETYPE_INT: {
            nkint32_t val2 = nkiValueToInt(vm, in2);

            if(val2 == 0 || (
                    in1->intData == -2147483648L &&
                    val2 == -1))
            {
                nkiAddError(
                    vm,
                    "Integer divide-by-zero.");
            } else {
                nkiVmStackPushInt(
                    vm,
                    in1->intData %
                    val2);
            }
        } break;

        default: {
            struct NKDynString *ds =
                nkiDynStrCreate(vm, "Modulo unimplemented for type ");
            nkiDynStrAppend(ds, nkiValueTypeGetName(type));
            nkiDynStrAppend(ds, ".");
            nkiAddError(
                vm,
                ds->data);
            nkiDynStrDelete(ds);
        } break;
    }
}

void nkiOpcode_negate(struct NKVM *vm)
{
    struct NKValue *in1 = nkiVmStackPop(vm);

    enum NKValueType type = in1->type;

    switch(type) {

        case NK_VALUETYPE_INT: {
            nkiVmStackPushInt(
                vm,
                -(in1->intData));
        } break;

        case NK_VALUETYPE_FLOAT: {
            nkiVmStackPushFloat(
                vm,
                -(in1->floatData));
        } break;

        default: {
            struct NKDynString *ds =
                nkiDynStrCreate(vm, "Negation unimplemented for type ");
            nkiDynStrAppend(ds, nkiValueTypeGetName(type));
            nkiDynStrAppend(ds, ".");
            nkiAddError(
                vm,
                ds->data);
            nkiDynStrDelete(ds);
        } break;
    }
}

void nkiOpcode_pop(struct NKVM *vm)
{
    nkiVmStackPop(vm);
}

void nkiOpcode_popN(struct NKVM *vm)
{
    struct NKValue *v = nkiVmStackPop(vm);
    nkiVmStackPopN(vm, nkiValueToInt(vm, v));
}

void nkiOpcode_stackPeek(struct NKVM *vm)
{
    // Read index.
    struct NKValue *v = nkiVmStackPop(vm);
    if(v->type != NK_VALUETYPE_INT) {
        nkiAddError(vm,
            "Attempted to use a non-integer as a stack index.");
        return;
    }

    // Copy stack data over.
    if(v->intData >= 0) {

        // Absolute stack address. This was used for global variables,
        // but it might not be needed anymore now that they have their
        // own address space.
        struct NKValue *vIn = nkiVmStackPeek(vm, v->intData);
        struct NKValue *vOut = nkiVmStackPush_internal(vm);
        *vOut = *vIn;

    } else {

        // Negative stack address. Probably a local variable.
        nkuint32_t stackAddress = vm->currentExecutionContext->stack.size + v->intData;
        struct NKValue *vIn = nkiVmStackPeek(
            vm, stackAddress);
        struct NKValue *vOut = nkiVmStackPush_internal(vm);
        *vOut = *vIn;

    }
}

void nkiOpcode_stackPoke(struct NKVM *vm)
{
    // Read index.
    struct NKValue *stackAddrValue = nkiVmStackPop(vm);
    if(stackAddrValue->type != NK_VALUETYPE_INT) {
        nkiAddError(vm,
            "Attempted to use a non-integer as a stack index.");
        return;
    }

    // Copy to stack.
    if(stackAddrValue->intData >= 0) {

        // Absolute stack address. Probably a global variable.
        struct NKValue *vIn = nkiVmStackPeek(vm, (vm->currentExecutionContext->stack.size - 1));
        struct NKValue *vOut = nkiVmStackPeek(vm, stackAddrValue->intData);
        *vOut = *vIn;

    } else {

        // Negative stack address. Probably a local variable.
        nkuint32_t stackAddress = vm->currentExecutionContext->stack.size + stackAddrValue->intData;
        struct NKValue *vIn = nkiVmStackPeek(vm, (vm->currentExecutionContext->stack.size - 1));
        struct NKValue *vOut = nkiVmStackPeek(vm, stackAddress);
        *vOut = *vIn;

    }
}

void nkiOpcode_staticPeek(struct NKVM *vm)
{
    // Read index.
    struct NKValue *v = nkiVmStackPop(vm);
    if(v->type != NK_VALUETYPE_INT) {
        nkiAddError(vm,
            "Attempted to use a non-integer as a static index.");
        return;
    }

    {
        nkuint32_t staticAddress = v->intData & vm->staticAddressMask;
        struct NKValue *vIn = &vm->staticSpace[staticAddress];
        struct NKValue *vOut = nkiVmStackPush_internal(vm);
        *vOut = *vIn;
    }
}

void nkiOpcode_staticPoke(struct NKVM *vm)
{
    // Read index.
    struct NKValue *staticAddrValue = nkiVmStackPop(vm);
    if(staticAddrValue->type != NK_VALUETYPE_INT) {
        nkiAddError(vm,
            "Attempted to use a non-integer as a static index.");
        return;
    }

    {
        nkuint32_t staticAddr = staticAddrValue->intData & vm->staticAddressMask;
        struct NKValue *vIn = nkiVmStackPeek(vm, (vm->currentExecutionContext->stack.size - 1));
        struct NKValue *vOut = &vm->staticSpace[staticAddr];
        *vOut = *vIn;
    }
}

void nkiOpcode_jumpRelative(struct NKVM *vm)
{
    struct NKValue *offsetVal = nkiVmStackPop(vm);
    vm->currentExecutionContext->instructionPointer += nkiValueToInt(vm, offsetVal);
}

void nkiOpcode_call(struct NKVM *vm)
{
    // Expected stack state at start...
    //   _argumentCount
    //   <_argumentCount number of arguments>
    //   function id

    // // FIXME: Remove this.
    // printf("in %s\n", __FUNCTION__);
    // nkiVmStackDump(vm);

    nkuint32_t argumentCount = 0;
    NKVMInternalFunctionID functionId = { NK_INVALID_VALUE };
    struct NKVMFunction *funcOb = NULL;
    struct NKValue *callableObjectData = NULL;

    // PEEK at the top of the stack. That's _argumentCount.
    argumentCount = nkiValueToInt(vm, nkiVmStackPeek(vm, (vm->currentExecutionContext->stack.size - 1)));

    // PEEK at the function id (stack top - _argumentCount). Save it.
    {
        struct NKValue *functionIdValue = nkiVmStackPeek(
            vm, vm->currentExecutionContext->stack.size - (argumentCount + 2));

        nkuint32_t redirectionCount = 0;

        // If we have an object instead of a function, then this may
        // be a callable object. Look up the "_exec" field and "_data"
        // field.
        while(functionIdValue->type == NK_VALUETYPE_OBJECTID) {

            struct NKValue *objectIdValue = functionIdValue;
            struct NKValue index;

            // FIXME: Make this less arbitrary.
            redirectionCount++;
            if(redirectionCount > 10) {
                nkiAddError(
                    vm,
                    "Too many callable object redirections.");
                return;
            }

            // Check for the function call field.
            nkxValueSetString(vm, &index, "_exec");
            functionIdValue = nkiVmObjectFindOrAddEntry_public(
                vm, objectIdValue, &index, nktrue);

            // Check for the data field.
            nkxValueSetString(vm, &index, "_data");
            callableObjectData = nkiVmObjectFindOrAddEntry_public(
                vm, objectIdValue, &index, nktrue);

            if(callableObjectData) {

                nkuint32_t i = 0;
                nkbool overflow = nkfalse;

                NK_CHECK_OVERFLOW_UINT_ADD(argumentCount, 1, argumentCount, overflow);

                if(!overflow) {

                    // Make room for the _data contents on the stack.
                    nkiVmStackPush_internal(vm);

                    // Shift everything up the stack.
                    for(i = argumentCount; i >= 1; i--) {
                        vm->currentExecutionContext->stack.values[vm->currentExecutionContext->stack.size - argumentCount - 2 + i + 1] =
                            vm->currentExecutionContext->stack.values[vm->currentExecutionContext->stack.size - argumentCount - 2 + i];
                    }

                    // Insert the _data contents into the beginning of
                    // the stack.
                    vm->currentExecutionContext->stack.values[vm->currentExecutionContext->stack.size - argumentCount - 1] =
                        *callableObjectData;

                } else {
                    nkiAddError(
                        vm,
                        "Argument count overflow.");
                    return;
                }
            }
        }

        if(functionIdValue->type != NK_VALUETYPE_FUNCTIONID) {
            nkiAddError(
                vm,
                "Tried to call something that is not a function id or a callable object.");

            // FIXME: Remove this.
            nkiAddError(
                vm,
                nkiValueToString(vm, functionIdValue));

            return;
        }

        functionId = functionIdValue->functionId;
    }

    // Look up the function in our table of function objects.
    if(functionId.id >= vm->functionCount) {
        nkiAddError(
            vm,
            "Bad function id.");
        return;
    }
    funcOb = &vm->functionTable[functionId.id];

    // Compare _argumentCount to the stored function object's
    // _argumentCount. Throw an error if they mismatch.
    if(funcOb->argumentCount != NK_INVALID_VALUE &&
        funcOb->argumentCount != argumentCount)
    {
        nkiAddError(
            vm,
            "Incorrect argument count for function call.");
        return;
    }

    // At this point the behavior changes depending on if it's a C
    // function or script function.
    if(funcOb->externalFunctionId.id != NK_INVALID_VALUE) {

        // In this case we just want to call the C function, pop all
        // of our data off the stack, push the return value, and then
        // return from this function.

        struct NKVMFunctionCallbackData data;
        struct NKVMExternalFunction *externalFunc;

        nkiMemset(&data, 0, sizeof(data));

        // First check that this is a valid function. Maybe we go
        // weird data from deserialization.
        if(funcOb->externalFunctionId.id >= vm->externalFunctionCount) {
            nkiAddError(
                vm,
                "Tried to call a bad native function.");
            return;
        }

        externalFunc = &vm->externalFunctionTable[funcOb->externalFunctionId.id];

        if(!externalFunc->CFunctionCallback) {
            nkiAddError(
                vm,
                "Tried to call a null native function.");
            return;
        }

        // Fill in important stuff here.
        data.vm = vm;
        data.argumentCount = argumentCount;
        data.arguments = (struct NKValue *)nkiMallocArray(
            vm, sizeof(struct NKValue), argumentCount);

        // Note: We're not simply giving the function a stack pointer,
        // because then the called function would have to worry about
        // the stack index mask and all that crap.

        // Copy arguments over.
        {
            nkuint32_t i;
            for(i = 0; i < argumentCount; i++) {
                data.arguments[i] =
                    vm->currentExecutionContext->stack.values[
                        vm->currentExecutionContext->stack.indexMask &
                        ((vm->currentExecutionContext->stack.size - argumentCount - 1) + i)];
            }
        }

        // Check argument count and types.
        if(externalFunc->argumentCount != NK_INVALID_VALUE) {

            nkuint32_t i;

            // Check count.
            if(externalFunc->argumentCount != argumentCount) {
                struct NKDynString *dynStr = nkiDynStrCreate(vm,
                    "Tried to call an external function ");
                nkiDynStrAppend(dynStr, externalFunc->name);
                nkiDynStrAppend(dynStr, " with an incorrect number of arguments.");
                nkiAddError(vm, dynStr->data);
                nkiDynStrDelete(dynStr);
                nkiFree(vm, data.arguments);
                return;
            }

            // Check types.
            for(i = 0; i < argumentCount; i++) {

                // nil type is the "I don't care what it is or I'll
                // check in the function itself" indicator, because
                // it's pointless to have a function that you MUST
                // pass nil to for some parameter.
                if(externalFunc->argTypes[i] != NK_VALUETYPE_NIL) {

                    if(externalFunc->argTypes[i] != data.arguments[i].type) {
                        struct NKDynString *dynStr = nkiDynStrCreate(vm,
                            "Incorrect argument type in call to ");
                        nkiDynStrAppend(dynStr, externalFunc->name);
                        nkiDynStrAppend(dynStr, ". Expected ");
                        nkiDynStrAppend(dynStr, nkiValueTypeGetName(externalFunc->argTypes[i]));
                        nkiDynStrAppend(dynStr, " but got ");
                        nkiDynStrAppend(dynStr, nkiValueTypeGetName(data.arguments[i].type));
                        nkiDynStrAppend(dynStr, " for argument ");
                        nkiDynStrAppendUint32(dynStr, i);
                        nkiDynStrAppend(dynStr, ".");
                        nkiAddError(vm, dynStr->data);
                        nkiDynStrDelete(dynStr);
                        nkiFree(vm, data.arguments);
                        return;
                    }

                    if(externalFunc->argTypes[i] == NK_VALUETYPE_OBJECTID) {

                        // Check external type.
                        NKVMExternalDataTypeID expectedType = externalFunc->argExternalTypes[i];
                        struct NKVMObject *object = nkiVmObjectTableGetEntryById(
                            &vm->objectTable,
                            data.arguments[i].objectId);

                        if(object) {

                            if(expectedType.id != NK_INVALID_VALUE &&
                                expectedType.id != object->externalDataType.id)
                            {
                                struct NKDynString *dynStr = nkiDynStrCreate(vm,
                                    "Bad external object type passed to external function ");
                                nkiDynStrAppend(dynStr, externalFunc->name);
                                nkiDynStrAppend(dynStr, " for argument ");
                                nkiDynStrAppendUint32(dynStr, i);
                                nkiDynStrAppend(dynStr, ".");
                                nkiAddError(vm, dynStr->data);
                                nkiDynStrDelete(dynStr);
                                nkiFree(vm, data.arguments);
                                return;
                            }

                        } else {

                            struct NKDynString *dynStr = nkiDynStrCreate(vm,
                                "Bad object ID passed to external function ");
                            nkiDynStrAppend(dynStr, externalFunc->name);
                            nkiDynStrAppend(dynStr, " for argument ");
                            nkiDynStrAppendUint32(dynStr, i);
                            nkiDynStrAppend(dynStr, ".");
                            nkiAddError(vm, dynStr->data);
                            nkiDynStrDelete(dynStr);
                            nkiFree(vm, data.arguments);
                            return;
                        }
                    }
                }
            }
        }

        // Actual function call.
        if(externalFunc->CFunctionCallback) {
            externalFunc->CFunctionCallback(&data);
            nkiFree(vm, data.arguments);
        } else {
            nkiFree(vm, data.arguments);
            nkiAddError(vm, "Attempted to call a C function that doesn't exist.");
        }

        // If the C function called some code back inside the VM, we
        // may have suffered a catastrophic failure in that code, so
        // returning up through the stack in this side would not be a
        // good idea, because the VM may be in a bad state. Chain
        // another catastrophe event to get us all the way back to the
        // last user call.
        if(NK_CHECK_CATASTROPHE()) {
            NK_CATASTROPHE();
        }

        // Pop all the arguments.
        nkiVmStackPopN(vm, argumentCount + 2);

        // Push return value.
        {
            struct NKValue *retVal = nkiVmStackPush_internal(vm);
            *retVal = data.returnValue;
        }

    } else {

        // Call an internal function.

        // Push the current instruction pointer (_returnPointer).
        nkiVmStackPushInt(vm, vm->currentExecutionContext->instructionPointer);

        // printf("Pushed return pointer\n");
        // nkiVmStackDump(vm);

        // Set the instruction pointer to the saved function object's
        // instruction pointer. Minus one, because the instruction
        // pointer will be incremented when this function returns.
        vm->currentExecutionContext->instructionPointer = funcOb->firstInstructionIndex - 1;

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
    contextCountValue = nkiVmStackPop(vm);

    // Pop a value off the stack. That's _returnValue.
    returnValue = nkiVmStackPop(vm);

    // Pop contextCount more values off the stack and throw them away.
    // (Function context we're done with.)
    nkiVmStackPopN(vm, nkiValueToInt(vm, contextCountValue));

    // Pop the _returnPointer off the stack and keep it.
    returnAddress = nkiValueToInt(vm, nkiVmStackPop(vm));

    // Pop the _argumentCount off the stack and keep it.
    argumentCount = nkiValueToInt(vm, nkiVmStackPop(vm));

    // Pop _argumentCount values off the stack and throw them away.
    // (Function arguments we're done with.)
    nkiVmStackPopN(vm, argumentCount);

    // Pop function id off the stack.
    nkiVmStackPop(vm);

    // FIXME (COROUTINES): Check returnAddress validity. Possibly
    // switch up to parent context.

    // Push _returnValue back onto the stack.
    {
        struct NKValue *returnValueWrite = nkiVmStackPush_internal(vm);
        *returnValueWrite = *returnValue;
    }

    // Set the instruction pointer to _returnPointer - 1.
    vm->currentExecutionContext->instructionPointer = returnAddress;

    // Expected stack state at end...
    //   _returnValue
}

void nkiOpcode_end(struct NKVM *vm)
{
    // This doesn't actually do anything. The iteration loops know to
    // check for it and stop, though.

    // Stack data remaining after the end of a progam is an indicator
    // of a bug in the compiler, and a lot of these can be subtle and
    // really irritating bugs, so we need to call them out as soon as
    // we can.
    if(vm->currentExecutionContext->stack.size) {
        nkiAddError(
            vm,
            "Stack data remaining after end of program.");
    }

    // Okay, it does do one thing, and that is keeping itself on the
    // end instruction.
    vm->currentExecutionContext->instructionPointer--;
}

void nkiOpcode_jz(struct NKVM *vm)
{
    struct NKValue *relativeOffsetValue = nkiVmStackPop(vm);
    struct NKValue *testValue = nkiVmStackPop(vm);

    if(nkiValueToInt(vm, testValue) == 0) {
        vm->currentExecutionContext->instructionPointer += nkiValueToInt(vm, relativeOffsetValue);
    }
}

nkint32_t nkiOpcode_internal_compare(struct NKVM *vm)
{
    struct NKValue *in2 = nkiVmStackPop(vm);
    struct NKValue *in1 = nkiVmStackPop(vm);

    return nkiValueCompare(vm, in1, in2, nkfalse);
}

void nkiOpcode_gt(struct NKVM *vm)
{
    nkint32_t comparison = nkiOpcode_internal_compare(vm);
    nkiVmStackPushInt(vm, comparison == 1);
}

void nkiOpcode_lt(struct NKVM *vm)
{
    nkint32_t comparison = nkiOpcode_internal_compare(vm);
    nkiVmStackPushInt(vm, comparison == -1);
}

void nkiOpcode_ge(struct NKVM *vm)
{
    nkint32_t comparison = nkiOpcode_internal_compare(vm);
    nkiVmStackPushInt(vm, comparison == 0 || comparison == 1);
}

void nkiOpcode_le(struct NKVM *vm)
{
    nkint32_t comparison = nkiOpcode_internal_compare(vm);
    nkiVmStackPushInt(vm, comparison == 0 || comparison == -1);
}

void nkiOpcode_eq(struct NKVM *vm)
{
    nkint32_t comparison = nkiOpcode_internal_compare(vm);
    nkiVmStackPushInt(vm, comparison == 0);
}

void nkiOpcode_ne(struct NKVM *vm)
{
    nkint32_t comparison = nkiOpcode_internal_compare(vm);
    nkiVmStackPushInt(vm, comparison != 0);
}

void nkiOpcode_eqsametype(struct NKVM *vm)
{
    nkuint32_t stackSize = vm->currentExecutionContext->stack.size;
    nkbool sameTypes =
        nkiVmStackPeek(vm, stackSize - 1)->type ==
        nkiVmStackPeek(vm, stackSize - 2)->type;

    if(sameTypes) {
        nkint32_t comparison = nkiOpcode_internal_compare(vm);
        nkiVmStackPushInt(vm, comparison == 0);
    } else {
        nkiVmStackPopN(vm, 2);
        nkiVmStackPushInt(vm, 0);
    }
}

void nkiOpcode_not(struct NKVM *vm)
{
    nkiVmStackPushInt(vm, !nkiValueToInt(vm, nkiVmStackPop(vm)));
}

void nkiOpcode_and(struct NKVM *vm)
{
    // Do NOT try to inline these function calls in this expression. C
    // shortcutting will ruin your day.
    nkbool in1 = nkiValueToInt(vm, nkiVmStackPop(vm));
    nkbool in2 = nkiValueToInt(vm, nkiVmStackPop(vm));
    nkiVmStackPushInt(vm, in1 && in2);
}

void nkiOpcode_or(struct NKVM *vm)
{
    // Do NOT try to inline these function calls in this expression. C
    // shortcutting will ruin your day.
    nkbool in1 = nkiValueToInt(vm, nkiVmStackPop(vm));
    nkbool in2 = nkiValueToInt(vm, nkiVmStackPop(vm));
    nkiVmStackPushInt(vm, in1 || in2);
}

void nkiOpcode_createObject(struct NKVM *vm)
{
    struct NKValue *v = nkiVmStackPush_internal(vm);
    v->type = NK_VALUETYPE_OBJECTID;
    v->objectId = nkiVmObjectTableCreateObject(vm);
}

void nkiOpcode_objectFieldGet_internal(struct NKVM *vm, nkbool popObject)
{
    struct NKValue *indexToGet = nkiVmStackPop(vm);
    struct NKValue *objectToGet;
    struct NKValue *output;
    struct NKValue *objectValue;
    struct NKVMObject *ob;

    if(popObject) {
        objectToGet = nkiVmStackPop(vm);
    } else {
        objectToGet = nkiVmStackPeek(vm, vm->currentExecutionContext->stack.size - 1);
    }

    if(objectToGet->type != NK_VALUETYPE_OBJECTID) {
        nkiAddError(
            vm,
            "Attempted to get a field on a value that is not an object.");
        return;
    }

    ob = nkiVmObjectTableGetEntryById(&vm->objectTable, objectToGet->objectId);

    if(!ob) {
        nkiAddError(
            vm,
            "Bad object id in nkiOpcode_objectFieldGet.");
        return;
    }

    output = nkiVmStackPush_internal(vm);

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
    struct NKValue *indexToSet  = nkiVmStackPop(vm);
    struct NKValue *objectToSet = nkiVmStackPop(vm);
    struct NKValue *valueToSet  = nkiVmStackPop(vm);

    // TODO: Actually assign the value.
    struct NKValue *objectValue;
    struct NKVMObject *ob;

    if(objectToSet->type != NK_VALUETYPE_OBJECTID) {
        nkiAddError(
            vm,
            "Attempted to set a field on a value that is not an object.");
        return;
    }

    ob = nkiVmObjectTableGetEntryById(&vm->objectTable, objectToSet->objectId);

    if(!ob) {
        nkiAddError(
            vm,
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
    *nkiVmStackPush_internal(vm) = *valueToSet;
}

void nkiOpcode_prepareSelfCall(struct NKVM *vm)
{
    // In a "self" call ("foo.bar()") we have the object, function,
    // arguments, and argument count all on the stack (in that order).
    // But we need it to be the function, object, argument, and
    // argument count + 1. So we'll swap the function and object, then
    // add 1 to the argument count.

    struct NKValue *argumentCount = nkiVmStackPeek(vm, vm->currentExecutionContext->stack.size - 1);
    nkuint32_t stackOffset = nkiValueToInt(vm, argumentCount);
    struct NKValue *function = nkiVmStackPeek(vm, vm->currentExecutionContext->stack.size - (stackOffset + 3));
    struct NKValue *object = nkiVmStackPeek(vm, vm->currentExecutionContext->stack.size - (stackOffset + 2));
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
    struct NKValue *v = nkiVmStackPush_internal(vm);
    v->type = NK_VALUETYPE_NIL;
}

// FIXME: Remove this.
nkbool nopop = nkfalse;

void nkiOpcode_coroutineCreate(struct NKVM *vm)
{
    nkuint32_t i;

    struct NKVMExecutionContext *executionContext =
        nkxMalloc(vm, sizeof(struct NKVMExecutionContext));

    struct NKVMExecutionContext *parentExecutionContext =
        vm->currentExecutionContext;

    struct NKValue argCountValue = *nkiVmStackPop(vm);
    struct NKValue functionValue = {0};

    nkint32_t argCount = nkiValueToInt(vm, &argCountValue);

    if(argCount < 0) {
        nkiAddError(vm, "Got negative argument count in coroutine creation.");
        return;
    }

    if(argCount > parentExecutionContext->stack.size) {
        nkiAddError(vm, "Argument count bigger than stack in coroutine creation.");
        return;
    }

    // Save all the arguments.
    struct NKValue *arguments = nkiMallocArray(
        vm, sizeof(struct NKValue), argCount);
    // FIXME: Check over/underflow.
    nkiMemcpy(
        arguments,
        parentExecutionContext->stack.values + (parentExecutionContext->stack.size - argCount),
        sizeof(struct NKValue) * argCount);

    // Remove them from the stack.
    nkiVmStackPopN(vm, argCount);

    // Save the function itself.
    struct NKValue *functionValuePtr = nkiVmStackPop(vm);
    if(functionValuePtr) {
        functionValue = *functionValuePtr;
    }

    // Create the coroutine object to leave on the stack.
    struct NKValue *v = nkiVmStackPush_internal(vm);

    nkiVmInitExecutionContext(vm, executionContext);

    v->type = NK_VALUETYPE_OBJECTID;
    v->objectId = nkiVmObjectTableCreateObject(vm);

    executionContext->coroutineObject = *v;

    nkiVmObjectSetExternalType(
        vm, &executionContext->coroutineObject,
        nkiVmFindExternalType(vm, "coroutine"));

    nkxVmObjectSetExternalData(
        vm, &executionContext->coroutineObject,
        executionContext);

    // Switch to the new context.
    nkxpVmPushExecutionContext(
        vm, executionContext);

    // Copy function ID to the new stack.
    *nkiVmStackPush_internal(vm) = functionValue;

    // Copy all the arguments into the new stack.
    for(i = 0; i < argCount; i++) {
        *nkiVmStackPush_internal(vm) =
            arguments[i];
    }

    // Copy the argument count into the new stack.
    nkiVmStackPushInt(vm, argCount);

    // FIXME (COROUTINES): Set the IP to a magic value so we know what
    // to return to.
    nkiOpcode_call(vm);

    // This is being done OUTSIDE of normal iteration, so we need to
    // advance the IP ourselves!
    vm->currentExecutionContext->instructionPointer++;

    // FIXME: Remove this.
    for(i = 0; i < 100; i++) {
        nkiVmIterate(vm);
    }

    // Switch back to the original context.
    nkxpVmPopExecutionContext(vm);

    nkiFree(vm, arguments);
}

void nkiOpcode_coroutineYield(struct NKVM *vm)
{
    // FIXME (COROUTINES): Make this actually return the yielded
    // value. Also, switch contexts and stuff.
    struct NKValue *v = nkiVmStackPush_internal(vm);
    v->type = NK_VALUETYPE_NIL;
}

void nkiOpcode_coroutineResume(struct NKVM *vm)
{
    // FIXME (COROUTINES): Make this actually return the value passed
    // in through resume(). Also, switch contexts and stuff.
    struct NKValue *v = nkiVmStackPush_internal(vm);
    v->type = NK_VALUETYPE_NIL;
}


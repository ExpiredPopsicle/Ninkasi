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
//   By Cliff "ExpiredPopsicle" Jolly
//     https://expiredpopsicle.com
//     https://intoxicoding.com
//     expiredpopsicle@gmail.com
//
// ----------------------------------------------------------------------
//
//   Copyright (c) 2017 Clifford Jolly
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
                vm, -1,
                ds->data);
            nkiDynStrDelete(ds);
            return;
        }
    }
}

void nkiOpcode_pushLiteral_int(struct NKVM *vm)
{
    struct NKValue *stackVal = nkiVmStackPush_internal(vm);
    vm->instructionPointer++;

    stackVal->type = NK_VALUETYPE_INT;
    stackVal->intData =
        vm->instructions[vm->instructionPointer & vm->instructionAddressMask].opData_int;
}

void nkiOpcode_pushLiteral_float(struct NKVM *vm)
{
    struct NKValue *stackVal = nkiVmStackPush_internal(vm);
    vm->instructionPointer++;

    stackVal->type = NK_VALUETYPE_FLOAT;
    stackVal->floatData =
        vm->instructions[vm->instructionPointer & vm->instructionAddressMask].opData_float;
}

void nkiOpcode_pushLiteral_string(struct NKVM *vm)
{
    struct NKValue *stackVal = nkiVmStackPush_internal(vm);
    vm->instructionPointer++;

    stackVal->type = NK_VALUETYPE_STRING;
    stackVal->stringTableEntry =
        vm->instructions[vm->instructionPointer & vm->instructionAddressMask].opData_string;

    nkiDbgWriteLine("Pushing literal string: %d = %d", vm->instructionPointer, stackVal->stringTableEntry);

}

void nkiOpcode_pushLiteral_functionId(struct NKVM *vm)
{
    struct NKValue *stackVal = nkiVmStackPush_internal(vm);
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
                vm, -1,
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
                vm, -1,
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
            if(val2 == 0) {
                nkiAddError(
                    vm, -1,
                    "Integer divide-by-zero.");
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
                vm, -1,
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
            if(val2 == 0) {
                nkiAddError(
                    vm, -1,
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
                vm, -1,
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
                vm, -1,
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
        nkiAddError(vm, -1,
            "Attempted to use a non-integer as a stack index.");
        return;
    }

    // Copy stack data over.
    if(v->intData >= 0) {

        // Absolute stack address. Probably a global variable.
        nkuint32_t stackAddress = v->intData;
        struct NKValue *vIn = nkiVmStackPeek(vm, v->intData);
        struct NKValue *vOut = nkiVmStackPush_internal(vm);
        *vOut = *vIn;

        nkiDbgWriteLine("Fetched global value at stack position: %u", stackAddress);

    } else {

        // Negative stack address. Probably a local variable.
        nkuint32_t stackAddress = vm->stack.size + v->intData;
        struct NKValue *vIn = nkiVmStackPeek(
            vm, stackAddress);
        struct NKValue *vOut = nkiVmStackPush_internal(vm);
        *vOut = *vIn;

        nkiDbgWriteLine("Fetched local value at stack position: %u", stackAddress);

    }
}

void nkiOpcode_stackPoke(struct NKVM *vm)
{
    // Read index.
    struct NKValue *stackAddrValue = nkiVmStackPop(vm);
    if(stackAddrValue->type != NK_VALUETYPE_INT) {
        nkiAddError(vm, -1,
            "Attempted to use a non-integer as a stack index.");
        return;
    }

    // Copy to stack.
    if(stackAddrValue->intData >= 0) {

        // Absolute stack address. Probably a global variable.
        nkuint32_t stackAddress = stackAddrValue->intData;
        struct NKValue *vIn = nkiVmStackPeek(vm, (vm->stack.size - 1));
        struct NKValue *vOut = nkiVmStackPeek(vm, stackAddrValue->intData);
        *vOut = *vIn;

        nkiDbgWriteLine("Set global value at stack position: %u", stackAddress);

    } else {

        // Negative stack address. Probably a local variable.
        nkuint32_t stackAddress = vm->stack.size + stackAddrValue->intData;
        struct NKValue *vIn = nkiVmStackPeek(vm, (vm->stack.size - 1));
        struct NKValue *vOut = nkiVmStackPeek(vm, stackAddress);
        *vOut = *vIn;

        nkiDbgWriteLine("Set local value at stack position: %u", stackAddress);
    }
}

void nkiOpcode_staticPeek(struct NKVM *vm)
{
    // Read index.
    struct NKValue *v = nkiVmStackPop(vm);
    if(v->type != NK_VALUETYPE_INT) {
        nkiAddError(vm, -1,
            "Attempted to use a non-integer as a static index.");
        return;
    }

    {
        nkuint32_t staticAddress = v->intData & vm->staticAddressMask;
        struct NKValue *vIn = &vm->staticSpace[staticAddress];
        struct NKValue *vOut = nkiVmStackPush_internal(vm);
        *vOut = *vIn;
        nkiDbgWriteLine("Fetched global value at static position: %u", staticAddress);
    }
}

void nkiOpcode_staticPoke(struct NKVM *vm)
{
    // Read index.
    struct NKValue *staticAddrValue = nkiVmStackPop(vm);
    if(staticAddrValue->type != NK_VALUETYPE_INT) {
        nkiAddError(vm, -1,
            "Attempted to use a non-integer as a static index.");
        return;
    }

    {
        nkuint32_t staticAddr = staticAddrValue->intData & vm->staticAddressMask;
        struct NKValue *vIn = nkiVmStackPeek(vm, (vm->stack.size - 1));
        struct NKValue *vOut = &vm->staticSpace[staticAddr];
        *vOut = *vIn;
        nkiDbgWriteLine("Set global value at static position: %u", staticAddr);
    }
}

void nkiOpcode_jumpRelative(struct NKVM *vm)
{
    struct NKValue *offsetVal = nkiVmStackPop(vm);
    vm->instructionPointer += nkiValueToInt(vm, offsetVal);
}

void nkiOpcode_call(struct NKVM *vm)
{
    // Expected stack state at start...
    //   _argumentCount
    //   <_argumentCount number of arguments>
    //   function id

    nkuint32_t argumentCount = 0;
    NKVMInternalFunctionID functionId = { NK_INVALID_VALUE };
    struct NKVMFunction *funcOb = NULL;

    // PEEK at the top of the stack. That's _argumentCount.
    argumentCount = nkiValueToInt(vm, nkiVmStackPeek(vm, (vm->stack.size - 1)));

    nkiDbgWriteLine("Calling function with argument count: %u", argumentCount);

    // PEEK at the function id (stack top - _argumentCount). Save it.
    {
        struct NKValue *functionIdValue = nkiVmStackPeek(
            vm, vm->stack.size - (argumentCount + 2));

        if(functionIdValue->type != NK_VALUETYPE_FUNCTIONID) {
            nkiAddError(
                vm,
                -1,
                "Tried to call something that is not a function id.");
            return;
        }

        functionId = functionIdValue->functionId;
        nkiDbgWriteLine("Calling function with id: %u", functionId);
    }

    // Look up the function in our table of function objects.
    if(functionId.id >= vm->functionCount) {
        nkiAddError(
            vm,
            -1,
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
            -1,
            "Incorrect argument count for function call.");

        nkiDbgWriteLine("funcOb->argumentCount: %u", funcOb->argumentCount);
        nkiDbgWriteLine("argumentCount:         %u", argumentCount);

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

        memset(&data, 0, sizeof(data));

        // First check that this is a valid function. Maybe we go
        // weird data from deserialization.
        if(funcOb->externalFunctionId.id >= vm->externalFunctionCount) {
            nkiAddError(vm, -1,
                "Tried to call a bad native function.");
            return;
        }
        externalFunc = &vm->externalFunctionTable[funcOb->externalFunctionId.id];
        if(!externalFunc->CFunctionCallback) {
            nkiAddError(vm, -1,
                "Tried to call a null native function.");
            return;
        }

        // Fill in important stuff here.
        data.vm = vm;
        data.argumentCount = argumentCount;
        data.arguments = nkiMallocArray(vm, sizeof(struct NKValue), argumentCount);

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
        externalFunc->CFunctionCallback(&data);

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
        nkiVmStackPopN(vm, argumentCount + 2);

        // Push return value.
        {
            struct NKValue *retVal = nkiVmStackPush_internal(vm);
            *retVal = data.returnValue;
        }

    } else {

        // Push the current instruction pointer (_returnPointer).
        nkiVmStackPushInt(vm, vm->instructionPointer);

        // Set the instruction pointer to the saved function object's
        // instruction pointer. Minus one, because the instruction
        // pointer will be incremented when this function returns.
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

    // Push _returnValue back onto the stack.
    {
        struct NKValue *returnValueWrite = nkiVmStackPush_internal(vm);
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
    struct NKValue *relativeOffsetValue = nkiVmStackPop(vm);
    struct NKValue *testValue = nkiVmStackPop(vm);

    nkiDbgWriteLine("Testing branch value %d. Address now: %u", nkiValueToInt(vm, testValue), vm->instructionPointer);
    if(nkiValueToInt(vm, testValue) == 0) {
        vm->instructionPointer += nkiValueToInt(vm, relativeOffsetValue);
        nkiDbgWriteLine("Branch taken. Address now: %u", vm->instructionPointer);
    } else {
        nkiDbgWriteLine("Branch NOT taken. Address now: %u", vm->instructionPointer);
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
    nkuint32_t stackSize = vm->stack.size;
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
        objectToGet = nkiVmStackPeek(vm, vm->stack.size - 1);
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
    *nkiVmStackPush_internal(vm) = *valueToSet;
}

void nkiOpcode_prepareSelfCall(struct NKVM *vm)
{
    struct NKValue *argumentCount = nkiVmStackPeek(vm, vm->stack.size - 1);
    nkuint32_t stackOffset = nkiValueToInt(vm, argumentCount);
    struct NKValue *function = nkiVmStackPeek(vm, vm->stack.size - (stackOffset + 3));
    struct NKValue *object = nkiVmStackPeek(vm, vm->stack.size - (stackOffset + 2));
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

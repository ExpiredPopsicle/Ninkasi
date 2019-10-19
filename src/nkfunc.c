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

struct NKVMFunction *nkiVmCreateFunction(
    struct NKVM *vm, NKVMInternalFunctionID *functionId)
{
    if(functionId) {
        functionId->id = vm->functionCount++;
    }

    vm->functionTable = (struct NKVMFunction *)nkiReallocArray(
        vm,
        vm->functionTable,
        sizeof(struct NKVMFunction), vm->functionCount);

    nkiMemset(
        &vm->functionTable[vm->functionCount - 1], 0,
        sizeof(struct NKVMFunction));

    vm->functionTable[vm->functionCount - 1].externalFunctionId.id = NK_INVALID_VALUE;

    return &vm->functionTable[vm->functionCount - 1];
}

void nkiVmCallFunction(
    struct NKVM *vm,
    struct NKValue *functionValue,
    nkuint32_t argumentCount,
    struct NKValue *arguments,
    struct NKValue *returnValue)
{
    if(functionValue->type != NK_VALUETYPE_FUNCTIONID) {
        nkiAddError(vm, "Tried to call a non-function with nkiVmCallFunction.");
        return;
    }

    {
        // Save whatever the real instruction pointer was so that we
        // can restore it after we're done with this artificially
        // injected function call.
        nkuint32_t oldInstructionPtr = vm->currentExecutionContext->instructionPointer;
        nkuint32_t i;

        // Push the function ID itself onto the stack.
        *nkiVmStackPush_internal(vm) = *functionValue;

        // Push arguments.
        for(i = 0; i < argumentCount; i++) {
            *nkiVmStackPush_internal(vm) = arguments[i];
        }

        // Push argument count.
        nkiVmStackPushInt(vm, argumentCount);

        // Set the instruction pointer to a special return value we
        // use for external calls into the VM. This will push a
        // NK_UINT_MAX-1 pointer onto the stack, which the VM will
        // return to after the function is called. Once it returns and
        // it moves to the next instruction, we'll check for the
        // instruction pointer to equal NK_UINT_MAX, indicating that
        // the function call is complete.
        vm->currentExecutionContext->instructionPointer = (NK_UINT_MAX - 1);

        // Execute the call instruction. When this returns we will
        // either be inside the function in the VM, or we will have
        // just returned from the C function the function ID is
        // associated with. Or an error.
        nkiOpcode_call(vm);
        if(vm->errorState.firstError) {
            return;
        }

        // The call opcode sets us in a position right before the
        // start of the function, because the instruction pointer will
        // be incremented after the call opcode returns during the
        // normal instruction iteration process. We have to mimic that
        // normal instruction iteration process as long as we're using
        // the opcode directly.
        vm->currentExecutionContext->instructionPointer++;

        // If the function call was a C function, we should have our
        // instruction pointer at NK_UINT_MAX right now. Otherwise
        // it'll be at the start of the function, with a return
        // pointer of NK_UINT_MAX-1. So to execute instructions util
        // we return, we'll just keep executing until we hit an error,
        // or the instruction pointer is at NK_UINT_MAX
        // (NK_UINT_MAX-1, +1 for the instruction iteration).
        while(vm->currentExecutionContext->instructionPointer != NK_UINT_MAX &&
            !vm->errorState.firstError)
        {
            nkiVmIterate(vm);
        }

        // Save return value.
        if(returnValue) {
            *returnValue = *nkiVmStackPop(vm);
        } else {
            nkiVmStackPop(vm);
        }

        // Restore old state.
        vm->currentExecutionContext->instructionPointer = oldInstructionPtr;
    }
}

NKVMExternalFunctionID nkiVmRegisterExternalFunction(
    struct NKVM *vm,
    const char *name,
    NKVMFunctionCallback func)
{
    // FIXME: DO NOT ALLOW THIS TO RUN AFTER THE VM IS FINALIZED!

    // Lookup function first, to make sure we aren't making duplicate
    // functions. (We're probably at compile time right now so we can
    // spend some time searching for this.)
    NKVMExternalFunctionID externalFunctionId = { NK_INVALID_VALUE };
    for(externalFunctionId.id = 0; externalFunctionId.id < vm->externalFunctionCount; externalFunctionId.id++) {
        if(vm->externalFunctionTable[externalFunctionId.id].CFunctionCallback == func &&
            !nkiStrcmp(vm->externalFunctionTable[externalFunctionId.id].name, name))
        {
            break;
        }
    }

    if(externalFunctionId.id == vm->externalFunctionCount) {
        // Function not found. Allocate a new one.
        return nkiVmRegisterExternalFunctionNoSearch(vm, name, func);
    }

    return externalFunctionId;
}

NKVMExternalFunctionID nkiVmRegisterExternalFunctionNoSearch(
    struct NKVM *vm,
    const char *name,
    NKVMFunctionCallback func)
{
    struct NKVMExternalFunction *funcEntry;

    vm->externalFunctionCount++;
    if(!vm->externalFunctionCount) {
        vm->externalFunctionCount--;
        nkiAddError(vm, "Too many external functions registered.");
        {
            NKVMExternalFunctionID ret = { NK_INVALID_VALUE };
            return ret;
        }
    }

    vm->externalFunctionTable = (struct NKVMExternalFunction *)nkiReallocArray(
        vm, vm->externalFunctionTable,
        vm->externalFunctionCount, sizeof(struct NKVMExternalFunction));

    funcEntry = &vm->externalFunctionTable[vm->externalFunctionCount - 1];
    nkiMemset(funcEntry, 0, sizeof(*funcEntry));
    funcEntry->internalFunctionId.id = NK_INVALID_VALUE;
    funcEntry->name = nkiStrdup(vm, name);
    funcEntry->CFunctionCallback = func;
    funcEntry->argumentCount = NK_INVALID_VALUE;
    funcEntry->argTypes = NULL;
    funcEntry->argExternalTypes = NULL;

    {
        NKVMExternalFunctionID ret;
        ret.id = vm->externalFunctionCount - 1;
        return ret;
    }
}

NKVMInternalFunctionID nkiVmGetOrCreateInternalFunctionForExternalFunction(
    struct NKVM *vm, NKVMExternalFunctionID externalFunctionId)
{
    NKVMInternalFunctionID functionId = { NK_INVALID_VALUE };

    if(externalFunctionId.id >= vm->externalFunctionCount) {
        nkiAddError(vm, "Tried to create an internal function to represent a bad external function.");
        return functionId;
    }

    if(vm->externalFunctionTable[externalFunctionId.id].internalFunctionId.id == NK_INVALID_VALUE) {

        // Gotta make a new function. Nothing exists yet.
        struct NKVMFunction *vmfunc =
            nkiVmCreateFunction(vm, &functionId);

        nkiMemset(vmfunc, 0, sizeof(*vmfunc));
        vmfunc->argumentCount = NK_INVALID_VALUE;

        // Set up function ID mappings.
        vmfunc->externalFunctionId = externalFunctionId;
        vm->externalFunctionTable[externalFunctionId.id].internalFunctionId = functionId;

    } else {

        // Some function already exists in the VM with this external
        // function ID.
        functionId = vm->externalFunctionTable[externalFunctionId.id].internalFunctionId;
    }

    return functionId;
}


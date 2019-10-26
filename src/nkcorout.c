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

struct NKVMCoroutineInternalData
{
    NKVMExternalDataTypeID coroutineTypeId;
};

void nkxCoroutineLibrary_libraryCleanup(struct NKVM *vm, void *internalData)
{
    if(internalData) {
        nkxFree(vm, internalData);
    }
}

void nkxCoroutineLibrary_librarySerialize(struct NKVM *vm, void *internalData)
{
    // I don't think there's anything we actually need to do here.
}

void nkxCoroutineLibrary_coroutineGCMark(
    struct NKVM *vm, struct NKValue *value,
    void *internalData, struct NKVMGCState *gcState)
{
    struct NKVMExecutionContext *context =
        (struct NKVMExecutionContext *)internalData;
    nkuint32_t i;

    assert(context->parent);

    // Mark the parent (and let its own coroutineGCMark run at the
    // appropriate time). Don't process all parents.
    nkxVmGarbageCollect_markValue(
        vm, gcState, &context->coroutineObject);

    // Mark the entire stack.
    for(i = 0; i < context->stack.size; i++) {
        nkxVmGarbageCollect_markValue(
            vm, gcState, &context->stack.values[i]);
    }
}

void nkxCoroutineLibrary_coroutineGCData(
    struct NKVM *vm, struct NKValue *val, void *data)
{
    struct NKVMExecutionContext *context =
        (struct NKVMExecutionContext *)data;

    nkxpVmDeinitExecutionContext(vm, context);
    nkxFree(vm, context);
}

void nkxCoroutineLibrary_coroutineSerializeData(
    struct NKVM *vm,
    struct NKValue *objectValue,
    void *data)
{
    // TODO (COROUTINES): Serialize the whole stack.

    // TODO (COROUTINES): Serialize the IP.
}

void nkxCoroutineLibrary_coroutineCreate(struct NKVMFunctionCallbackData *data)
{
    struct NKVMCoroutineInternalData *internalData;
    struct NKVM *vm = data->vm;

    internalData =
        (struct NKVMCoroutineInternalData *)nkxGetExternalSubsystemDataOrError(
            data->vm, "coroutines");

    if(data->argumentCount < 1) {
        nkxAddError(vm, "Coroutine creation missing function argument.");
        return;
    }

    if(internalData) {

        // TODO (COROUTINES): Finish this.

        //nkxVmFindExternalType?

        struct NKVMExecutionContext *executionContext =
            nkxMalloc(vm, sizeof(struct NKVMExecutionContext));

        if(executionContext) {

            nkuint32_t i;

            nkxpVmInitExecutionContext(vm, executionContext);

            nkxCreateObject(vm, &executionContext->coroutineObject);

            nkxVmObjectSetExternalType(
                vm, &executionContext->coroutineObject,
                internalData->coroutineTypeId);

            nkxVmObjectSetExternalData(
                vm, &executionContext->coroutineObject,
                executionContext);

            nkxpVmPushExecutionContext(
                vm, executionContext);

            // Push the function.
            nkxpVmStackPushValue(
                vm, &data->arguments[0]);

            // Push all arguments.
            for(i = 1; i < data->argumentCount; i++) {
                nkxpVmStackPushValue(
                    vm, &data->arguments[i]);
            }

            // Push argument count.
            //
            // FIXME (COROUTINES): Check overflow. (uint to int
            // conversion)
            {
                struct NKValue argCount;
                nkxValueSetInt(vm, &argCount, data->argumentCount - 1);
                nkxpVmStackPushValue(
                    vm, &argCount);
            }

            // FIXME (COROUTINES): Set IP to like a magic value or
            // something?

            // FIXME (COROUTINES): Wrap in nkxp.
            nkiOpcode_call(vm);

            // Switch back to the original context.
            nkxpVmPopExecutionContext(vm);
        }

        // TODO: Figure out what we're going to do when we hit the
        // invebitable return statement.

        data->returnValue = executionContext->coroutineObject;
    }
}

void nkxCoroutineLibrary_coroutineResume(struct NKVMFunctionCallbackData *data)
{
    struct NKVMExecutionContext *context =
        (struct NKVMExecutionContext *)nkxVmObjectGetExternalData(
            data->vm, &data->arguments[0]);

    nkxpVmPushExecutionContext(data->vm, context);

    // FIXME (COROUTINES): Push some junk onto the stack to make up
    // for ascklmdc;kmasc;klsmc TLDR there's shit on the stack that's
    // messing stuff up when this returns. It's probably our own
    // return value.
}

void nkxCoroutineLibrary_coroutineYield(struct NKVMFunctionCallbackData *data)
{
    if(data->argumentCount > 0) {
        data->returnValue = data->arguments[0];
    }

    if(data->argumentCount > 1) {
        nkxAddError(data->vm, "Extra arguments in yield call.");
    }

    nkxpVmPopExecutionContext(data->vm);
}

void nkxCoroutineLibrary_init(struct NKVM *vm, struct NKCompilerState *cs)
{
    struct NKVMCoroutineInternalData *internalData =
        (struct NKVMCoroutineInternalData*)nkxMalloc(
            vm,
            sizeof(struct NKVMCoroutineInternalData));

    internalData->coroutineTypeId.id = NK_INVALID_VALUE;

    if(!nkxInitSubsystem(
            vm, cs, "coroutines",
            internalData,
            nkxCoroutineLibrary_libraryCleanup,
            nkxCoroutineLibrary_librarySerialize))
    {
        nkxFree(vm, internalData);
        return;
    }

    internalData->coroutineTypeId = nkxVmRegisterExternalType(
        vm, "coroutine",
        nkxCoroutineLibrary_coroutineSerializeData,
        nkxCoroutineLibrary_coroutineGCData,
        nkxCoroutineLibrary_coroutineGCMark);

    if(internalData->coroutineTypeId.id == NK_INVALID_VALUE) {
        nkxFree(vm, internalData);
        return;
    }

    nkxVmSetupExternalFunction(
        vm, cs, "coroutine_create",
        nkxCoroutineLibrary_coroutineCreate,
        nktrue,
        // Arbitrary argument count so we can pass a ton of stuff to
        // the function. First argument must be a function, though.
        NK_INVALID_VALUE);

    nkxVmSetupExternalFunction(
        vm, cs, "coroutine_resume",
        nkxCoroutineLibrary_coroutineResume,
        nktrue,
        1,
        NK_VALUETYPE_OBJECTID, internalData->coroutineTypeId);

    nkxVmSetupExternalFunction(
        vm, cs, "coroutine_yield",
        nkxCoroutineLibrary_coroutineYield,
        nktrue,
        NK_INVALID_VALUE); // 1 or 0 things to yield back
}




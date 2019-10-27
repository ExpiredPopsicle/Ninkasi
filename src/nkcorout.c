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

void nkxCoroutineLibrary_librarySerialize(struct NKVM *vm, void *internalData)
{
    // I don't think there's anything we actually need to do here.
}

void nkiCoroutineLibrary_coroutineGCMark(
    struct NKVM *vm, struct NKValue *value,
    void *internalData, struct NKVMGCState *gcState)
{
    struct NKVMExecutionContext *context =
        (struct NKVMExecutionContext *)internalData;
    nkuint32_t i;

    // assert(context->parent);

    // FIXME: Remove this.
    printf("GC MARKING COROUTINE\n");

    // Mark the parent (and let its own coroutineGCMark run at the
    // appropriate time). Don't process all parents.
    if(context->parent) {
        nkiVmGarbageCollect_markValue(
            gcState, &context->parent->coroutineObject);
    }

    // Mark the entire stack.
    for(i = 0; i < context->stack.size; i++) {
        nkiVmGarbageCollect_markValue(
            gcState,
            &context->stack.values[i]);
    }
}

void nkiCoroutineLibrary_coroutineGCData(
    struct NKVM *vm, struct NKValue *val, void *data)
{
    struct NKVMExecutionContext *context =
        (struct NKVMExecutionContext *)data;

    nkiVmDeinitExecutionContext(vm, context);
    nkiFree(vm, context);
}

void nkiCoroutineLibrary_coroutineSerializeData(
    struct NKVM *vm,
    struct NKValue *objectValue,
    void *data)
{
    // TODO (COROUTINES): Serialize the whole stack.

    // TODO (COROUTINES): Serialize the IP.
}

void nkxCoroutineLibrary_init(struct NKVM *vm)
{
    // FIXME (COROUTINES): Add this to VM state instead of some
    // external system data.
    nkxVmRegisterExternalType(
        vm, "coroutine",
        nkiCoroutineLibrary_coroutineSerializeData,
        nkiCoroutineLibrary_coroutineGCData,
        nkiCoroutineLibrary_coroutineGCMark);
}

void nkiVmPopExecutionContext(
    struct NKVM *vm)
{
    struct NKVMExecutionContext *context =
        vm->currentExecutionContext;

    // FIXME (DONOTCHECKIN): Make this a runtime error.
    assert(context->parent);

    vm->currentExecutionContext =
        context->parent;

    context->parent = NULL;
}

void nkiVmPushExecutionContext(
    struct NKVM *vm,
    struct NKVMExecutionContext *context)
{
    // FIXME (DONOTCHECKIN): Make this a runtime error.
    assert(!context->parent);

    context->parent = vm->currentExecutionContext;
    vm->currentExecutionContext = context;
}


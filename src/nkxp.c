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

void nkxpVmInitExecutionContext(
    struct NKVM *vm,
    struct NKVMExecutionContext *context)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiVmInitExecutionContext(vm, context);
    NK_CLEAR_FAILURE_RECOVERY();
}

void nkxpVmDeinitExecutionContext(
    struct NKVM *vm,
    struct NKVMExecutionContext *context)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiVmDeinitExecutionContext(vm, context);
    NK_CLEAR_FAILURE_RECOVERY();
}

// FIXME (COROUTINES): make internal
void nkxpVmPushExecutionContext(
    struct NKVM *vm,
    struct NKVMExecutionContext *context)
{
    // FIXME (DONOTCHECKIN): Make this a runtime error.
    assert(!context->parent);

    context->parent = vm->currentExecutionContext;
    vm->currentExecutionContext = context;
}

// FIXME (COROUTINES): make internal
void nkxpVmPopExecutionContext(
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

void nkxpVmStackPushValue(
    struct NKVM *vm,
    struct NKValue *value)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    *nkiVmStackPush_internal(vm) = *value;
    NK_CLEAR_FAILURE_RECOVERY();
}



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

void nkiVmStackInit(struct NKVM *vm)
{
    struct NKVMStack *stack = &vm->stack;
    stack->values = nkiMalloc(vm, sizeof(struct NKValue));
    memset(stack->values, 0, sizeof(struct NKValue));
    stack->size = 0;
    stack->capacity = 1;
    stack->indexMask = 0;
}

void nkiVmStackDestroy(struct NKVM *vm)
{
    struct NKVMStack *stack = &vm->stack;
    nkiFree(vm, stack->values);
    memset(stack, 0, sizeof(struct NKVMStack));
}

struct NKValue *nkiVmStackPush_internal(struct NKVM *vm)
{
    struct NKVMStack *stack = &vm->stack;

    // Make a stack if we don't have one. This can happen with weird
    // binaries.
    if(!stack->capacity) {
        stack->capacity = 1;
        stack->values = nkiReallocArray(
            vm,
            stack->values,
            stack->capacity, sizeof(struct NKValue));
        memset(stack->values, 0, sizeof(struct NKValue));
    }

    // Grow the stack if necessary.
    if(stack->size == stack->capacity) {

        // TODO: Add an adjustable stack size limit.
        if(stack->capacity > 0xffff) {

            nkiAddError(
                vm, -1,
                "Stack overflow.");

            return &stack->values[0];
        }

        // Thanks AFL! Continued attempts to keep pushing stuff to the
        // stack (even though the stack overflow had occurred) would
        // cause the stack size to keep doubling anyway, but without
        // any actual allocation. Now the stack capacity change
        // happens AFTER the overflow check.
        stack->capacity <<= 1;

        // TODO: Make a more reasonable stack space limit than "we ran
        // out of 32-bit integer".
        if(!stack->capacity) {
            nkiAddError(
                vm, -1,
                "Stack ran out of address space.");
            return &stack->values[0];
        }

        if(stack->capacity > vm->limits.maxStackSize) {
            nkiAddError(
                vm, -1,
                "Reached stack capacity limit.");
            return &stack->values[0];
        }

        stack->indexMask <<= 1;
        stack->indexMask |= 1;
        stack->values = nkiReallocArray(
            vm,
            stack->values,
            stack->capacity, sizeof(struct NKValue));

        memset(
            &stack->values[stack->size], 0,
            sizeof(struct NKValue) * (stack->capacity - stack->size));
    }

    {
        struct NKValue *ret = &stack->values[stack->size];
        assert(ret < stack->values + stack->capacity);
        stack->size++;
        return ret;
    }
}

nkbool nkiVmStackPushInt(struct NKVM *vm, nkint32_t value)
{
    struct NKValue *data = nkiVmStackPush_internal(vm);
    if(data) {
        data->type = NK_VALUETYPE_INT;
        data->intData = value;
        return nktrue;
    }
    return nkfalse;
}

nkbool nkiVmStackPushFloat(struct NKVM *vm, float value)
{
    struct NKValue *data = nkiVmStackPush_internal(vm);
    if(data) {
        data->type = NK_VALUETYPE_FLOAT;
        data->floatData = value;
        return nktrue;
    }
    return nkfalse;
}

nkbool nkiVmStackPushString(struct NKVM *vm, const char *str)
{
    struct NKValue *data = nkiVmStackPush_internal(vm);
    if(data) {
        data->type = NK_VALUETYPE_STRING;
        data->stringTableEntry =
            nkiVmStringTableFindOrAddString(
                vm, str);
        return nktrue;
    }
    return nkfalse;
}

struct NKValue *nkiVmStackPop(struct NKVM *vm)
{
    struct NKVMStack *stack = &vm->stack;

    // TODO: Shrink the stack if we can?

    if(stack->size == 0) {
        // Stack underflow. We'll return the bottom of the stack, just
        // so that whatever is expecting a valid piece of data here
        // won't explode, but the error will be visible next check.
        nkiAddError(vm, -1, "Stack underflow in pop.");
        return &stack->values[0];
    }

    stack->size--;
    return &stack->values[stack->size];
}

void nkiVmStackPopN(struct NKVM *vm, nkuint32_t count)
{
    struct NKVMStack *stack = &vm->stack;

    // TODO: Shrink the stack if we can?

    nkiDbgWriteLine("pop count: %u", count);

    if(stack->size < count) {
        // Stack underflow. We'll return the bottom of the stack, just
        // so that whatever is expecting a valid piece of data here
        // won't explode, but the error will be visible next check.
        nkiAddError(vm, -1, "Stack underflow in popN.");
        stack->size = 0;
        return;
    }

    stack->size -= count;
}

struct NKValue *nkiVmStackPeek(struct NKVM *vm, nkuint32_t index)
{
    return &vm->stack.values[index & vm->stack.indexMask];
}

void nkiVmStackClear(struct NKVM *vm)
{
    struct NKVMStack *stack = &vm->stack;
    nkiFree(vm, stack->values);
    stack->values = nkiMalloc(vm, sizeof(struct NKValue));
    memset(stack->values, 0, sizeof(struct NKValue));
    stack->size = 0;
    stack->capacity = 1;
    stack->indexMask = 0;
}

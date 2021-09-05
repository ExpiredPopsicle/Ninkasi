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

#ifndef NINKASI_VMSTACK_H
#define NINKASI_VMSTACK_H

#include "nktypes.h"

struct NKValue;
struct NKVM;

struct NKVMStack
{
    struct NKValue *values;
    nkuint32_t size;
    nkuint32_t capacity;
    nkuint32_t indexMask;
};

/// Push an integer onto the stack.
nkbool nkiVmStackPushInt(struct NKVM *vm, nkint32_t value);

/// Push a float onto the stack.
nkbool nkiVmStackPushFloat(struct NKVM *vm, float value);

/// Push a string onto the stack.
nkbool nkiVmStackPushString(struct NKVM *vm, const char *str);

/// Pop something off the stack and return it. NOTE: Returns a pointer
/// to the object on the stack, which will be overwritten the next
/// time you push something!
struct NKValue *nkiVmStackPop(struct NKVM *vm);
void nkiVmStackPopN(struct NKVM *vm, nkuint32_t count);

/// Fetch a value from an arbitrary stack position, masked to the
/// stack's current range.
struct NKValue *nkiVmStackPeek(struct NKVM *vm, nkuint32_t index);

/// Init the stack on the VM object.
void nkiVmStackInit(struct NKVM *vm, struct NKVMStack *stack);

/// Tear down the stack on the VM object.
void nkiVmStackDestroy(struct NKVM *vm, struct NKVMStack *stack);

/// Pushes a new value and returns a pointer to it (so the caller may
/// fill it in).
struct NKValue *nkiVmStackPush_internal(struct NKVM *vm);

/// Clear the entire stack. Memory will only be reduced if freeMem is
/// set to nktrue, but this will cause an extra reallocation and
/// should NOT be used in places where an allocation error could cause
/// issues (such as VM cleanup).
void nkiVmStackClear(
    struct NKVM *vm, struct NKVMStack *stack,
    nkbool freeMem);

#endif // NINKASI_VMSTACK_H


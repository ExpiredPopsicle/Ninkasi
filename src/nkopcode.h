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

#ifndef NINKASI_OPCODE_H
#define NINKASI_OPCODE_H

#include "nkconfig.h"
#include "nkvalue.h"
#include "nkfunc.h"

// ----------------------------------------------------------------------
// Types.

struct NKVMStack;
struct NKVM;

struct NKInstruction
{
    // Must be 32-bits!
    union {
        enum NKOpcode opcode;

        nkint32_t opData_int;
        float opData_float;
        nkuint32_t opData_string;
        NKVMInternalFunctionID opData_functionId;
    };

  #if NK_VM_DEBUG
    // Must be LAST for serialization purposes.
    nkuint32_t lineNumber;
  #endif
};

// ----------------------------------------------------------------------
// Individual opcode functions.

void nkiOpcode_add(struct NKVM *vm);
void nkiOpcode_subtract(struct NKVM *vm);
void nkiOpcode_multiply(struct NKVM *vm);
void nkiOpcode_divide(struct NKVM *vm);
void nkiOpcode_modulo(struct NKVM *vm);
void nkiOpcode_negate(struct NKVM *vm);

void nkiOpcode_pushLiteral_int(struct NKVM *vm);
void nkiOpcode_pushLiteral_float(struct NKVM *vm);
void nkiOpcode_pushLiteral_string(struct NKVM *vm);
void nkiOpcode_pushLiteral_functionId(struct NKVM *vm);

void nkiOpcode_pop(struct NKVM *vm);
void nkiOpcode_popN(struct NKVM *vm);

void nkiOpcode_nop(struct NKVM *vm);

/// Pops a stack index off the top of the stack, and then pushes a
/// copy of the value at that index to the top of the stack.
void nkiOpcode_stackPeek(struct NKVM *vm);

/// Pops a stack index off the top of the stack, and then copies the
/// new top of the stack into that index. Does NOT pop the second
/// value off!
void nkiOpcode_stackPoke(struct NKVM *vm);

/// Reads a value from the stack, looks it up in the static/global
/// variable space, and pushes the value from that slot onto the
/// stack. (index = *stackTop; *stackTop = statics[index]).
void nkiOpcode_staticPeek(struct NKVM *vm);

/// Pops a static index off the top of the static, and then copies the
/// new top of the stack into that index. Does NOT pop the second
/// value off! (index = *stackTop; stackTop--; statics[index] =
/// *stackTop).
void nkiOpcode_staticPoke(struct NKVM *vm);

/// Pops a value off the stack and adds that number to the instruction
/// pointer.
void nkiOpcode_jumpRelative(struct NKVM *vm);

void nkiOpcode_call(struct NKVM *vm);
void nkiOpcode_return(struct NKVM *vm);

void nkiOpcode_end(struct NKVM *vm);

void nkiOpcode_jz(struct NKVM *vm);

void nkiOpcode_gt(struct NKVM *vm);
void nkiOpcode_lt(struct NKVM *vm);
void nkiOpcode_ge(struct NKVM *vm);
void nkiOpcode_le(struct NKVM *vm);
void nkiOpcode_eq(struct NKVM *vm);
void nkiOpcode_ne(struct NKVM *vm);
void nkiOpcode_eqsametype(struct NKVM *vm);
void nkiOpcode_not(struct NKVM *vm);

void nkiOpcode_and(struct NKVM *vm);
void nkiOpcode_or(struct NKVM *vm);

void nkiOpcode_createObject(struct NKVM *vm);
void nkiOpcode_objectFieldGet(struct NKVM *vm);
void nkiOpcode_objectFieldSet(struct NKVM *vm);

void nkiOpcode_prepareSelfCall(struct NKVM *vm);
void nkiOpcode_objectFieldGet_noPop(struct NKVM *vm);

void nkiOpcode_pushNil(struct NKVM *vm);

#endif // NINKASI_OPCODE_H

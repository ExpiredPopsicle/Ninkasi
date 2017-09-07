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

#ifndef NINKASI_COMPILER_H
#define NINKASI_COMPILER_H

#include "nkfunc.h"

// ----------------------------------------------------------------------
// Internals

struct NKInstruction;
struct NKToken;

struct NKCompilerStateContextVariable
{
    char *name;
    nkbool isGlobal;

    // True if we should not pop this variable off the stack when it
    // goes out of scope. This is for things like function arguments,
    // which really belong to the calling function.
    nkbool doNotPopWhenOutOfScope;

    // If this is a global, then position is the position in the
    // static space. If this is not a global, then position is the
    // position from the start of the stack frame.
    nkuint32_t position;

    struct NKCompilerStateContextVariable *next;
};

struct NKCompilerStateContext
{
    struct NKCompilerStateContext *parent;
    struct NKCompilerStateContextVariable *variables;

    nkuint32_t stackFrameOffset;

    NKVMInternalFunctionID currentFunctionId;
    nkbool isLoopContext;
    nkuint32_t *loopContextFixups;
    nkuint32_t loopContextFixupCount;
};

struct NKCompilerState
{
    struct NKVM *vm;

    nkuint32_t instructionWriteIndex;

    struct NKCompilerStateContext *context;

    struct NKToken *currentToken;
    nkuint32_t currentLineNumber;

    nkuint32_t recursionCount;

    nkuint32_t staticVariableCount;
};

extern nkint32_t nkiCompilerStackOffsetTable[NK_OPCODE_PADDEDCOUNT];

// ----------------------------------------------------------------------
// Creation and cleanup.

/// Create a compiler.
struct NKCompilerState *nkiCompilerCreate(
    struct NKVM *vm);

/// Destroy a compiler. This will also finish off any remaining tasks
/// like setting up the global variable list in the VM.
void nkiCompilerFinalize(
    struct NKCompilerState *cs);

// ----------------------------------------------------------------------
// Basic compiling.

/// This can be done multiple times. It'll just be the equivalent of
/// appending each script onto the end, except for the line number
/// counts.
nkbool nkiCompilerCompileScript(
    struct NKCompilerState *cs,
    const char *script);

// ----------------------------------------------------------------------
// Basic instruction writing.

void nkiCompilerAddInstructionSimple(
    struct NKCompilerState *cs, enum NKOpcode opcode,
    nkbool adjustStackFrame);

void nkiCompilerAddInstruction(
    struct NKCompilerState *cs, struct NKInstruction *inst,
    nkbool adjustStackFrame);

// ----------------------------------------------------------------------
// Context manipulation.

void nkiCompilerPushContext(struct NKCompilerState *cs);
void nkiCompilerPopContext(struct NKCompilerState *cs);

// ----------------------------------------------------------------------
// Variables.

/// Adds a variable to the current context. Emits code on-the-spot to
/// make room for the stack if asked to, and points to the current
/// stack offset. TL;DR: You cannot use this to add variables at any
/// time.
struct NKCompilerStateContextVariable *nkiCompilerAddVariable(
    struct NKCompilerState *cs, const char *name, nkbool useValueAtStackTop);

struct NKCompilerStateContextVariable *nkiCompilerLookupVariable(
    struct NKCompilerState *cs,
    const char *name);

/// Create a C function and assign it a variable name at the current
/// scope. Use this to make a globally defined C function at
/// compile-time. Do this before script compilation, so the script
/// itself can access it.
void nkiCompilerCreateCFunctionVariable(
    struct NKCompilerState *cs,
    const char *name,
    NKVMFunctionCallback func);

// ----------------------------------------------------------------------
// Recursive-descent compiler functions.

nkbool nkiCompilerCompileStatement(struct NKCompilerState *cs);
nkbool nkiCompilerCompileBlock(struct NKCompilerState *cs, nkbool noBracesOrContext);
nkbool nkiCompilerCompileVariableDeclaration(struct NKCompilerState *cs);
nkbool nkiCompilerCompileFunctionDefinition(struct NKCompilerState *cs);
nkbool nkiCompilerCompileReturnStatement(struct NKCompilerState *cs);
nkbool nkiCompilerCompileIfStatement(struct NKCompilerState *cs);
nkbool nkiCompilerCompileWhileStatement(struct NKCompilerState *cs);
nkbool nkiCompilerCompileForStatement(struct NKCompilerState *cs);
nkbool nkiCompilerCompileBreakStatement(struct NKCompilerState *cs);

// ----------------------------------------------------------------------
// Bytecode output functions. Sometimes we need something a little
// more complicated than a single nkiCompilerAddInstruction, in a
// place that we'll use it many times.

void nkiCompilerEmitPushLiteralInt(struct NKCompilerState *cs, nkint32_t value, nkbool adjustStackFrame);
void nkiCompilerEmitPushLiteralFloat(struct NKCompilerState *cs, float value, nkbool adjustStackFrame);
void nkiCompilerEmitPushLiteralString(struct NKCompilerState *cs, const char *str, nkbool adjustStackFrame);
void nkiCompilerEmitPushLiteralFunctionId(
    struct NKCompilerState *cs, NKVMInternalFunctionID functionId, nkbool adjustStackFrame);
void nkiCompilerEmitPushNil(struct NKCompilerState *cs, nkbool adjustStackFrame);
void nkiCompilerEmitReturn(struct NKCompilerState *cs);

/// Emit a jump instruction. Return value is the index of the jump
/// address in the instructions list, so we can go back and patch
/// stuff up later. This way we can write a jump statement for a
/// conditional block or loop and set the jump address when we're done
/// parsing the loop.
nkuint32_t nkiCompilerEmitJump(struct NKCompilerState *cs, nkuint32_t target);

/// Emit conditional jump.
nkuint32_t nkiCompilerEmitJumpIfZero(struct NKCompilerState *cs, nkuint32_t target);

/// Go back and fix up a jump address with a new target.
void nkiCompilerModifyJump(
    struct NKCompilerState *cs,
    nkuint32_t pushLiteralBeforeJumpAddress,
    nkuint32_t target);

// ----------------------------------------------------------------------
// Token traversal state stuff.

struct NKToken *nkiCompilerNextToken(struct NKCompilerState *cs);
enum NKTokenType nkiCompilerCurrentTokenType(struct NKCompilerState *cs);
nkuint32_t nkiCompilerCurrentTokenLinenumber(struct NKCompilerState *cs);
const char *nkiCompilerCurrentTokenString(struct NKCompilerState *cs);
nkbool nkiCompilerExpectAndSkipToken(
    struct NKCompilerState *cs, enum NKTokenType t);

// Compiler-specific version of nkiAddError. Adds in line number.
void nkiCompilerAddError(struct NKCompilerState *cs, const char *error);

// Recursion counter to prevent the stack from getting too big. (DOS
// compatibility.)
nkbool nkiCompilerPushRecursion(struct NKCompilerState *cs);
void nkiCompilerPopRecursion(struct NKCompilerState *cs);

#endif // NINKASI_COMPILER_H

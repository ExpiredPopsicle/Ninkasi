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
    bool isGlobal;

    // True if we should not pop this variable off the stack when it
    // goes out of scope. This is for things like function arguments,
    // which really belong to the calling function.
    bool doNotPopWhenOutOfScope;

    // If this is a global, then stackPos is the position in the stack
    // from the very beginning of the stack, meaning we should refer
    // to it with its absolute position. If this is not a global, then
    // stackPos is the position from the start of the stack frame.
    uint32_t stackPos;

    struct NKCompilerStateContextVariable *next;
};

struct NKCompilerStateContext
{
    struct NKCompilerStateContext *parent;
    struct NKCompilerStateContextVariable *variables;

    uint32_t stackFrameOffset;

    uint32_t currentFunctionId;
    bool isLoopContext;
    uint32_t *loopContextFixups;
    uint32_t loopContextFixupCount;
};

struct NKCompilerState
{
    struct NKVM *vm;

    uint32_t instructionWriteIndex;

    struct NKCompilerStateContext *context;

    struct NKToken *currentToken;
    uint32_t currentLineNumber;

    uint32_t recursionCount;
};

extern int32_t nkiCompilerStackOffsetTable[NK_OPCODE_PADDEDCOUNT];

void nkiCompilerAddInstructionSimple(
    struct NKCompilerState *cs, enum NKOpcode opcode,
    bool adjustStackFrame);
void nkiCompilerAddInstruction(
    struct NKCompilerState *cs, struct NKInstruction *inst,
    bool adjustStackFrame);

void nkiCompilerPushContext(struct NKCompilerState *cs);
void nkiCompilerPopContext(struct NKCompilerState *cs);

/// Adds a variable to the current context. Emits code on-the-spot to
/// make room for the stack if asked to, and points to the current
/// stack offset. TL;DR: You cannot use this to add variables at any
/// time.
struct NKCompilerStateContextVariable *nkiCompilerAddVariable(
    struct NKCompilerState *cs, const char *name, bool allocateStackSpace);

struct NKCompilerStateContextVariable *lookupVariable(
    struct NKCompilerState *cs,
    const char *name);

bool compileStatement(struct NKCompilerState *cs);
bool compileBlock(struct NKCompilerState *cs, bool noBracesOrContext);
bool compileVariableDeclaration(struct NKCompilerState *cs);
bool compileFunctionDefinition(struct NKCompilerState *cs);
bool compileReturnStatement(struct NKCompilerState *cs);
bool compileIfStatement(struct NKCompilerState *cs);
bool compileWhileStatement(struct NKCompilerState *cs);
bool compileForStatement(struct NKCompilerState *cs);
bool compileBreakStatement(struct NKCompilerState *cs);

void nkiCompilerEmitPushLiteralInt(struct NKCompilerState *cs, int32_t value, bool adjustStackFrame);
void nkiCompilerEmitPushLiteralFloat(struct NKCompilerState *cs, float value, bool adjustStackFrame);
void nkiCompilerEmitPushLiteralString(struct NKCompilerState *cs, const char *str, bool adjustStackFrame);
void nkiCompilerEmitPushLiteralFunctionId(struct NKCompilerState *cs, uint32_t functionId, bool adjustStackFrame);
void nkiCompilerEmitPushNil(struct NKCompilerState *cs, bool adjustStackFrame);

// Token traversal state stuff.
struct NKToken *nkiCompilerNextToken(struct NKCompilerState *cs);
enum NKTokenType nkiCompilerCurrentTokenType(struct NKCompilerState *cs);
uint32_t nkiCompilerCurrentTokenLinenumber(struct NKCompilerState *cs);
const char *nkiCompilerCurrentTokenString(struct NKCompilerState *cs);
bool nkiCompilerExpectAndSkipToken(
    struct NKCompilerState *cs, enum NKTokenType t);

// Compiler-specific version of nkiAddError. Adds in line number.
void nkiCompilerAddError(struct NKCompilerState *cs, const char *error);

bool nkiCompilerPushRecursion(struct NKCompilerState *cs);
void nkiCompilerPopRecursion(struct NKCompilerState *cs);

#endif // NINKASI_COMPILER_H

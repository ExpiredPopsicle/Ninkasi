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
bool nkiCompilerCompileScript(
    struct NKCompilerState *cs,
    const char *script);

// ----------------------------------------------------------------------
// Basic instruction writing.

void nkiCompilerAddInstructionSimple(
    struct NKCompilerState *cs, enum NKOpcode opcode,
    bool adjustStackFrame);

void nkiCompilerAddInstruction(
    struct NKCompilerState *cs, struct NKInstruction *inst,
    bool adjustStackFrame);

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
    struct NKCompilerState *cs, const char *name, bool allocateStackSpace);

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
    VMFunctionCallback func,
    void *userData);

// ----------------------------------------------------------------------
// Recursive-descent compiler functions.

bool nkiCompilerCompileStatement(struct NKCompilerState *cs);
bool nkiCompilerCompileBlock(struct NKCompilerState *cs, bool noBracesOrContext);
bool nkiCompilerCompileVariableDeclaration(struct NKCompilerState *cs);
bool nkiCompilerCompileFunctionDefinition(struct NKCompilerState *cs);
bool nkiCompilerCompileReturnStatement(struct NKCompilerState *cs);
bool nkiCompilerCompileIfStatement(struct NKCompilerState *cs);
bool nkiCompilerCompileWhileStatement(struct NKCompilerState *cs);
bool nkiCompilerCompileForStatement(struct NKCompilerState *cs);
bool nkiCompilerCompileBreakStatement(struct NKCompilerState *cs);

// Bytecode output functions. Sometimes we need something a little
// more complicated than a single nkiCompilerAddInstruction, in a
// place that we'll use it many times.
void nkiCompilerEmitPushLiteralInt(struct NKCompilerState *cs, int32_t value, bool adjustStackFrame);
void nkiCompilerEmitPushLiteralFloat(struct NKCompilerState *cs, float value, bool adjustStackFrame);
void nkiCompilerEmitPushLiteralString(struct NKCompilerState *cs, const char *str, bool adjustStackFrame);
void nkiCompilerEmitPushLiteralFunctionId(struct NKCompilerState *cs, uint32_t functionId, bool adjustStackFrame);
void nkiCompilerEmitPushNil(struct NKCompilerState *cs, bool adjustStackFrame);
void nkiCompilerEmitReturn(struct NKCompilerState *cs);

// Token traversal state stuff.
struct NKToken *nkiCompilerNextToken(struct NKCompilerState *cs);
enum NKTokenType nkiCompilerCurrentTokenType(struct NKCompilerState *cs);
uint32_t nkiCompilerCurrentTokenLinenumber(struct NKCompilerState *cs);
const char *nkiCompilerCurrentTokenString(struct NKCompilerState *cs);
bool nkiCompilerExpectAndSkipToken(
    struct NKCompilerState *cs, enum NKTokenType t);

// Compiler-specific version of nkiAddError. Adds in line number.
void nkiCompilerAddError(struct NKCompilerState *cs, const char *error);

// Recursion counter to prevent the stack from getting too big. (DOS
// compatibility.)
bool nkiCompilerPushRecursion(struct NKCompilerState *cs);
void nkiCompilerPopRecursion(struct NKCompilerState *cs);

#endif // NINKASI_COMPILER_H

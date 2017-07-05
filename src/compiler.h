#ifndef COMPILER_H
#define COMPILER_H

#include "function.h"

// ----------------------------------------------------------------------
// Public interface

struct CompilerState;
struct VM;

/// Create a compiler.
struct CompilerState *vmCompilerCreate(
    struct VM *vm);

/// Create a C function and assign it a variable name at the current
/// scope. Use this to make a globally defined C function at
/// compile-time. Do this before script compilation, so the script
/// itself can access it.
void vmCompilerCreateCFunctionVariable(
    struct CompilerState *cs,
    const char *name,
    VMFunctionCallback func);

/// This can be done multiple times. It'll just be the equivalent of
/// appending each script onto the end, except for the line number
/// counts.
bool vmCompilerCompileScript(
    struct CompilerState *cs,
    const char *script);

bool vmCompilerCompileScriptFile(
    struct CompilerState *cs,
    const char *scriptFilename);

/// Destroy a compiler. This will also finish off any remaining tasks
/// like setting up the global variable list in the VM.
void vmCompilerFinalize(
    struct CompilerState *cs);

// ----------------------------------------------------------------------
// Internals

struct Instruction;
struct Token;

struct CompilerStateContextVariable
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

    struct CompilerStateContextVariable *next;
};

struct CompilerStateContext
{
    struct CompilerStateContext *parent;
    struct CompilerStateContextVariable *variables;

    uint32_t stackFrameOffset;

    uint32_t currentFunctionId;
};

struct CompilerState
{
    struct VM *vm;

    uint32_t instructionWriteIndex;

    struct CompilerStateContext *context;

    struct Token *currentToken;
    uint32_t currentLineNumber;
};

void addInstruction(struct CompilerState *cs, struct Instruction *inst);
void addInstructionSimple(struct CompilerState *cs, enum Opcode opcode);

void pushContext(struct CompilerState *cs);
void popContext(struct CompilerState *cs);

void addVariable(struct CompilerState *cs, const char *name);
struct CompilerStateContextVariable *addVariableWithoutStackAllocation(
    struct CompilerState *cs, const char *name);

struct CompilerStateContextVariable *lookupVariable(
    struct CompilerState *cs,
    const char *name);

bool compileStatement(struct CompilerState *cs);
bool compileBlock(struct CompilerState *cs, bool noBracesOrContext);
bool compileVariableDeclaration(struct CompilerState *cs);
bool compileFunctionDefinition(struct CompilerState *cs);
bool compileReturnStatement(struct CompilerState *cs);
bool compileIfStatement(struct CompilerState *cs);
bool compileWhileStatement(struct CompilerState *cs);
bool compileForStatement(struct CompilerState *cs);

void emitPushLiteralInt(struct CompilerState *cs, int32_t value);
void emitPushLiteralFloat(struct CompilerState *cs, float value);
void emitPushLiteralString(struct CompilerState *cs, const char *str);
void emitPushLiteralFunctionId(struct CompilerState *cs, uint32_t functionId);

struct Token *vmCompilerNextToken(struct CompilerState *cs);
enum TokenType vmCompilerTokenType(struct CompilerState *cs);
uint32_t vmCompilerGetLinenumber(struct CompilerState *cs);
const char *vmCompilerTokenString(struct CompilerState *cs);
void vmCompilerAddError(struct CompilerState *cs, const char *error);

#endif

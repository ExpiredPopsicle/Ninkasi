#ifndef COMPILER_H
#define COMPILER_H

struct VM;
struct Instruction;

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
    const char *name,
    uint32_t lineNumber);

bool compileStatement(struct CompilerState *cs, struct Token **currentToken);
bool compileBlock(struct CompilerState *cs, struct Token **currentToken, bool noBracesOrContext);
bool compileVariableDeclaration(struct CompilerState *cs, struct Token **currentToken);
bool compileFunctionDefinition(struct CompilerState *cs, struct Token **currentToken);
bool compileReturnStatement(struct CompilerState *cs, struct Token **currentToken);

void emitPushLiteralInt(struct CompilerState *cs, int32_t value);
void emitPushLiteralFloat(struct CompilerState *cs, float value);
void emitPushLiteralString(struct CompilerState *cs, const char *str);

#endif

#ifndef COMPILER_H
#define COMPILER_H

struct VM;
struct Instruction;

struct CompilerStateContextVariable
{
    char *name;
    bool isGlobal;

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

struct CompilerStateContextVariable *lookupVariable(
    struct CompilerState *cs,
    const char *name,
    uint32_t lineNumber);

bool compileStatement(struct CompilerState *cs, struct Token **currentToken);

bool compileBlock(struct CompilerState *cs, struct Token **currentToken);


#endif

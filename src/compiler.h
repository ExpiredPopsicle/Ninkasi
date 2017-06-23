#ifndef COMPILER_H
#define COMPILER_H

struct VM;
struct Instruction;

struct CompilerState
{
    struct VM *vm;

    uint32_t instructionWriteIndex;
};

void addInstruction(struct CompilerState *cs, struct Instruction *inst);
void addInstructionSimple(struct CompilerState *cs, enum Opcode opcode);

#endif

#ifndef OPCODE_H
#define OPCODE_H

#include "value.h"

struct VMStack;
struct VM;

struct Instruction
{
    enum Opcode opcode;

    union
    {
        struct
        {
            struct Value value;
        } pushLiteralData;
    };
};


void opcode_add(struct VM *vm, struct Instruction *instruction);
void opcode_subtract(struct VM *vm, struct Instruction *instruction);
void opcode_multiply(struct VM *vm, struct Instruction *instruction);
void opcode_divide(struct VM *vm, struct Instruction *instruction);
void opcode_negate(struct VM *vm, struct Instruction *instruction);

void opcode_pushLiteral(struct VM *vm, struct Instruction *instruction);
void opcode_nop(struct VM *vm, struct Instruction *instruction);

#endif

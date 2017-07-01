#ifndef OPCODE_H
#define OPCODE_H

#include "value.h"

struct VMStack;
struct VM;

struct Instruction
{
    union {
        enum Opcode opcode;

        int32_t opData_int;
        float opData_float;
        uint32_t opData_string;
    };
};


void opcode_add(struct VM *vm, struct Instruction *instruction);
void opcode_subtract(struct VM *vm, struct Instruction *instruction);
void opcode_multiply(struct VM *vm, struct Instruction *instruction);
void opcode_divide(struct VM *vm, struct Instruction *instruction);
void opcode_negate(struct VM *vm, struct Instruction *instruction);

void opcode_pushLiteral_int(struct VM *vm, struct Instruction *instruction);
void opcode_pushLiteral_float(struct VM *vm, struct Instruction *instruction);
void opcode_pushLiteral_string(struct VM *vm, struct Instruction *instruction);

void opcode_pop(struct VM *vm, struct Instruction *instruction);

void opcode_nop(struct VM *vm, struct Instruction *instruction);

void opcode_dump(struct VM *vm, struct Instruction *instruction);

void opcode_stackPeek(struct VM *vm, struct Instruction *instruction);
void opcode_stackPoke(struct VM *vm, struct Instruction *instruction);

void opcode_jumpRelative(struct VM *vm, struct Instruction *instruction);

#endif

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
        uint32_t opData_functionId;
    };

  #if VM_DEBUG
    uint32_t lineNumber;
  #endif
};


void opcode_add(struct VM *vm, struct Instruction *instruction);
void opcode_subtract(struct VM *vm, struct Instruction *instruction);
void opcode_multiply(struct VM *vm, struct Instruction *instruction);
void opcode_divide(struct VM *vm, struct Instruction *instruction);
void opcode_negate(struct VM *vm, struct Instruction *instruction);

void opcode_pushLiteral_int(struct VM *vm, struct Instruction *instruction);
void opcode_pushLiteral_float(struct VM *vm, struct Instruction *instruction);
void opcode_pushLiteral_string(struct VM *vm, struct Instruction *instruction);
void opcode_pushLiteral_functionId(struct VM *vm, struct Instruction *instruction);

void opcode_pop(struct VM *vm, struct Instruction *instruction);

void opcode_nop(struct VM *vm, struct Instruction *instruction);

void opcode_dump(struct VM *vm, struct Instruction *instruction);

/// Pops a stack index off the top of the stack, and then pushes a
/// copy of the value at that index to the top of the stack.
void opcode_stackPeek(struct VM *vm, struct Instruction *instruction);

/// Pops a stack index off the top of the stack, and then copies the
/// new top of the stack into that index. Does NOT pop the second
/// value off!
void opcode_stackPoke(struct VM *vm, struct Instruction *instruction);

/// Pops a value off the stack and adds that number to the instruction
/// pointer.
void opcode_jumpRelative(struct VM *vm, struct Instruction *instruction);

void opcode_call(struct VM *vm, struct Instruction *instruction);
void opcode_return(struct VM *vm, struct Instruction *instruction);

#endif

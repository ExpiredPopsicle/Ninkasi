#ifndef OPCODE_H
#define OPCODE_H

#include "config.h"
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


void opcode_add(struct VM *vm);
void opcode_subtract(struct VM *vm);
void opcode_multiply(struct VM *vm);
void opcode_divide(struct VM *vm);
void opcode_modulo(struct VM *vm);
void opcode_negate(struct VM *vm);

void opcode_pushLiteral_int(struct VM *vm);
void opcode_pushLiteral_float(struct VM *vm);
void opcode_pushLiteral_string(struct VM *vm);
void opcode_pushLiteral_functionId(struct VM *vm);

void opcode_pop(struct VM *vm);
void opcode_popN(struct VM *vm);

void opcode_nop(struct VM *vm);

void opcode_dump(struct VM *vm);

/// Pops a stack index off the top of the stack, and then pushes a
/// copy of the value at that index to the top of the stack.
void opcode_stackPeek(struct VM *vm);

/// Pops a stack index off the top of the stack, and then copies the
/// new top of the stack into that index. Does NOT pop the second
/// value off!
void opcode_stackPoke(struct VM *vm);

/// Pops a value off the stack and adds that number to the instruction
/// pointer.
void opcode_jumpRelative(struct VM *vm);

void opcode_call(struct VM *vm);
void opcode_return(struct VM *vm);

void opcode_end(struct VM *vm);

void opcode_jz(struct VM *vm);

void opcode_gt(struct VM *vm);
void opcode_lt(struct VM *vm);
void opcode_ge(struct VM *vm);
void opcode_le(struct VM *vm);
void opcode_eq(struct VM *vm);
void opcode_ne(struct VM *vm);
void opcode_eqsametype(struct VM *vm);
void opcode_not(struct VM *vm);

void opcode_and(struct VM *vm);
void opcode_or(struct VM *vm);

void opcode_createObject(struct VM *vm);
void opcode_objectFieldGet(struct VM *vm);
void opcode_objectFieldSet(struct VM *vm);

void opcode_prepareSelfCall(struct VM *vm);
void opcode_objectFieldGet_noPop(struct VM *vm);

#endif

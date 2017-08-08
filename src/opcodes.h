#ifndef NINKASI_OPCODE_H
#define NINKASI_OPCODE_H

#include "nkconfig.h"
#include "value.h"

struct NKVMStack;
struct NKVM;

struct NKInstruction
{
    union {
        enum NKOpcode opcode;

        int32_t opData_int;
        float opData_float;
        uint32_t opData_string;
        uint32_t opData_functionId;
    };

  #if NK_VM_DEBUG
    uint32_t lineNumber;
  #endif
};


void opcode_add(struct NKVM *vm);
void opcode_subtract(struct NKVM *vm);
void opcode_multiply(struct NKVM *vm);
void opcode_divide(struct NKVM *vm);
void opcode_modulo(struct NKVM *vm);
void opcode_negate(struct NKVM *vm);

void opcode_pushLiteral_int(struct NKVM *vm);
void opcode_pushLiteral_float(struct NKVM *vm);
void opcode_pushLiteral_string(struct NKVM *vm);
void opcode_pushLiteral_functionId(struct NKVM *vm);

void opcode_pop(struct NKVM *vm);
void opcode_popN(struct NKVM *vm);

void opcode_nop(struct NKVM *vm);

void opcode_dump(struct NKVM *vm);

/// Pops a stack index off the top of the stack, and then pushes a
/// copy of the value at that index to the top of the stack.
void opcode_stackPeek(struct NKVM *vm);

/// Pops a stack index off the top of the stack, and then copies the
/// new top of the stack into that index. Does NOT pop the second
/// value off!
void opcode_stackPoke(struct NKVM *vm);

/// Pops a value off the stack and adds that number to the instruction
/// pointer.
void opcode_jumpRelative(struct NKVM *vm);

void opcode_call(struct NKVM *vm);
void opcode_return(struct NKVM *vm);

void opcode_end(struct NKVM *vm);

void opcode_jz(struct NKVM *vm);

void opcode_gt(struct NKVM *vm);
void opcode_lt(struct NKVM *vm);
void opcode_ge(struct NKVM *vm);
void opcode_le(struct NKVM *vm);
void opcode_eq(struct NKVM *vm);
void opcode_ne(struct NKVM *vm);
void opcode_eqsametype(struct NKVM *vm);
void opcode_not(struct NKVM *vm);

void opcode_and(struct NKVM *vm);
void opcode_or(struct NKVM *vm);

void opcode_createObject(struct NKVM *vm);
void opcode_objectFieldGet(struct NKVM *vm);
void opcode_objectFieldSet(struct NKVM *vm);

void opcode_prepareSelfCall(struct NKVM *vm);
void opcode_objectFieldGet_noPop(struct NKVM *vm);

void opcode_pushNil(struct NKVM *vm);

#endif // NINKASI_OPCODE_H

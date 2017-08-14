#ifndef NINKASI_OPCODE_H
#define NINKASI_OPCODE_H

#include "nkconfig.h"
#include "nkvalue.h"

// ----------------------------------------------------------------------
// Types.

struct NKVMStack;
struct NKVM;

struct NKInstruction
{
    union {
        enum NKOpcode opcode;

        nkint32_t opData_int;
        float opData_float;
        nkuint32_t opData_string;
        nkuint32_t opData_functionId;
    };

  #if NK_VM_DEBUG
    nkuint32_t lineNumber;
  #endif
};

// ----------------------------------------------------------------------
// Individual opcode functions.

void nkiOpcode_add(struct NKVM *vm);
void nkiOpcode_subtract(struct NKVM *vm);
void nkiOpcode_multiply(struct NKVM *vm);
void nkiOpcode_divide(struct NKVM *vm);
void nkiOpcode_modulo(struct NKVM *vm);
void nkiOpcode_negate(struct NKVM *vm);

void nkiOpcode_pushLiteral_int(struct NKVM *vm);
void nkiOpcode_pushLiteral_float(struct NKVM *vm);
void nkiOpcode_pushLiteral_string(struct NKVM *vm);
void nkiOpcode_pushLiteral_functionId(struct NKVM *vm);

void nkiOpcode_pop(struct NKVM *vm);
void nkiOpcode_popN(struct NKVM *vm);

void nkiOpcode_nop(struct NKVM *vm);

void nkiOpcode_dump(struct NKVM *vm);

/// Pops a stack index off the top of the stack, and then pushes a
/// copy of the value at that index to the top of the stack.
void nkiOpcode_stackPeek(struct NKVM *vm);

/// Pops a stack index off the top of the stack, and then copies the
/// new top of the stack into that index. Does NOT pop the second
/// value off!
void nkiOpcode_stackPoke(struct NKVM *vm);

/// Pops a value off the stack and adds that number to the instruction
/// pointer.
void nkiOpcode_jumpRelative(struct NKVM *vm);

void nkiOpcode_call(struct NKVM *vm);
void nkiOpcode_return(struct NKVM *vm);

void nkiOpcode_end(struct NKVM *vm);

void nkiOpcode_jz(struct NKVM *vm);

void nkiOpcode_gt(struct NKVM *vm);
void nkiOpcode_lt(struct NKVM *vm);
void nkiOpcode_ge(struct NKVM *vm);
void nkiOpcode_le(struct NKVM *vm);
void nkiOpcode_eq(struct NKVM *vm);
void nkiOpcode_ne(struct NKVM *vm);
void nkiOpcode_eqsametype(struct NKVM *vm);
void nkiOpcode_not(struct NKVM *vm);

void nkiOpcode_and(struct NKVM *vm);
void nkiOpcode_or(struct NKVM *vm);

void nkiOpcode_createObject(struct NKVM *vm);
void nkiOpcode_objectFieldGet(struct NKVM *vm);
void nkiOpcode_objectFieldSet(struct NKVM *vm);

void nkiOpcode_prepareSelfCall(struct NKVM *vm);
void nkiOpcode_objectFieldGet_noPop(struct NKVM *vm);

void nkiOpcode_pushNil(struct NKVM *vm);

#endif // NINKASI_OPCODE_H

#ifndef OPCODE_H
#define OPCODE_H

struct VMStack;

enum Opcodes
{
    OP_ADD,
};

bool opcode_add(struct VMStack *stack);

#endif

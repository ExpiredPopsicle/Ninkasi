#ifndef OPCODE_H
#define OPCODE_H

#include "value.h"

struct VMStack;

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


bool opcode_add(struct VMStack *stack);

#endif

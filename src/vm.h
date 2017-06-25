#ifndef VM_H
#define VM_H

#include "error.h"
#include "vmstack.h"
#include "opcodes.h"
#include "vmstring.h"

struct VM
{
    struct ErrorState errorState;
    struct VMStack stack;

    struct Instruction *instructions;
    uint32_t instructionAddressMask;
    uint32_t instructionPointer;

    struct VMStringTable stringTable;

    uint32_t lastGCPass;
};

void vmInit(struct VM *vm);
void vmDestroy(struct VM *vm);

void vmIterate(struct VM *vm);

void vmGarbageCollect(struct VM *vm);

void vmRescanProgramStrings(struct VM *vm);

#endif

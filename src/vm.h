#ifndef VM_H
#define VM_H

#include "error.h"
#include "vmstack.h"
#include "opcodes.h"
#include "vmstring.h"

struct VMFunction;

struct VM
{
    struct ErrorState errorState;
    struct VMStack stack;

    struct Instruction *instructions;
    uint32_t instructionAddressMask;
    uint32_t instructionPointer;

    struct VMStringTable stringTable;

    uint32_t lastGCPass;

    uint32_t functionCount;
    struct VMFunction *functionTable;
};

void vmInit(struct VM *vm);
void vmDestroy(struct VM *vm);

/// Run a single instruction inside the VM and advance the program
/// counter.
void vmIterate(struct VM *vm);

/// Force a garbage collection pass.
void vmGarbageCollect(struct VM *vm);

/// Re-check all strings in the string table to see if they're in-use
/// by any program code. If program code has been removed that
/// references strings, then they can be made garbage-collectible.
void vmRescanProgramStrings(struct VM *vm);

const char *vmGetOpcodeName(enum Opcode op);

struct VMFunction *vmCreateFunction(struct VM *vm, uint32_t *functionId);

#endif

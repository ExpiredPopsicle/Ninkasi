#ifndef VM_H
#define VM_H

#include "error.h"
#include "vmstack.h"
#include "opcodes.h"
#include "vmstring.h"
#include "function.h"

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
    uint32_t gcInterval;
    uint32_t gcCountdown;

    uint32_t functionCount;
    struct VMFunction *functionTable;

    // NOTE: When you add the object table, you will need to add
    // external reference counts. We don't care about this for strings
    // (external code can just strdup it) or functions (they are
    // permanently part of the program and GC doesn't even exist for
    // them), but we do care about this for objects. We'll also have
    // to keep a list of all objects that are externally referenced
    // anywhere, so that the GC can start on those and mark others.
    //
    // Maybe just an external reference list? (Can store multiple
    // entries for multiple external references to the same object.)
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



void vmCreateCFunction(struct CompilerState *cs, const char *name, VMFunctionCallback func);


void vmCallFunction(
    struct VM *vm,
    struct Value *functionValue,
    uint32_t argumentCount,
    struct Value *arguments,
    struct Value *returnValue);

// void vmCallFunctionByName(
//     struct CompilerState *cs,
//     const char *name,
//     uint32_t argumentCount,
//     struct Value *arguments,
//     struct Value *returnValue);

// ----------------------------------------------------------------------

/// Compiler internal function creation. Don't use this outside. Not
/// for that.
struct VMFunction *vmCreateFunction(struct VM *vm, uint32_t *functionId);


#endif

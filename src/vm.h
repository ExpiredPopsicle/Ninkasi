#ifndef VM_H
#define VM_H

#include <setjmp.h>

#include "error.h"
#include "vmstack.h"
#include "opcodes.h"
#include "vmstring.h"
#include "function.h"
#include "objects.h"

struct VM;

// ----------------------------------------------------------------------
// Internals

struct VMFunction;
struct NKMemoryHeader;

struct VMLimits
{
    // Many of these values are only checked when a section needs to
    // expand (double) its memory usage. So only powers of two are
    // meaningful numbers for string count, stack size, and object
    // count.
    uint32_t maxStrings;
    uint32_t maxStringLength;
    uint32_t maxStacksize;
    uint32_t maxObjects;
    uint32_t maxFieldsPerObject;
    uint32_t maxAllocatedMemory;
};

struct VM
{
    struct ErrorState errorState;
    struct VMStack stack;

    struct Instruction *instructions;
    uint32_t instructionAddressMask;
    uint32_t instructionPointer;

    struct VMStringTable stringTable;
    struct VMObjectTable objectTable;
    // TODO: External data table.

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

    // TODO: Add a global variable table and global variable lookup,
    // so we don't have to keep the compiler around.
    struct GlobalVariableRecord
    {
        uint32_t stackPosition;
        const char *name;
    } *globalVariables;
    char *globalVariableNameStorage;
    uint32_t globalVariableCount;

    struct VMLimits limits;
    uint32_t currentMemoryUsage;
    uint32_t peakMemoryUsage;

    struct NKMemoryHeader *allocations;

    jmp_buf catastrophicFailureJmpBuf;
};

/// Re-check all strings in the string table to see if they're in-use
/// by any program code. If program code has been removed that
/// references strings, then they can be made garbage-collectible.
void vmRescanProgramStrings(struct VM *vm);

const char *vmGetOpcodeName(enum Opcode op);

// ----------------------------------------------------------------------

/// Compiler internal function creation. Don't use this outside. Not
/// for that.
struct VMFunction *vmCreateFunction(struct VM *vm, uint32_t *functionId);


#endif

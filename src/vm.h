#ifndef NINKASI_VM_H
#define NINKASI_VM_H

#include <setjmp.h>

#include "nkerror.h"
#include "vmstack.h"
#include "nkopcode.h"
#include "nkstring.h"
#include "nkfunc.h"
#include "objects.h"

struct NKVM;

// ----------------------------------------------------------------------
// Internals

struct NKVMFunction;
struct NKMemoryHeader;

struct NKVMLimits
{
    // Many of these values are only checked when a section needs to
    // expand (double) its memory usage. So only powers of two are
    // meaningful numbers for string count, stack size, and object
    // count.
    nkuint32_t maxStrings;
    nkuint32_t maxStringLength;
    nkuint32_t maxStacksize;
    nkuint32_t maxObjects;
    nkuint32_t maxFieldsPerObject;
    nkuint32_t maxAllocatedMemory;
};

struct NKVM
{
    struct NKErrorState errorState;
    struct NKVMStack stack;

    struct NKInstruction *instructions;
    nkuint32_t instructionAddressMask;
    nkuint32_t instructionPointer;

    struct NKVMStringTable stringTable;
    struct NKVMObjectTable objectTable;

    // TODO: External data table.

    nkuint32_t lastGCPass;
    nkuint32_t gcInterval; // TODO: Move to VMLimits, maybe.
    nkuint32_t gcCountdown;
    nkuint32_t gcNewObjectInterval; // TODO: Move to VMLimits, maybe.
    nkuint32_t gcNewObjectCountdown;

    nkuint32_t functionCount;
    struct NKVMFunction *functionTable;

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
        nkuint32_t stackPosition;
        const char *name;
    } *globalVariables;
    char *globalVariableNameStorage;
    nkuint32_t globalVariableCount;

    struct NKVMLimits limits;
    nkuint32_t currentMemoryUsage;
    nkuint32_t peakMemoryUsage;

    struct NKMemoryHeader *allocations;

    jmp_buf catastrophicFailureJmpBuf;
};

/// Re-check all strings in the string table to see if they're in-use
/// by any program code. If program code has been removed that
/// references strings, then they can be made garbage-collectible.
void vmRescanProgramStrings(struct NKVM *vm);

const char *vmGetOpcodeName(enum NKOpcode op);

// ----------------------------------------------------------------------

/// Compiler internal function creation. Don't use this outside. Not
/// for that.
struct NKVMFunction *vmCreateFunction(struct NKVM *vm, nkuint32_t *functionId);

#endif // NINKASI_VM_H


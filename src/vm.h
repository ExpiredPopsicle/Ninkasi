#ifndef VM_H
#define VM_H

#include "error.h"
#include "vmstack.h"
#include "opcodes.h"
#include "vmstring.h"
#include "function.h"
#include "objects.h"

struct VM;

// ----------------------------------------------------------------------
// Public interface

/// Create and initialize a VM object.
struct VM *vmCreate(void);

/// Run the compiled program.
bool vmExecuteProgram(struct VM *vm);

/// De-initialize and free a VM object.
void vmDelete(struct VM *vm);

/// Get the number of errors that have occurred. Compile errors and
/// runtime errors are both stored here.
uint32_t vmGetErrorCount(struct VM *vm);

// TODO: Error string functions.

/// Run a single instruction inside the VM and advance the program
/// counter.
void vmIterate(struct VM *vm);

/// Force a garbage collection pass.
void vmGarbageCollect(struct VM *vm);

/// Call a function inside the VM. This does not do any kind of
/// iteration control, and will simply keep iterating until the
/// instruction pointer points to the end of addressable program
/// space, indicating that the function has returned.
///
/// arguments is an input pointing to the first element in an array of
///   argumentCount elements.
///
/// returnValue is an output pointing to the Value to fill with the
///   return value from the function call.
void vmCallFunction(
    struct VM *vm,
    struct Value *functionValue,
    uint32_t argumentCount,
    struct Value *arguments,
    struct Value *returnValue);

/// Create a C function and write it to some Value.
///
/// func is the C function pointer itself.
///
/// output is the Value to write the pointer to.
void vmCreateCFunction(
    struct VM *vm,
    VMFunctionCallback func,
    struct Value *output);

/// Look up a global variable. Do not use this before executing the
/// program at least once, or only accessing global variables that
/// were declared earlier in the program than whatever point it's at
/// now. (Global variables are created when the program reaches the
/// point where they are declared, and until that point, the stack
/// area they occupy may be used by other things, or may not exist at
/// all.)
struct Value *vmFindGlobalVariable(
    struct VM *vm, const char *name);

// ----------------------------------------------------------------------
// Internals

struct VMFunction;

struct VMLimits
{
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
};

void vmInit(struct VM *vm);
void vmDestroy(struct VM *vm);

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

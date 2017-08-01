#ifndef NINKASI_PUBLIC_H
#define NINKASI_PUBLIC_H

struct NKVM;
struct NKVMFunctionCallbackData;
typedef void (*VMFunctionCallback)(struct NKVMFunctionCallbackData *data);
struct NKValue;

#include "basetype.h"
#include "value.h"
#include "vm.h" // FIXME: Get rid of this

// ----------------------------------------------------------------------
// Public VM interface

void vmInit(struct NKVM *vm);

void vmDestroy(struct NKVM *vm);

/// Run the compiled program.
bool vmExecuteProgram(struct NKVM *vm);

/// Get the number of errors that have occurred. Compile errors and
/// runtime errors are both stored here.
uint32_t vmGetErrorCount(struct NKVM *vm);

// TODO: Error string functions.

/// Run a single instruction inside the VM and advance the program
/// counter.
void vmIterate(struct NKVM *vm);

/// Force a garbage collection pass.
void vmGarbageCollect(struct NKVM *vm);

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
    struct NKVM *vm,
    struct NKValue *functionValue,
    uint32_t argumentCount,
    struct NKValue *arguments,
    struct NKValue *returnValue);

/// Create a C function and write it to some Value.
///
/// func is the C function pointer itself.
///
/// output is the Value to write the pointer to.
void vmCreateCFunction(
    struct NKVM *vm,
    VMFunctionCallback func,
    struct NKValue *output);

/// Look up a global variable. Do not use this before executing the
/// program at least once, or only accessing global variables that
/// were declared earlier in the program than whatever point it's at
/// now. (Global variables are created when the program reaches the
/// point where they are declared, and until that point, the stack
/// area they occupy may be used by other things, or may not exist at
/// all.)
struct NKValue *vmFindGlobalVariable(
    struct NKVM *vm, const char *name);

// ----------------------------------------------------------------------
// Public compiler interface

struct NKCompilerState;
struct NKVM;

/// Create a compiler.
struct NKCompilerState *vmCompilerCreate(
    struct NKVM *vm);

/// Create a C function and assign it a variable name at the current
/// scope. Use this to make a globally defined C function at
/// compile-time. Do this before script compilation, so the script
/// itself can access it.
void vmCompilerCreateCFunctionVariable(
    struct NKCompilerState *cs,
    const char *name,
    VMFunctionCallback func,
    void *userData);

/// This can be done multiple times. It'll just be the equivalent of
/// appending each script onto the end, except for the line number
/// counts.
bool vmCompilerCompileScript(
    struct NKCompilerState *cs,
    const char *script);

// bool vmCompilerCompileScriptFile(
//     struct NKCompilerState *cs,
//     const char *scriptFilename);

/// Destroy a compiler. This will also finish off any remaining tasks
/// like setting up the global variable list in the VM.
void vmCompilerFinalize(
    struct NKCompilerState *cs);

// ----------------------------------------------------------------------
// Public types

struct NKVMFunctionCallbackData
{
    struct NKVM *vm;

    struct NKValue *arguments;
    uint32_t argumentCount;

    // Set this to something to return a value.
    struct NKValue returnValue;

    void *userData;
};

#endif


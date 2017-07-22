#ifndef NINKASI_PUBLIC_H
#define NINKASI_PUBLIC_H

struct VM;
struct VMFunctionCallbackData;
typedef void (*VMFunctionCallback)(struct VMFunctionCallbackData *data);
struct Value;

#include "basetype.h"
#include "value.h"
#include "vm.h" // FIXME: Get rid of this

// ----------------------------------------------------------------------
// Public VM interface

/// Create and initialize a VM object.
struct VM *vmCreate(void);

void vmInit(struct VM *vm);

void vmDestroy(struct VM *vm);

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
// Public compiler interface

struct CompilerState;
struct VM;

/// Create a compiler.
struct CompilerState *vmCompilerCreate(
    struct VM *vm);

/// Create a C function and assign it a variable name at the current
/// scope. Use this to make a globally defined C function at
/// compile-time. Do this before script compilation, so the script
/// itself can access it.
void vmCompilerCreateCFunctionVariable(
    struct CompilerState *cs,
    const char *name,
    VMFunctionCallback func,
    void *userData);

/// This can be done multiple times. It'll just be the equivalent of
/// appending each script onto the end, except for the line number
/// counts.
bool vmCompilerCompileScript(
    struct CompilerState *cs,
    const char *script);

bool vmCompilerCompileScriptFile(
    struct CompilerState *cs,
    const char *scriptFilename);

/// Destroy a compiler. This will also finish off any remaining tasks
/// like setting up the global variable list in the VM.
void vmCompilerFinalize(
    struct CompilerState *cs);

// ----------------------------------------------------------------------
// Public types

struct VMFunctionCallbackData
{
    struct VM *vm;

    struct Value *arguments;
    uint32_t argumentCount;

    // Set this to something to return a value.
    struct Value returnValue;

    void *userData;
};

#endif


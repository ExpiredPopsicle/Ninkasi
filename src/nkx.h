// ----------------------------------------------------------------------
// Ninkasi external interface (NKX).
// ----------------------------------------------------------------------
//
// These are the only functions that should ever be called by a
// hosting application. Every call in here wraps to an internal call
// after setting up an error handler with setjmp.
//
// Internally, the VM should NEVER call any of these wrapper
// functions, or it'll end up with the catastrophic error handler set
// up twice, and won't return to the hosting application correctly.

#ifndef NINKASI_NKX
#define NINKASI_NKX

#include "basetype.h"
#include "value.h"

struct VM;
struct VMFunctionCallbackData;
typedef void (*VMFunctionCallback)(struct VMFunctionCallbackData *data);
struct Value;

// ----------------------------------------------------------------------
// Public VM interface

/// Create and initialize a VM object.
struct VM *nkxVmCreate(void);

/// De-initialize and free a VM object.
void nkxVmDelete(struct VM *vm);

/// Run the compiled program.
bool nkxVmExecuteProgram(struct VM *vm);

/// Get the number of errors that have occurred. Compile errors and
/// runtime errors are both stored here.
uint32_t nkxVmGetErrorCount(struct VM *vm);

/// Run some number of instructions inside the VM and advance the
/// program counter.
void nkxVmIterate(struct VM *vm, uint32_t count);

/// Force a garbage collection pass.
void nkxVmGarbageCollect(struct VM *vm);

/// Call a function inside the VM. This does not do any kind of
/// iteration control, and will simply keep iterating until the
/// instruction pointer points to the end of addressable program
/// space, indicating that the function has returned.
///
/// functionValue is an input Value containing the function id.
///
/// arguments is an input pointing to the first element in an array of
///   argumentCount elements.
///
/// returnValue is an output pointing to the Value to fill with the
///   return value from the function call.
void nkxVmCallFunction(
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
void nkxVmCreateCFunction(
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
struct Value *nkxVmFindGlobalVariable(
    struct VM *vm, const char *name);

// ----------------------------------------------------------------------
// Public compiler interface

/// Create a compiler.
struct NKCompilerState *nkxVmCompilerCreate(
    struct VM *vm);

/// Create a C function and assign it a variable name at the current
/// scope. Use this to make a globally defined C function at
/// compile-time. Do this before script compilation, so the script
/// itself can access it.
void nkxVmCompilerCreateCFunctionVariable(
    struct NKCompilerState *cs,
    const char *name,
    VMFunctionCallback func,
    void *userData);

/// This can be done multiple times. It'll just be the equivalent of
/// appending each script onto the end, except for the line number
/// counts.
bool nkxVmCompilerCompileScript(
    struct NKCompilerState *cs,
    const char *script);

bool nkxVmCompilerCompileScriptFile(
    struct NKCompilerState *cs,
    const char *scriptFilename);

/// Destroy a compiler. This will also finish off any remaining tasks
/// like setting up the global variable list in the VM.
void nkxVmCompilerFinalize(
    struct NKCompilerState *cs);

const char *nkxValueToString(struct VM *vm, struct Value *value);

/// Force a catastrophic failure. This is mainly to test error
/// recovery by C functions and callbacks.
void nkxForceCatastrophicFailure(struct VM *vm);

#endif // NINKASI_NKX

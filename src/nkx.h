// ----------------------------------------------------------------------
//
//        ▐ ▄ ▪   ▐ ▄ ▄ •▄  ▄▄▄· .▄▄ · ▪
//       •█▌▐███ •█▌▐██▌▄▌▪▐█ ▀█ ▐█ ▀. ██
//       ▐█▐▐▌▐█·▐█▐▐▌▐▀▀▄·▄█▀▀█ ▄▀▀▀█▄▐█·
//       ██▐█▌▐█▌██▐█▌▐█.█▌▐█ ▪▐▌▐█▄▪▐█▐█▌
//       ▀▀ █▪▀▀▀▀▀ █▪·▀  ▀ ▀  ▀  ▀▀▀▀ ▀▀▀
//
// ----------------------------------------------------------------------
//
//   Ninkasi 0.01
//
//   By Cliff "ExpiredPopsicle" Jolly
//     https://expiredpopsicle.com
//     https://intoxicoding.com
//     expiredpopsicle@gmail.com
//
// ----------------------------------------------------------------------
//
//   Copyright (c) 2017 Clifford Jolly
//
//   Permission is hereby granted, free of charge, to any person
//   obtaining a copy of this software and associated documentation files
//   (the "Software"), to deal in the Software without restriction,
//   including without limitation the rights to use, copy, modify, merge,
//   publish, distribute, sublicense, and/or sell copies of the Software,
//   and to permit persons to whom the Software is furnished to do so,
//   subject to the following conditions:
//
//   The above copyright notice and this permission notice shall be
//   included in all copies or substantial portions of the Software.
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
//   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
//   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//   SOFTWARE.
//
// -------------------------- END HEADER -------------------------------------

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
//
// In theory, this is the only file that should be #included when
// embedding Ninkasi into a hosting application.

#ifndef NINKASI_NKX
#define NINKASI_NKX

#include "nktypes.h"
#include "nkvalue.h"

struct NKVM;
struct NKVMFunctionCallbackData;
typedef void (*NKVMFunctionCallback)(struct NKVMFunctionCallbackData *data);
struct NKValue;

// ----------------------------------------------------------------------
// Public VM interface

/// Create and initialize a VM object. (Simple version. Just uses
/// malloc/free directly.)
struct NKVM *nkxVmCreate(void);

/// Creation parameters for advanced setup.
struct NKVMCreateParams
{
    void *(*mallocReplacement)(nkuint32_t size, void *userData);
    void (*freeReplacement)(void *ptr, void *userData);
    void *mallocAndFreeReplacementUserData;
};

/// Advanced version of nkxVmCreate(). Use this if you want to use a
/// different allocator than malloc() and free().
struct NKVM *nkxVmCreateEx(
    struct NKVMCreateParams *params);

/// De-initialize and free a VM object.
void nkxVmDelete(struct NKVM *vm);

/// Run the compiled program.
nkbool nkxVmExecuteProgram(struct NKVM *vm);

/// Get the number of errors that have occurred. Compile errors and
/// runtime errors are both stored here.
nkuint32_t nkxVmGetErrorCount(struct NKVM *vm);

/// Get whether or not the VM has any errors.
nkbool nkxVmHasErrors(struct NKVM *vm);

/// Run some number of instructions inside the VM and advance the
/// program counter.
void nkxVmIterate(struct NKVM *vm, nkuint32_t count);

/// Force a garbage collection pass.
void nkxVmGarbageCollect(struct NKVM *vm);

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
    struct NKVM *vm,
    struct NKValue *functionValue,
    nkuint32_t argumentCount,
    struct NKValue *arguments,
    struct NKValue *returnValue);

/// Look up a global variable. Do not use this before executing the
/// program at least once, or only accessing global variables that
/// were declared earlier in the program than whatever point it's at
/// now. (Global variables are created when the program reaches the
/// point where they are declared, and until that point, the stack
/// area they occupy may be used by other things, or may not exist at
/// all.)
struct NKValue *nkxVmFindGlobalVariable(
    struct NKVM *vm, const char *name);

/// Convert a value to a string. This will return a pointer to an
/// internal string table entry (which it may have to create), and the
/// address returned may be freed in the next garbage collection pass.
/// So do not free it yourself, and save a copy of it if you wish to
/// hold onto it.
const char *nkxValueToString(struct NKVM *vm, struct NKValue *value);

/// Force a catastrophic failure. This is mainly to test error
/// recovery by C functions and callbacks.
void nkxForceCatastrophicFailure(struct NKVM *vm);

/// Convenience function for C function callbacks. Check the argument
/// count that a function was called with. If it does not match, an
/// error will be added and this function will return false. Otherwise
/// it will return nktrue.
nkbool nkxFunctionCallbackCheckArgCount(
    struct NKVMFunctionCallbackData *data,
    nkuint32_t argCount,
    const char *functionName);

/// Increment the reference count for an object. This keeps it (and
/// everything referenced from it) from being garbage collected.
void nkxVmObjectAcquireHandle(struct NKVM *vm, struct NKValue *value);

/// Decrement the reference count for an object. Objects that reach
/// zero references and have no owning references inside the VM will
/// be deleted next garbage collection pass.
void nkxVmObjectReleaseHandle(struct NKVM *vm, struct NKValue *value);

// ----------------------------------------------------------------------
// Native C function call interface.

/// This structure is passed into C functions called from inside the
/// VM, as a pointer. The C functions are expected to take no other
/// parameters, and decode the arguments from the list.
struct NKVMFunctionCallbackData
{
    struct NKVM *vm;

    struct NKValue *arguments;
    nkuint32_t argumentCount;

    // Set this to something to return a value.
    struct NKValue returnValue;
};

/// Set an arbitrary pointer to associate with the VM. This will let
/// you associate VMs with some context and have them visible from
/// called functions without horrible global variable hacks. This
/// pointer is not used by anything inside the VM.
void nkxSetUserData(struct NKVM *vm, void *userData);

/// Get the user data pointer set with nkxSetUserData.
void *nkxGetUserData(struct NKVM *vm);

/// Set a native (C-side) callback to fire off immediately before an
/// object is deleted from the VM. callbackFunction is the native
/// function id returned from nkxVmRegisterExternalFunction().
void nkxVmObjectSetGarbageCollectionCallback(
    struct NKVM *vm,
    struct NKValue *object,
    nkuint32_t callbackFunction);

/// Register a new external function. You should do this before
/// compiling or deserializing. It may also take a long time searching
/// for duplicates. You may have to use this if you do not know if a
/// function exists or not inside the VM yet, or if you do know and
/// need the ID to be found. The function ID returned from this is an
/// index into the VM's native (external) callback table, NOT the
/// table of all functions.
nkuint32_t nkxVmRegisterExternalFunction(
    struct NKVM *vm,
    const char *name,
    NKVMFunctionCallback func);

/// Look up or create an internal function to represent some external
/// function. This should execute fast (no searching), but may have to
/// instantiate a new function object. Use this to map external native
/// functions to VM function IDs that can be assigned to function
/// objects (functionId on NKValue).
nkuint32_t nkxVmGetOrCreateInternalFunctionForExternalFunction(
    struct NKVM *vm, nkuint32_t externalFunctionId);

// ----------------------------------------------------------------------
// Public compiler interface

/// Create a compiler.
struct NKCompilerState *nkxCompilerCreate(
    struct NKVM *vm);

/// Create a C function and assign it a variable name at the current
/// scope. Use this to make a globally defined C function at
/// compile-time. Do this before script compilation, so the script
/// itself can access it.
void nkxCompilerCreateCFunctionVariable(
    struct NKCompilerState *cs,
    const char *name,
    NKVMFunctionCallback func);

/// This can be done multiple times. It'll just be the equivalent of
/// appending each script onto the end, except for the line number
/// counts.
nkbool nkxCompilerCompileScript(
    struct NKCompilerState *cs,
    const char *script);

nkbool nkxCompilerCompileScriptFile(
    struct NKCompilerState *cs,
    const char *scriptFilename);

/// Destroy a compiler. This will also finish off any remaining tasks
/// like setting up the global variable list in the VM.
void nkxCompilerFinalize(
    struct NKCompilerState *cs);

#endif // NINKASI_NKX

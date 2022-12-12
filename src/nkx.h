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
//   By Kiri "ExpiredPopsicle" Jolly
//     https://expiredpopsicle.com
//     https://intoxicoding.com
//     expiredpopsicle@gmail.com
//
// ----------------------------------------------------------------------
//
//   Copyright (c) 2017 Kiri Jolly
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

#include <stdio.h>

#include "nktypes.h"
#include "nkvalue.h"

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

/// Look up a global variable.
///
/// Note that global variables defined in the script may not be filled
/// with their initial value until that code has been reached in the
/// script.
///
/// Note that the returned pointer can become invalid if more
/// variables are added, because the VM may reallocate the static
/// variable space inside the VM.
///
/// The "cs" parameter may be NULL if no compiler is active, but must
/// be used if nkxCompilerFinalize() has not been called yet.
///
/// Returns NULL if the variable does not exist.
struct NKValue *nkxVmFindGlobalVariable(
    struct NKVM *vm,
    struct NKCompilerState *cs,
    const char *name);

/// Convert a value to a string. This will return a pointer to an
/// internal string table entry (which it may have to create), and the
/// address returned may be freed in the next garbage collection pass.
/// So do not free it yourself, and save a copy of it if you wish to
/// hold onto it.
const char *nkxValueToString(struct NKVM *vm, struct NKValue *value);

nkint32_t nkxValueToInt(struct NKVM *vm, struct NKValue *value);

float nkxValueToFloat(struct NKVM *vm, struct NKValue *value);

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

/// Get the number of active external handles for an object.
nkuint32_t nkxVmObjectGetExternalHandleCount(struct NKVM *vm, struct NKValue *value);

/// Write an integer into an NKValue.
void nkxValueSetInt(struct NKVM *vm, struct NKValue *value, nkint32_t intData);

/// Write a float into an NKValue.
void nkxValueSetFloat(struct NKVM *vm, struct NKValue *value, float floatData);

/// Write a string into an NKValue. This actually finds or creates a
/// string table entry for the given string, then assigns the ID of
/// that entry to the value.
void nkxValueSetString(struct NKVM *vm, struct NKValue *value, const char *str);

/// Write a function ID into an NKValue.
void nkxValueSetFunction(struct NKVM *vm, struct NKValue *value, NKVMInternalFunctionID id);

/// Write a nil into an NKValue.
void nkxValueSetNil(struct NKVM *vm, struct NKValue *value);

/// Clear a field on an object.
void nkxVmObjectClearEntry(
    struct NKVM *vm,
    struct NKValue *objectId,
    struct NKValue *key);

// TODO: Add some more convenience functions for various index types
// so we don't have to make the NKValue directly.

/// Get a pointer to a field on an object. This pointer may be
/// invalidated once VM execution resumes!
struct NKValue *nkxVmObjectFindOrAddEntry(
    struct NKVM *vm,
    struct NKValue *objectId,
    struct NKValue *key,
    nkbool noAdd);

/// Get the number of key/value pairs in a given object.
nkuint32_t nkxVmObjectGetSize(
    struct NKVM *vm,
    struct NKValue *objectId);

/// Returns true if the program has ended. If there are no errors, it
/// is still valid to call functions in the VM. In many cases this
/// just means the program has set up all its event callbacks and is
/// waiting, but it has still reached the end of the script and seen
/// an "end" instruction.
nkbool nkxVmProgramHasEnded(struct NKVM *vm);

/// Returns true if an allocation error has occurred.
nkbool nkxVmHasAllocationFailure(struct NKVM *vm);

/// Returns the current memory usage by the VM. It's just the
/// cumulative value of all the allocation call sizes.
nkuint32_t nkxVmGetCurrentMemoryUsage(struct NKVM *vm);

/// Returns the peak memory usage by the VM.
nkuint32_t nkxVmGetPeakMemoryUsage(struct NKVM *vm);

// ----------------------------------------------------------------------
// Limits-related stuff

// Note: The first limit to set for sandboxing purposes should always
// be the maximum allocated memory (nkxSetMaxAllocatedMemory()). The
// next should be the instruction count limit
// (nkxSetRemainingInstructionLimit()). These set effective limits for
// the entire system, instead of limits for specific subsystems.

/// Set the maximum stack size. Only affects newly pushed values.
void nkxSetMaxStackSize(struct NKVM *vm, nkuint32_t maxStackSize);
nkuint32_t nkxGetMaxStackSize(struct NKVM *vm);

/// Set the maximum number of fields per object. Only affects newly
/// created fields.
void nkxSetMaxFieldsPerObject(struct NKVM *vm, nkuint32_t maxFieldsPerObject);
nkuint32_t nkxGetMaxFieldsPerObject(struct NKVM *vm);

/// Set the maximum allocated memory, ONLY counting the actual number
/// of bytes requested and freed. Only affects new allocations.
void nkxSetMaxAllocatedMemory(struct NKVM *vm, nkuint32_t maxAllocatedMemory);
nkuint32_t nkxGetMaxAllocatedMemory(struct NKVM *vm);

/// Set the garbage collection interval, in number of instructions
/// executed. Garbage collection will only occur if enough new objects
/// have been created in that time to exceed the "new object interval"
/// also.
void nkxSetGarbageCollectionInterval(struct NKVM *vm, nkuint32_t gcInterval);
nkuint32_t nkxGetGarbageCollectionInterval(struct NKVM *vm);

/// Set the number of objects that can be created before triggering
/// the next garbage collection pass.
void nkxSetGarbageCollectionNewObjectInterval(struct NKVM *vm, nkuint32_t gcNewObjectInterval);
nkuint32_t nkxGetGarbageCollectionNewObjectInterval(struct NKVM *vm);

/// Set the number of instructions to execute before throwing an
/// error. Set to NK_INVALID_VALUE for no limit (default). Note that
/// this limit is only checked on intervals where the garbage
/// collector may activate for performance reasons. Its granularity is
/// affected by NKGarbageCollectionInfo::gcInterval (settable with
/// nkxSetGarbageCollectionInterval()).
void nkxSetRemainingInstructionLimit(struct NKVM *vm, nkuint32_t count);

/// Get the number of instructions left from
/// nkxSetRemainingInstructionLimit. Note that this value is only
/// updated on garbage collection intervals!
nkuint32_t nkxGetRemainingInstructionLimit(struct NKVM *vm);

// ----------------------------------------------------------------------
// Native C function call interface

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

/// Register an internal function, assocate it with an internal
/// function ID, and create a global variable for it.
///
/// "cs" may be NULL, in which case all of the variable creation and
/// internal function ID creation are skipped.
///
/// If setupGlobalvariable is set to nktrue, then a global variable
/// will be set up with the same name as the function internal name.
/// This requires the compiler to be present (cs != NULL).
///
/// This function takes a variable number of arguments. Those
/// arguments are types to perform argument count and type checking
/// against. Specify NK_INVALID_VALUE for argumentCount to disable
/// argument count and type checking. Otherwise, set argumentCount to
/// the number of arguments, and then for each argument, specify a
/// type.
///
/// For fields where type should not be type-checked, use
/// NK_VALUETYPE_NIL.
///
/// For fields that take an NK_VALUETYPE_OBJECTID, an extra field must
/// be present afterward indicating the external type ID, or
/// NK_INVALID_VALUE to skip external object type checking. See
/// nkxVmRegisterExternalType() to understand external data types.
///
/// Example:
///   nkxVmSetupExternalFunction(vm, cs, "testFunctionName",
///       testFunctionPointer,   // Function pointer.
///       nktrue,                // Global variable.
///       5,                     // 5 parameters.
///       NK_VALUETYPE_INT,      // Integer field.
///       NK_VALUETYPE_STRING,   // String field
///       NK_VALUETYPE_NIL,      // Un-checked wildcard field.
///       // Object with specific external data attached to it.
///       NK_VALUETYPE_OBJECTID, someExternalTypeId,
///       // Object, which may contain external data, but either way
///       // we don't care. We just want an object.
///       NK_VALUETYPE_OBJECTID, NK_INVALID_VALUE);
NKVMExternalFunctionID nkxVmSetupExternalFunction(
    struct NKVM *vm, struct NKCompilerState *cs,
    const char *name, NKVMFunctionCallback func,
    nkbool setupGlobalVariable,
    nkuint32_t argumentCount, ...);

/// Note: See nkxVmSetupExternalFunction(). That is probably what
/// you're looking for.
///
/// Register a new external function. You should do this before
/// compiling or deserializing. It may also take a long time searching
/// for duplicates. You may have to use this if you do not know if a
/// function exists or not inside the VM yet, or if you do know and
/// need the ID to be found. The function ID returned from this is an
/// index into the VM's native (external) callback table, NOT the
/// table of all functions.
///
/// This is the low-level interface to the external function table.
/// This does not create a corresponding text-script-accessible
/// function on its own. Functions created with this will be
/// accessible to serialized scripts that already had the same
/// function registered prior to serialization.
///
/// To make a function that is immediately usable to scripts as a
/// global variable, see nkxVmSetupExternalFunction().
///
/// To register an internal function corresponding to this external
/// function (for applying to other values that may not be global
/// variables), see
/// nkxVmGetOrCreateInternalFunctionForExternalFunction().
NKVMExternalFunctionID nkxVmRegisterExternalFunction(
    struct NKVM *vm,
    const char *name,
    NKVMFunctionCallback func);

/// Look up or create an internal function to represent some external
/// function. This should execute fast (no searching), but may have to
/// instantiate a new function object. Use this to map external native
/// functions to VM function IDs that can be assigned to function
/// objects (functionId on NKValue).
NKVMInternalFunctionID nkxVmGetOrCreateInternalFunctionForExternalFunction(
    struct NKVM *vm, NKVMExternalFunctionID externalFunctionId);

/// Decode arguments for an external function call. Returns nkfalse on
/// error. expectedArgumentCount not matching the passed argument
/// count is an error.
///
/// Errors in parameter decoding are automatically added to the VM
/// state. See nkxAddError for details on how errors are added.
///
/// Varargs arguments are NKValueTypes, corresponding to expected
/// argument types.
///
/// Example usage:
///
///   if(!nkxFunctionCallbackDecodeArguments(
///       data, "someGenericFunction", 3,
///       NK_VALUETYPE_INT,
///       NK_VALUETYPE_FLOAT,
///       NK_VALUETYPE_STRING)) return;
///
nkbool nkxFunctionCallbackCheckArguments(
    struct NKVMFunctionCallbackData *data,
    const char *functionName,
    nkuint32_t expectedArgumentCount,
    ...);

/// Decodes and type-checks a piece of external data on an object.
/// Adds an error to the VM state and returns NULL if the type does
/// not match.
void *nkxFunctionCallbackGetExternalDataArgument(
    struct NKVMFunctionCallbackData *data,
    const char *functionName,
    nkuint32_t argumentNumber,
    NKVMExternalDataTypeID externalDataType);

// ----------------------------------------------------------------------
// External data interface

/// Call this rarely. It will cause a search of all the existing types
/// to make sure duplicates don't creep in. It's an
/// initialization-time thing.
///
/// serializationCallback is called when this object type is getting
/// serialized or deserialized. Use this to save or load the contents
/// of any buffers or external data associated with the object.
///
/// cleanupCallback is called when the object is being destroyed by
/// the garbage collector. Use this to clean up extra buffers or data
/// associated with the object.
///
/// gcMarkCallback is called when the object is marked as in-use by
/// the garbage collector. Use it to mark any associated content that
/// you don't want to be destroyed by the garbage collector (eg other
/// objects referenced by native data structures).
NKVMExternalDataTypeID nkxVmRegisterExternalType(
    struct NKVM *vm, const char *name,
    NKVMExternalObjectSerializationCallback serializationCallback,
    NKVMExternalObjectCleanupCallback cleanupCallback,
    NKVMExternalObjectGCMarkCallback gcMarkCallback);

/// Search through all existing types for a matching name. Returns a
/// NKVMExternalDataTypeID with NK_INVALID_VALUE on failure.
NKVMExternalDataTypeID nkxVmFindExternalType(
    struct NKVM *vm, const char *name);

/// Get a type name.
const char *nkxVmGetExternalTypeName(
    struct NKVM *vm, NKVMExternalDataTypeID id);

void nkxCreateObject(
    struct NKVM *vm,
    struct NKValue *outValue);

/// Set the external type of an object.
nkbool nkxVmObjectSetExternalType(
    struct NKVM *vm,
    struct NKValue *object,
    NKVMExternalDataTypeID externalType);

/// Get the external type of an object.
NKVMExternalDataTypeID nkxVmObjectGetExternalType(
    struct NKVM *vm,
    struct NKValue *object);

/// Set a pointer to some external data to associate with this object.
nkbool nkxVmObjectSetExternalData(
    struct NKVM *vm,
    struct NKValue *object,
    void *data);

/// Get the previously set external data (NULL if not set).
void *nkxVmObjectGetExternalData(
    struct NKVM *vm,
    struct NKValue *object);

/// Use this to iterate through all the objects in the VM of a
/// specific external data type.
///
/// The value pointed to by startIndex will change with each call.
///
/// This function returns nkfalse on error, or when the iteration has
/// reached the end of the list.
///
/// Note: This is usuable in emergency cleanup (allocation failure)
/// conditions, meaning that it can be used for an external
/// subsystem's (see nkxInitSubsystem()) cleanupCallback to find and
/// destroy lingering objects that it owns but does not hold separate
/// references to.
nkbool nkxGetNextObjectOfExternalType(
    struct NKVM *vm,
    struct NKVMExternalDataTypeID type,
    struct NKValue *outValue,
    nkuint32_t *startIndex);

/// Mark a value as in-use during the garbage collection process. This
/// should only be called from a NKVMExternalObjectGCMarkCallback type
/// callback. Used for marking externally referenced data.
void nkxVmGarbageCollect_markValue(
    struct NKVM *vm,
    struct NKVMGCState *gcState,
    struct NKValue *value);

// ----------------------------------------------------------------------
// External subsystem stuff.

/// Get a pointer to a subsystem's external data by name. Returns NULL
/// if it is not set (or is set to NULL).
void *nkxGetExternalSubsystemData(
    struct NKVM *vm,
    const char *name);

/// Same as above, but adds an error if the subsystem data does not
/// exist. (Just to reduce function callback boilterplate.)
void *nkxGetExternalSubsystemDataOrError(
    struct NKVM *vm,
    const char *name);

/// Register an external subsystem.
///
/// internalData is a user data pointer that will be associated with
/// the subsystem.
///
/// cleanupCallback and serializationCallback will be used to cleanup
/// the subsystem on VM destruction, or serialize/deserialize the VM,
/// respectively.
///
/// Returns nkfalse on error, or nktrue on success.
nkbool nkxInitSubsystem(
    struct NKVM *vm,
    struct NKCompilerState *cs,
    const char *name,
    void *internalData,
    NKVMSubsystemCleanupCallback cleanupCallback,
    NKVMSubsystemSerializationCallback serializationCallback);

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

/// Create a global variable.
///
/// Note that the returned pointer can become invalid if more
/// variables are added, because the VM may reallocate the static
/// variable space inside the VM.
struct NKValue *nkxCompilerCreateGlobalVariable(
    struct NKCompilerState *cs,
    const char *name);

/// This can be done multiple times. It'll just be the equivalent of
/// appending each script onto the end, except for the line number
/// counts.
nkbool nkxCompilerCompileScript(
    struct NKCompilerState *cs,
    const char *script,
    const char *filename);

/// This is a convenience wrapper around nkxCompilerCompileScript().
/// Note that it uses the system heap through malloc() instead of the
/// normal VM internal allocation mechanism.
nkbool nkxCompilerCompileScriptFile(
    struct NKCompilerState *cs,
    const char *scriptFilename);

/// Destroy a compiler. This will also finish off any remaining tasks
/// like setting up the global variable list in the VM.
void nkxCompilerFinalize(
    struct NKCompilerState *cs);

// ----------------------------------------------------------------------
// Serializer

/// Serialize the entire VM state.
nkbool nkxVmSerialize(
    struct NKVM *vm,
    NKVMSerializationWriter writer,
    void *userdata,
    nkbool writeMode);

/// Shrink a VM's memory usage, if we can reduce the size of some of
/// the tables.
void nkxVmShrink(struct NKVM *vm);

/// Returns nktrue if the VM is currenty writing data out, and nkfalse
/// if the VM is currently reading data in. Used within serialization
/// callbacks.
nkbool nkxSerializerGetWriteMode(struct NKVM *vm);

/// Read in or write out a chunk of data, depending on current write
/// mode. See nkxSerializerGetWriteMode().
nkbool nkxSerializeData(struct NKVM *vm, void *data, nkuint32_t size);

// ----------------------------------------------------------------------
// Public-facing error stuff

/// Get the number of errors that have occurred. Compile errors and
/// runtime errors are both stored here.
nkuint32_t nkxGetErrorCount(struct NKVM *vm);

/// Manually add an error to the error list.
void nkxAddError(
    struct NKVM *vm,
    const char *str);

/// Manually add an error to the error list. Override the default
/// filename and line number handling. For adding errors from a
/// preprocessor stage.
void nkxAddErrorEx(
    struct NKVM *vm,
    const char *str,
    const char *filename,
    nkuint32_t lineNumber);

/// Get whether or not the VM has any errors.
nkbool nkxVmHasErrors(struct NKVM *vm);

/// Get the length of the buffer that would be needed to store every
/// accumulated error so far, including a null terminator.
nkuint32_t nkxGetErrorLength(struct NKVM *vm);

/// Read all the errors into a buffer with newlines between each
/// error.
void nkxGetErrorText(struct NKVM *vm, char *buffer);

// ----------------------------------------------------------------------
// Debugger-related stuff

/// Dump the entire (known) state of the VM. For comparison when
/// testing serialized data.
void nkxDbgDumpState(struct NKVM *vm, const char *script, FILE *stream);

// ----------------------------------------------------------------------
// Memory stuff

/// Allocate memory. This goes through the VM's malloc callback
/// assigned by the hosting application, and is tracked by the VM as
/// an allocation for leak detection. The VM memory usage limits will
/// also apply and an allocation failure will be tracked as any other
/// VM allocation failure.
void *nkxMalloc(
    struct NKVM *vm, nkuint32_t size);

/// Allocate an array. Use this instead of multiplying size and count
/// yourself. This will detect integer overflow from the
/// multiplication and return NULL if the value cannot be expressed in
/// a nkuint32_t.
void *nkxMallocArray(
    struct NKVM *vm, nkuint32_t size, nkuint32_t count);

/// Free memory.
void nkxFree(struct NKVM *vm, void *data);

/// Reallocate memory.
void *nkxRealloc(struct NKVM *vm, void *data, nkuint32_t size);

/// Reallocate an array. See nkxMallocArray() for why we do this.
void *nkxReallocArray(
    struct NKVM *vm, void *data, nkuint32_t size, nkuint32_t count);

/// Duplicate a C string.
char *nkxStrdup(struct NKVM *vm, const char *str);

#endif // NINKASI_NKX

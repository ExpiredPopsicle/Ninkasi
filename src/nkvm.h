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

#ifndef NINKASI_VM_H
#define NINKASI_VM_H

#include <setjmp.h>

#include "nkmem.h"

#include "nkerror.h"
#include "nkstack.h"
#include "nkopcode.h"
#include "nkstring.h"
#include "nkfunc.h"
#include "nkobjs.h"

#include "nktypes.h"
#include "nkvalue.h"
#include "nkx.h"

struct NKVMFunction;
struct NKMemoryHeader;

struct NKVMLimits
{
    // Many of these values are only checked when a section needs to
    // expand (double) its memory usage. So only powers of two are
    // meaningful numbers for string count, stack size, and object
    // count.
    nkuint32_t maxStackSize;
    nkuint32_t maxFieldsPerObject;
    nkuint32_t maxAllocatedMemory;
};

struct NKGarbageCollectionInfo
{
    nkuint32_t lastGCPass;
    nkuint32_t gcInterval; // TODO: Move to VMLimits, maybe.
    nkuint32_t gcCountdown;
    nkuint32_t gcNewObjectInterval; // TODO: Move to VMLimits, maybe.
    nkuint32_t gcNewObjectCountdown;
};

struct NKVMExternalSubsystemData
{
    char *name;

    // Called by the serialization system.
    NKVMSubsystemSerializationCallback serializationCallback;

    // Called on VM destruction. This function is responsible for
    // cleaning up the data stored on this object, but also objects
    // owned by this system in the case of an allocation failure. The
    // reason for this is that the normal garbage collector cleanup
    // function cannot run once the allocation failure flag has been
    // set.
    NKVMSubsystemCleanupCallback cleanupCallback;

    void *data;

    struct NKVMExternalSubsystemData *nextInHashTable;
};

struct NKGlobalVariableRecord
{
    nkuint32_t staticPosition;
    char *name;
};

struct NKVMExternalType
{
    char *name;
    NKVMExternalObjectSerializationCallback serializationCallback;
    NKVMExternalObjectCleanupCallback cleanupCallback;
    NKVMExternalObjectGCMarkCallback gcMarkCallback;
};

// Debug position marker so we know where errors happened. One of
// these will correspond to a block of instructions. It will take a
// while to search through to find the marker for a specific
// instruction, but it'll use a heck of a lot less memory than the way
// we've been store file and line number info on every single
// instruction.
struct NKVMFilePositionMarker
{
    // Index into NKVM::instructions array for the start of a block of
    // instrucions that come from a specific line and file.
    nkuint32_t instructionIndex;

    // Index into the NKVM::sourceFileList array, indicating which
    // file the instruction (and following instructions) ccome from.
    nkuint32_t fileIndex;

    // Line number the instructions come from.
    nkuint32_t lineNumber;

    // Line number the instructions come from, ignoring #file and
    // #line directives.
    nkuint32_t actualLineNumber;
};

enum NKVMCoroutineState
{
    // Not a coroutine. Probably the root context.
    NK_COROUTINE_INVALID,

    // Created, but not yet resumed. In this state we don't push
    // anything onto the coroutine's own stack, because it has not
    // executed a yield() from which to return a value.
    NK_COROUTINE_CREATED,

    // Created and has executed. From here on, all resumes will leave
    // a value on the stack that was passed in from the parent
    // context.
    NK_COROUTINE_RUNNING,

    // The coroutine has finished. All calls to resume at this point
    // should do nothing and return nil.
    NK_COROUTINE_FINISHED,
};

struct NKVMExecutionContext
{
    // Stack data.
    struct NKVMStack stack;

    // Instruction pointer.
    nkuint32_t instructionPointer;

    // Parent context. This is the one to return to when yielding,
    // returning, or ending.
    struct NKVMExecutionContext *parent;

    // Object that corresponds to whatever coroutine or thread we're
    // in. This will be nil instead of an object for the root
    // execution context.
    struct NKValue coroutineObject;

    // This corresponds to the NKVMCoroutineState enum.
    nkuint32_t coroutineState;
};

/// The VM object itself.
struct NKVM
{
    // Errors.
    struct NKErrorState errorState;

    struct NKVMExecutionContext rootExecutionContext;
    struct NKVMExecutionContext *currentExecutionContext;

    // Static data.
    struct NKValue *staticSpace;
    nkuint32_t staticAddressMask;

    // Instructions.
    struct NKInstruction *instructions;
    nkuint32_t instructionAddressMask;

    // Strings.
    struct NKVMTable stringTable;
    struct NKVMString *stringsByHash[nkiVmStringHashTableSize];

    // Objects.
    struct NKVMTable objectTable;
    struct NKVMObject *objectsWithExternalHandles;

    // Functions.
    nkuint32_t functionCount;
    struct NKVMFunction *functionTable;

    // Garbage collection status.
    struct NKGarbageCollectionInfo gcInfo;

    // External C functions. This is a block that represents every
    // external C function linked to the VM, but separate from the
    // main function table. Some of these may not have functions in
    // the function table representing them. This table should be
    // filled in prior to any kind of deserialization, so that loaded
    // functions can be mapped to real functions by name. Use
    // nkiVmRegisterExternalFunction (or the nkx* equivalent) to fill
    // it in.
    nkuint32_t externalFunctionCount;
    struct NKVMExternalFunction *externalFunctionTable;

    // TODO: Add a global variable table and global variable lookup,
    // so we don't have to keep the compiler around.
    struct NKGlobalVariableRecord *globalVariables;
    nkuint32_t globalVariableCount;

    // Memory usage stuff.

    struct NKVMLimits limits;
    nkuint32_t currentMemoryUsage;
    nkuint32_t peakMemoryUsage;

    struct NKMemoryHeader *allocations;
#if NK_EXTRA_FANCY_LEAK_TRACKING_LINUX
    nkuint32_t allocationCount;
#endif // NK_EXTRA_FANCY_LEAK_TRACKING_LINUX

    void *(*mallocReplacement)(nkuint32_t size, void *userData);
    void (*freeReplacement)(void *ptr, void *userData);
    void *mallocAndFreeReplacementUserData;

    jmp_buf *catastrophicFailureJmpBuf;

    void *userData;

    nkuint32_t externalTypeCount;
    struct NKVMExternalType *externalTypes;

    struct
    {
        NKVMSerializationWriter writer;
        void *userdata;
        nkbool writeMode;
    } serializationState;

    nkuint32_t instructionsLeftBeforeTimeout;

    struct NKVMExternalSubsystemData *subsystemDataTable[nkiVmExternalSubsystemHashTableSize];

    // Source code file names, for error reporting and debugging.
    char **sourceFileList;
    nkuint32_t sourceFileCount;

    // Position markers, so we know what file/line any given
    // instruction comes from.
    struct NKVMFilePositionMarker *positionMarkerList;
    nkuint32_t positionMarkerCount;

    // Internal data types.
    struct
    {
        NKVMExternalDataTypeID coroutine;
    } internalObjectTypes;
};

/// Initialize an already-allocated VM.
void nkiVmInit(struct NKVM *vm);

/// De-initialize a VM. Does not deallocate the VM structure.
void nkiVmDestroy(struct NKVM *vm);

const char *nkiVmGetOpcodeName(enum NKOpcode op);

void nkiVmStaticDump(struct NKVM *vm);

/// Clear and free the source file list. This info isn't really needed
/// in a completely error-free program, because it's mainly used for
/// error reporting.
void nkiVmClearSourceFileList(struct NKVM *vm);

/// Add a source file to the source file list and return the index in
/// the list. If the file is already present in the list, just return
/// the index of that file.
nkuint32_t nkiVmAddSourceFile(struct NKVM *vm, const char *filename);

/// Find the source file and line marker for a given instructtion.
struct NKVMFilePositionMarker *nkiVmFindSourceMarker(struct NKVM *vm, nkuint32_t instructionIndex);

/// Find the source file and line marker for the current instructtion.
struct NKVMFilePositionMarker *nkiVmFindCurrentSourceMarker(struct NKVM *vm);

/// Initialize an NKVMExecutionContext object.
void nkiVmInitExecutionContext(
    struct NKVM *vm,
    struct NKVMExecutionContext *context);

/// Tear down an NKVMExecutionContext object.
void nkiVmDeinitExecutionContext(
    struct NKVM *vm,
    struct NKVMExecutionContext *context);

// ----------------------------------------------------------------------

/// Run the compiled program.
nkbool nkiVmExecuteProgram(struct NKVM *vm);

// TODO: Error string functions.

/// Run a single instruction inside the VM and advance the program
/// counter.
void nkiVmIterate(struct NKVM *vm);

/// Force a garbage collection pass.
void nkiVmGarbageCollect(struct NKVM *vm);

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
void nkiVmCallFunction(
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
struct NKValue *nkiVmFindGlobalVariable(
    struct NKVM *vm, const char *name);

// Returns nktrue if the instruction pointer is pointing at an "end"
// instruction.
nkbool nkiVmProgramHasEnded(struct NKVM *vm);

// ----------------------------------------------------------------------
// External data interface

/// Call this rarely. It will cause a search of all the existing types
/// to make sure duplicates don't creep in. It's an
/// initialization-time thing.
NKVMExternalDataTypeID nkiVmRegisterExternalType(
    struct NKVM *vm, const char *name,
    NKVMExternalObjectSerializationCallback serializationCallback,
    NKVMExternalObjectCleanupCallback cleanupCallback,
    NKVMExternalObjectGCMarkCallback gcMarkCallback);

/// Search through all existing types for a matching name. Returns a
/// NKVMExternalDataTypeID with NK_INVALID_VALUE on failure.
NKVMExternalDataTypeID nkiVmFindExternalType(
    struct NKVM *vm, const char *name);

/// Get a type name.
const char *nkiVmGetExternalTypeName(
    struct NKVM *vm, NKVMExternalDataTypeID id);

void *nkiGetExternalSubsystemData(
    struct NKVM *vm,
    const char *name);

struct NKVMExternalSubsystemData *nkiFindExternalSubsystemData(
    struct NKVM *vm,
    const char *name,
    nkbool create);

#endif // NINKASI_VM_H


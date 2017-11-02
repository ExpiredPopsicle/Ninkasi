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

#ifndef NINKASI_FUNCTION_H
#define NINKASI_FUNCTION_H

#include "nkvalue.h"
#include "nkx.h"
#include "nkfuncid.h"

// ----------------------------------------------------------------------
// Types

/// Internal function record.
struct NKVMFunction
{
    nkuint32_t argumentCount;
    nkuint32_t firstInstructionIndex;

    /// ID of the C function callback to use.
    NKVMExternalFunctionID externalFunctionId;
};

/// Native C function record.
struct NKVMExternalFunction
{
    /// name field used to reconnect native C functions during
    /// deserialization, where the function pointer might not line up
    /// to the same function pointer in a different binary. Not that
    /// we'd want to trust a function pointer from a binary anyway.
    char *name;

    /// The C function pointer itself.
    NKVMFunctionCallback CFunctionCallback;

    /// This is the ID of an internal function that has this as its
    /// externalFunctionId. Note that there may be more than one
    /// NKVMFunction for a single NKVMExternalFunction, and this only
    /// points to ONE of those. It may also be NK_INVALID_VALUE if
    /// there are no known internal functions that reference this.
    NKVMInternalFunctionID internalFunctionId;
};

// ----------------------------------------------------------------------
// External function interface

/// Register a new external function. You should do this before
/// compiling or deserializing. It may also take a long time searching
/// for duplicates. You may have to use this if you do not know if a
/// function exists or not inside the VM yet, or if you do know and
/// need the ID to be found.
NKVMExternalFunctionID nkiVmRegisterExternalFunction(
    struct NKVM *vm,
    const char *name,
    NKVMFunctionCallback func);

/// Register a new external function. You should do this before
/// compiling or deserializing. This version will not waste time
/// searching for an existing copy of the function object. Use only if
/// you know that a duplicate of the function does not exist yet.
NKVMExternalFunctionID nkiVmRegisterExternalFunctionNoSearch(
    struct NKVM *vm,
    const char *name,
    NKVMFunctionCallback func);

/// Look up or create an internal function to represent some external
/// function. This should execute fast (no searching), but may have to
/// instantiate a new function object.
NKVMInternalFunctionID nkiVmGetOrCreateInternalFunctionForExternalFunction(
    struct NKVM *vm, NKVMExternalFunctionID externalFunctionId);

// ----------------------------------------------------------------------
// Compiler helpers

/// Compiler internal function creation. Don't use this outside. Not
/// for that. This ONLY allocates a function.
struct NKVMFunction *nkiVmCreateFunction(
    struct NKVM *vm, NKVMInternalFunctionID *functionId);

#endif // NINKASI_FUNCTION_H


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

#ifndef NINKASI_VALUE_H
#define NINKASI_VALUE_H

#include "nkenums.h"
#include "nkfuncid.h"

struct NKVM;

/// Basic "value" type. This can represent an integer, float, pointer
/// to an object, pointer to a string, or function ID. It does no
/// reference counting, and it is safe to instantiate outside of the
/// VM, or copy the structure in its entirety from a VM-owned
/// instance.
///
/// (Note: The garbage collector will not see NKValues that are not
/// owned by the VM. Use nkxVmObjectAcquireHandle() to keep the VM
/// from garbage collecting objects that are potentially only tracked
/// outside of the VM.)
struct NKValue
{
    // The type_uint field here forces the field to be 32-bits on DOS,
    // where the compiler I'm using still uses 16-bit values for
    // enums.
    union {
        enum NKValueType type;
        nkuint32_t type_uint;
    };

    union
    {
        nkint32_t intData;
        float floatData;
        nkuint32_t stringTableEntry;
        NKVMInternalFunctionID functionId;
        nkuint32_t objectId;

        // Used for ints, floats, functionIds, and objectIds. NOT for
        // strings. String hashes are stored in the corresponding
        // entry in the string table.
        nkuint32_t basicHashValue;
    };
};

/// Dump a value to stdout for debugging purposes.
nkbool nkiValueDump(struct NKVM *vm, struct NKValue *value);

/// Get a string representation for a type.
const char *nkiValueTypeGetName(enum NKValueType type);

/// Convert a value to an integer.
nkint32_t nkiValueToInt(struct NKVM *vm, struct NKValue *value);

/// Convert a value to a floating point number.
float nkiValueToFloat(struct NKVM *vm, struct NKValue *value);

/// Returns a string for a value, possibly converting internally.
/// Values are only guaranteed to be valid until the next garbage
/// collection pass.
const char *nkiValueToString(struct NKVM *vm, struct NKValue *value);

/// The return of this value is like strcmp(). -1 for less, 0 for
/// equal, 1 for greater-than. Set strictType to nktrue to force a
/// comparison failure when types differ. You MUST do this for things
/// like binary trees to ensure things have a consistent order.
nkint32_t nkiValueCompare(
    struct NKVM *vm,
    struct NKValue *in1,
    struct NKValue *in2,
    nkbool strictType);

/// Get a (NOT CRYPTOGRAPHICALLY SECURE) hash value for a value.
nkuint32_t nkiValueHash(struct NKVM *vm, struct NKValue *value);

/// Write an integer into an NKValue.
void nkiValueSetInt(struct NKVM *vm, struct NKValue *value, nkint32_t intData);

/// Write a float into an NKValue.
void nkiValueSetFloat(struct NKVM *vm, struct NKValue *value, float floatData);

/// Write a string into an NKValue. This actually finds or creates a
/// string table entry for the given string, then assigns the ID of
/// that entry to the value.
void nkiValueSetString(struct NKVM *vm, struct NKValue *value, const char *str);

#endif // NINKASI_VALUE_H


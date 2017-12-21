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

#ifndef NINKASI_OBJECTS_H
#define NINKASI_OBJECTS_H

#include "nktable.h"

/// Dumb linked-list for key/value pairs inside of an object.
struct NKVMObjectElement
{
    struct NKValue key;
    struct NKValue value;
    struct NKVMObjectElement *next;
};

#define nkiVMObjectHashBucketCount 16

struct NKVMObject
{
    nkuint32_t objectTableIndex;
    nkuint32_t lastGCPass;

    // Cached count of number of entries.
    nkuint32_t size;

    struct NKVMObjectElement *hashBuckets[nkiVMObjectHashBucketCount];

    // External handle stuff.
    struct NKVMObject *nextObjectWithExternalHandles;
    struct NKVMObject **previousExternalHandleListPtr;
    nkuint32_t externalHandleCount;

    // This has an internal function ID type, because it must actually
    // be callable by VM code, but only external functions are valid
    // here. Putting the wrong type here will throw an error when it
    // is actually called. Also, it's not a function pointer because
    // we can actually serialize these things.
    NKVMInternalFunctionID gcCallback;

    // Same specifics as gcCallback.
    NKVMInternalFunctionID serializationCallback;

    NKVMExternalDataTypeID externalDataType;
    void *externalData;
};

void nkiVmObjectTableInit(struct NKVM *vm);
void nkiVmObjectTableDestroy(struct NKVM *vm);

struct NKVMObject *nkiVmObjectTableGetEntryById(
    struct NKVMTable *table,
    nkuint32_t index);

nkuint32_t nkiVmObjectTableCreateObject(
    struct NKVM *vm);

void nkiVmObjectTableCleanOldObjects(
    struct NKVM *vm,
    nkuint32_t lastGCPass);

// FIXME: Make a public version of this that takes Value* instead of
// VMObject*.
struct NKValue *nkiVmObjectFindOrAddEntry(
    struct NKVM *vm,
    struct NKVMObject *ob,
    struct NKValue *key,
    nkbool noAdd);

// FIXME: Make a public version of this that takes Value* instead of
// VMObject*.
void nkiVmObjectClearEntry(
    struct NKVM *vm,
    struct NKVMObject *ob,
    struct NKValue *key);

// Internal version of nkxVmObjectAcquireHandle.
void nkiVmObjectAcquireHandle(struct NKVM *vm, struct NKValue *value);

// Internal version of nkxVmObjectReleaseHandle.
void nkiVmObjectReleaseHandle(struct NKVM *vm, struct NKValue *value);

void nkiVmObjectSetGarbageCollectionCallback(
    struct NKVM *vm,
    struct NKValue *object,
    NKVMExternalFunctionID callbackFunction);

void nkiVmObjectSetSerializationCallback(
    struct NKVM *vm,
    struct NKValue *object,
    NKVMExternalFunctionID callbackFunction);

void nkiVmObjectSetExternalType(
    struct NKVM *vm,
    struct NKValue *object,
    NKVMExternalDataTypeID externalType);

NKVMExternalDataTypeID nkiVmObjectGetExternalType(
    struct NKVM *vm,
    struct NKValue *object);

void nkiVmObjectSetExternalData(
    struct NKVM *vm,
    struct NKValue *object,
    void *data);

void *nkiVmObjectGetExternalData(
    struct NKVM *vm,
    struct NKValue *object);

// VM-teardown function. Does not create table holes.
void nkiVmObjectTableCleanAllObjects(
    struct NKVM *vm);

#endif // NINKASI_OBJECTS_H

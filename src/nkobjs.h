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

#ifndef NINKASI_OBJECTS_H
#define NINKASI_OBJECTS_H

#include "nkfuncid.h"
#include "nktable.h"
#include "nkvalue.h"

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

    NKVMExternalDataTypeID externalDataType;
    void *externalData;
};

void nkiVmObjectTableInit(struct NKVM *vm);
void nkiVmObjectTableDestroy(struct NKVM *vm);

struct NKVMObject *nkiVmGetObjectFromValue(
    struct NKVM *vm,
    struct NKValue *value);

struct NKVMObject *nkiVmObjectTableGetEntryById(
    struct NKVMTable *table,
    nkuint32_t index);

nkuint32_t nkiVmObjectTableCreateObject(
    struct NKVM *vm);

void nkiVmObjectTableCleanOldObjects(
    struct NKVM *vm,
    nkuint32_t lastGCPass);

struct NKValue *nkiVmObjectFindOrAddEntry(
    struct NKVM *vm,
    struct NKVMObject *ob,
    struct NKValue *key,
    nkbool noAdd);

nkuint32_t nkiVmObjectGetSize(
    struct NKVM *vm,
    struct NKValue *objectId);

void nkiVmObjectClearEntry(
    struct NKVM *vm,
    struct NKVMObject *ob,
    struct NKValue *key);

// Internal version of nkxVmObjectAcquireHandle.
void nkiVmObjectAcquireHandle(struct NKVM *vm, struct NKValue *value);

// Internal version of nkxVmObjectReleaseHandle.
void nkiVmObjectReleaseHandle(struct NKVM *vm, struct NKValue *value);

nkuint32_t nkiVmObjectGetExternalHandleCount(struct NKVM *vm, struct NKValue *value);

nkbool nkiVmObjectSetExternalType(
    struct NKVM *vm,
    struct NKValue *object,
    NKVMExternalDataTypeID externalType);

NKVMExternalDataTypeID nkiVmObjectGetExternalType(
    struct NKVM *vm,
    struct NKValue *object);

nkbool nkiVmObjectSetExternalData(
    struct NKVM *vm,
    struct NKValue *object,
    void *data);

void *nkiVmObjectGetExternalData(
    struct NKVM *vm,
    struct NKValue *object);

nkbool nkiVmObjectClearExternalDataAndType(
    struct NKVM *vm,
    struct NKValue *object);

// VM-teardown function. Does not create table holes.
void nkiVmObjectTableCleanAllObjects(
    struct NKVM *vm);

// Wrapper around nkiVmObjectClearEntry() that gets the object itself
// based on the objectId value.
void nkiVmObjectClearEntry_public(
    struct NKVM *vm,
    struct NKValue *objectId,
    struct NKValue *key);

// Wrapper around nkiVmObjectFindOrAddEntry() that gets the object
// itself based on the objectId value.
struct NKValue *nkiVmObjectFindOrAddEntry_public(
    struct NKVM *vm,
    struct NKValue *objectId,
    struct NKValue *key,
    nkbool noAdd);

// TODO: Move this to a GC related header.
void nkiVmGarbageCollect_markValue(
    struct NKVMGCState *gcState,
    struct NKValue *v);

#endif // NINKASI_OBJECTS_H

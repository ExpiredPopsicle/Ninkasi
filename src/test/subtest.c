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

// This file demonstrates how to make a simple library for Ninkasi.
//
// The library includes one external data type (Widgets), with some
// external data associated with them. It supports serialization,
// deserialization, and garbage collection of Widgets.
//
// There is also a chunk of data associated with the library that is
// inaccessible to the script itself, but will also be serialized and
// deserialzed with the VM state.
//
// Finally, it demonstrates how to set up function calls to C
// functions.

#include "subtest.h"

#include "../nkx.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

// ----------------------------------------------------------------------
// Malloc wrapper and memory leak tracking.

// This is just for tracking memory leaks in the test application. We
// need to ensure that VMs properly clean up external data, even in
// cases of catastrophic failures. This is just a system to track all
// allocations and, if there are any still active when the program
// ends, fire off an assert. AFL will be able to probe out these
// cases.
//
// This system is very not-thread-safe.

// Header for every allocation.
struct SubsystemTest_MallocHeader
{
    // So we know where a thing was allocated, we can include a
    // description. Every allocation has its own instance of this,
    // which means that every allocation call is actually two
    // allocations (malloc + strdup).
    char *description;

    // Allocation size, not including header and description.
    nkuint32_t size;

    // Doubly linked list of all allocations.
    struct SubsystemTest_MallocHeader *nextBlock;
    struct SubsystemTest_MallocHeader **prevPtr;
};

// Global list of allocations.
static struct SubsystemTest_MallocHeader *subsystemTest_allocationList = NULL;

// Count of all allocations.
static nkuint32_t subsystemTest_debugOnly_dataCount = 0;

// malloc() replacement.
void *subsystemTest_mallocWrapper(nkuint32_t size, const char *description)
{
    struct SubsystemTest_MallocHeader *header = NULL;

    if(size == 0) {
        return NULL;
    }

    header = (struct SubsystemTest_MallocHeader *)malloc(
        size + sizeof(struct SubsystemTest_MallocHeader));

    if(!header) {
        return NULL;
    }

    header->description = strdup(description);
    subsystemTest_debugOnly_dataCount++;

    // Add to allocation list.
    header->nextBlock = subsystemTest_allocationList;
     if(header->nextBlock) {
        header->nextBlock->prevPtr = &header->nextBlock;
    }
    header->prevPtr = &subsystemTest_allocationList;
    subsystemTest_allocationList = header;

    return header + 1;
}

// strdup() replacement.
char *subsystemTest_strdupWrapper(const char *str, const char *description)
{
    if(str) {
        char *data = (char* )subsystemTest_mallocWrapper(
            strlen(str) + 1, description);
        if(data) {
            memcpy(data, str, strlen(str) + 1);
            return data;
        }
    }
    return NULL;
}

// free() replacement.
void subsystemTest_freeWrapper(void *data)
{
    if(data) {
        struct SubsystemTest_MallocHeader *header =
            (struct SubsystemTest_MallocHeader *)data - 1;

        // free(NULL) should just be a no-op.
        if(!data) {
            return;
        }

        // Remove this from the global allocation list.
        if(header->nextBlock) {
            header->nextBlock->prevPtr = header->prevPtr;
        }
        *header->prevPtr = header->nextBlock;

        free(header->description);
        free(header);

        subsystemTest_debugOnly_dataCount--;
    }
}

// This function exists only to check that every piece of data we've
// created has been cleaned up, meaning that we're still able to clean
// up our data in case the VM has a catastrophic (allocation) error.
// This is only for testing. Use of atexit() to register this kind of
// callback is not endorsed.
void subsystemTest_debugOnly_exitCheck(void)
{
    struct SubsystemTest_MallocHeader *header = subsystemTest_allocationList;

    // Dump all still-allocated blocks.
    while(header) {
        fprintf(
            stderr,
            "Block still allocated: %p: %s\n",
            header + 1,
            header->description);
        header = header->nextBlock;
    }

    if(subsystemTest_debugOnly_dataCount) {
        fprintf(
            stderr,
            "Debug data count: " NK_PRINTF_UINT32 "\n",
            subsystemTest_debugOnly_dataCount);
    }

    assert(subsystemTest_debugOnly_dataCount == 0);
    assert(!subsystemTest_allocationList);
}

void subsystemTest_registerExitCheck(void)
{
    static nkbool registered = nkfalse;
    if(!registered) {
        registered = nktrue;
        atexit(subsystemTest_debugOnly_exitCheck);
    }
}

// ----------------------------------------------------------------------
// SubsystemTest types

// This is the global subsystem data that lives on the VM, but is not
// accessible to the script itself.
struct SubsystemTest_InternalData
{
    // This is the type ID we get back when we create wth Widget type
    // in the VM. We'll need it to create new Widgets and also to type
    // check parameters coming into function callbacks.
    NKVMExternalDataTypeID widgetTypeId;

    // This is a function that we aren't going to expose at the global
    // scope. Instead we're going to make it a method on Widget
    // objects that we create.
    NKVMInternalFunctionID objectSelfCallTestId;

    // These are some arbitrary bits of data to demonstrate generic
    // stuff getting stored in an external subsystem.
    char *testString;
    nkuint32_t widgetCount;
};

// This is the data we attach to our object type.
struct SubsystemTest_WidgetData
{
    // This is an arbitrary piece of data to demonstrate data getting
    // attached to an object in a VM.
    nkuint32_t data;
};

// ----------------------------------------------------------------------
// Widget-related functions

void subsystemTest_widgetCreate(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData;
    struct SubsystemTest_WidgetData *newData;

    internalData =
        (struct SubsystemTest_InternalData *)nkxGetExternalSubsystemDataOrError(
            data->vm, "subsystemTest");

    if(internalData) {

        nkxCreateObject(data->vm, &data->returnValue);

        // Check for errors after the heap-allocating call to
        // nkxCreateObject. If an error occurred, then it's not safe to do
        // our own allocation and assign it to the object, because it's
        // going to leak. (There will be no object to assign the data to!)
        if(!nkxVmHasErrors(data->vm)) {

            newData =
                (struct SubsystemTest_WidgetData *)subsystemTest_mallocWrapper(
                    sizeof(struct SubsystemTest_WidgetData),
                    "subsystemTest_widgetCreate");

            // Totally arbitrary value for testing purposes.
            if(newData) {
                newData->data = 5678;
            } else {
                nkxAddError(data->vm, "malloc() failed in subsystemTest_widgetCreate.");
            }

            // Set the type to our widget type ID we created in the init
            // function, set the external data to the newly allocated
            // data. None of these calls can fail due to allocation
            // failure, because none of them allocate memory, so we don't
            // have to worry about leaking the new data anymore.
            nkxVmObjectSetExternalData(
                data->vm, &data->returnValue, newData);
            nkxVmObjectSetExternalType(
                data->vm, &data->returnValue, internalData->widgetTypeId);

            {
                struct NKValue fieldTestKey;
                struct NKValue *fieldTest = NULL;

                nkxValueSetString(data->vm, &fieldTestKey, "testField");
                fieldTest = nkxVmObjectFindOrAddEntry(
                    data->vm, &data->returnValue,
                    &fieldTestKey, nkfalse);
                if(fieldTest) {
                    nkxValueSetString(data->vm, fieldTest, "This is some data on an object.");
                }

                nkxValueSetString(data->vm, &fieldTestKey, "testMethod");
                fieldTest = nkxVmObjectFindOrAddEntry(
                    data->vm, &data->returnValue,
                    &fieldTestKey, nkfalse);
                if(fieldTest) {
                    nkxValueSetFunction(data->vm, fieldTest, internalData->objectSelfCallTestId);
                }
            }

            internalData->widgetCount++;
            printf("Widget count now: " NK_PRINTF_UINT32 "\n", internalData->widgetCount);

        }
    }
}

void subsystemTest_widgetSetData(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData;
    struct SubsystemTest_WidgetData *widgetData;

    internalData =
        (struct SubsystemTest_InternalData *)nkxGetExternalSubsystemDataOrError(
            data->vm, "subsystemTest");

    if(internalData) {

        widgetData =
            (struct SubsystemTest_WidgetData *)nkxFunctionCallbackGetExternalDataArgument(
                data, "subsystemTest_widgetSetData", 0, internalData->widgetTypeId);

        if(widgetData) {
            widgetData->data = nkxValueToInt(data->vm, &data->arguments[1]);
        }
    }
}

void subsystemTest_widgetGetData(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData;
    struct SubsystemTest_WidgetData *widgetData;

    internalData =
        (struct SubsystemTest_InternalData *)nkxGetExternalSubsystemDataOrError(
            data->vm, "subsystemTest");

    if(internalData) {

        widgetData =
            (struct SubsystemTest_WidgetData *)nkxFunctionCallbackGetExternalDataArgument(
                data, "subsystemTest_widgetGetData", 0, internalData->widgetTypeId);

        if(widgetData) {
            nkxValueSetInt(data->vm, &data->returnValue, widgetData->data);
        }
    }
}

void subsystemTest_widgetSerializeData(
    struct NKVM *vm,
    struct NKValue *objectValue,
    void *data)
{
    struct SubsystemTest_InternalData *internalData;
    struct SubsystemTest_WidgetData *widgetData =
        (struct SubsystemTest_WidgetData *)data;

    internalData =
        (struct SubsystemTest_InternalData *)nkxGetExternalSubsystemDataOrError(
            vm, "subsystemTest");

    if(!internalData) {
        return;
    }

    // TODO: Explain read/write modes here.

    if(!widgetData) {

        widgetData =
            (struct SubsystemTest_WidgetData *)subsystemTest_mallocWrapper(
                sizeof(*widgetData),
                "subsystemTest_widgetSerializeData");

        if(!widgetData) {
            nkxAddError(vm, "Malloc failed in subsystemTest_widgetSerializeData.");
        }

        widgetData->data = 0;
        nkxVmObjectSetExternalData(vm, objectValue, widgetData);

        // No more type remapping. Ensure that all garbage collection
        // and type data is now consistent.
        nkxVmObjectSetExternalType(
            vm, objectValue, internalData->widgetTypeId);
    }

    nkxSerializeData(vm, widgetData, sizeof(*widgetData));
}

void subsystemTest_widgetGCData(struct NKVM *vm, struct NKValue *val, void *data)
{
    struct SubsystemTest_InternalData *internalData;
    struct SubsystemTest_WidgetData *widgetData =
        (struct SubsystemTest_WidgetData *)data;

    // NOTE: Do not use nkxGetExternalSubsystemDataOrError here! That
    // can cause an allocation, and this could run in allocation fail
    // cleanup mode!
    internalData =
        (struct SubsystemTest_InternalData *)nkxGetExternalSubsystemData(
            vm, "subsystemTest");

    // This is a GC callback, so it can run in cases where maybe the
    // VM or subsystem data weren't entirely set up, so we have to be
    // careful about potential null dereferences.
    if(internalData) {
        if(widgetData) {
            subsystemTest_freeWrapper(widgetData);
            internalData->widgetCount--;
            printf("Widget count now: " NK_PRINTF_UINT32 "\n", internalData->widgetCount);
        }
    }
}

// ----------------------------------------------------------------------
// Functions callable from script code

// These functions all have a lot of common boilerplate to ensure that
// the internal data exists. Normally I would abstract that all away
// with a #define or something, but this is example code so it's
// better to leave it for the sake of demonstration.

void subsystemTest_setTestString(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData;

    internalData =
        (struct SubsystemTest_InternalData *)nkxGetExternalSubsystemDataOrError(
            data->vm, "subsystemTest");

    if(internalData) {
        if(internalData->testString) {
            subsystemTest_freeWrapper(internalData->testString);
            internalData->testString = NULL;
        }
        internalData->testString = subsystemTest_strdupWrapper(
            nkxValueToString(data->vm, &data->arguments[0]),
            "subsystemTest_setTestString");
    }
}

void subsystemTest_getTestString(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData;

    internalData =
        (struct SubsystemTest_InternalData *)nkxGetExternalSubsystemDataOrError(
            data->vm, "subsystemTest");

    if(internalData) {
        nkxValueSetString(
            data->vm, &data->returnValue,
            internalData->testString);
    }
}

void subsystemTest_printTestString(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData;

    internalData =
        (struct SubsystemTest_InternalData *)nkxGetExternalSubsystemDataOrError(
            data->vm, "subsystemTest");

    if(internalData) {
        printf("subsystemTest_print: %s\n", internalData->testString);
    }
}

void subsystemTest_widget_selfCallTest(struct NKVMFunctionCallbackData *data)
{
    printf("Widget self-call test...\n");
    printf("  Argument count: " NK_PRINTF_UINT32 "\n", data->argumentCount);

    {
        struct NKValue testDataId;
        struct NKValue *testData;
        nkxValueSetString(data->vm, &testDataId, "testField");
        testData = nkxVmObjectFindOrAddEntry(
            data->vm, &data->arguments[0], &testDataId, nktrue);
        if(testData) {
            printf("  Test data: %s\n", nkxValueToString(data->vm, testData));
        }
    }
}

// ----------------------------------------------------------------------
// Cleanup, init, serialization, etc

void subsystemTest_cleanup(struct NKVM *vm, void *internalData)
{
    struct SubsystemTest_InternalData *systemData =
        (struct SubsystemTest_InternalData*)internalData;

    printf("subsystemTest_cleanup\n");

    if(systemData) {

        struct NKValue ob;
        nkuint32_t index = 0;
        while(nkxGetNextObjectOfExternalType(vm, systemData->widgetTypeId, &ob, &index)) {

            // Note: nkxVmObjectGetExternalData() is one of the few
            // nkx functions that can still operate when the
            // out-of-memory error flag is set.
            void *objectExternalData = nkxVmObjectGetExternalData(vm, &ob);
            printf("Object found for cleanup: " NK_PRINTF_UINT32 " - " NK_PRINTF_UINT32 "\n",
                ob.objectId, index);
            subsystemTest_freeWrapper(objectExternalData);

            systemData->widgetCount--;
            printf("(Cleanup) Widget count now: " NK_PRINTF_UINT32 "\n", systemData->widgetCount);
        }

        // Free all of our internal data that's not attached to single
        // objects.
        subsystemTest_freeWrapper(systemData->testString);
        subsystemTest_freeWrapper(systemData);

    } else {

        printf("systemData not found!\n");

    }

    // Note: Do NOT assert here on !subsystemTest_debugOnly_dataCount.
    // We may have multiple VM instances.

}

void subsystemTest_serialize(struct NKVM *vm, void *internalData)
{
    struct SubsystemTest_InternalData *systemData =
        (struct SubsystemTest_InternalData*)internalData;

    if(systemData) {

        nkuint32_t len = systemData->testString ? strlen(systemData->testString) : 0;
        nkxSerializeData(vm, &len, sizeof(len));

        if(!nkxSerializerGetWriteMode(vm)) {

            // We're in read mode, so let's nuke whatever data we have
            // here already to make room for the data that's about to
            // come in.

            if(systemData->testString) {
                subsystemTest_freeWrapper(systemData->testString);
            }
            if(len) {
                systemData->testString =
                    (char *)subsystemTest_mallocWrapper(
                        1 + len,
                        nkxSerializerGetWriteMode(vm) ?
                        "subsystemTest_serialize (write)" :
                        "subsystemTest_serialize (read)");
                if(!systemData->testString) {
                    return;
                }
            } else {
                systemData->testString = NULL;
            }
        }

        if(systemData->testString) {

            // nkxSerializeData() will either read the string data in,
            // or write the data out depening on the mode. But we
            // don't have to do anything else special here depending
            // on the read/write mode.

            if(nkxSerializeData(vm, systemData->testString, len)) {
                systemData->testString[len] = 0;
            } else {
                systemData->testString[0] = 0;
            }
        }

        // DO NOT TRUST TYPE IDS FROM BINARY FILES. Trust what was set
        // up on VM initialization!
        {
            NKVMExternalDataTypeID tmpId = systemData->widgetTypeId;
            nkxSerializeData(
                vm, &tmpId,
                sizeof(tmpId));
        }

        nkxSerializeData(
            vm, &systemData->objectSelfCallTestId,
            sizeof(systemData->objectSelfCallTestId));

        // Save/load widget count. Note: Don't do anything important
        // based on this. We don't trust data coming from the binary!
        nkxSerializeData(vm, &systemData->widgetCount, sizeof(nkuint32_t));
    }
}

void subsystemTest_widgetGCMark(
    struct NKVM *vm, struct NKValue *value,
    void *internalData, struct NKVMGCState *gcState)
{
    // FIXME: Screwing up our diffs.
    //
    // printf("Widget marked by GC: %p\n", internalData);
    printf("Widget marked by GC\n");

    // FIXME: This is redundant, because it's marking itself. This is
    // an example of a call to nkxVmGarbageCollect_markValue, which
    // should be used on data associated with this object that lives
    // inside the VM, but where all those associations are external to
    // VM data.
    nkxVmGarbageCollect_markValue(vm, gcState, value);
}

void subsystemTest_initLibrary(struct NKVM *vm, struct NKCompilerState *cs)
{
    struct SubsystemTest_InternalData *internalData = NULL;

    // FIXME: Screwing up our diffs.
    //
    // printf("subsystemTest: Initializing on VM: %p\n", vm);
    printf("subsystemTest: Initializing on VM\n");

    subsystemTest_registerExitCheck();

    internalData =
        (struct SubsystemTest_InternalData *)subsystemTest_mallocWrapper(
            sizeof(struct SubsystemTest_InternalData),
            "subsystemTest_initLibrary");

    // Initialize some data, just as an example of how we'd normally
    // store data on an external data structure.
    internalData->testString = NULL;
    internalData->widgetCount = 0;

    // Initialize the subsystem inside the VM.
    if(!nkxInitSubsystem(
        vm, cs, "subsystemTest",
        internalData,
        subsystemTest_cleanup,
        subsystemTest_serialize))
    {
        subsystemTest_freeWrapper(internalData);
        internalData = NULL;
        return;
    }

    // Register a custom type.
    internalData->widgetTypeId = nkxVmRegisterExternalType(
        vm, "subsystemTest_widget",
        subsystemTest_widgetSerializeData,
        subsystemTest_widgetGCData,
        subsystemTest_widgetGCMark);

    nkxVmSetupExternalFunction(vm, cs, "subsystemTest_setTestString", subsystemTest_setTestString,
        nktrue,
        1,
        NK_VALUETYPE_STRING);

    nkxVmSetupExternalFunction(
        vm, cs, "subsystemTest_getTestString",
        subsystemTest_getTestString,
        nktrue,
        0);

    nkxVmSetupExternalFunction(
        vm, cs, "subsystemTest_printTestString",
        subsystemTest_printTestString,
        nktrue,
        0);

    nkxVmSetupExternalFunction(
        vm, cs, "subsystemTest_widgetCreate",
        subsystemTest_widgetCreate,
        nktrue,
        0);

    nkxVmSetupExternalFunction(
        vm, cs, "subsystemTest_widgetSetData",
        subsystemTest_widgetSetData,
        nktrue,
        2,
        NK_VALUETYPE_OBJECTID, internalData->widgetTypeId,
        NK_VALUETYPE_NIL);

    nkxVmSetupExternalFunction(
        vm, cs, "subsystemTest_widgetGetData",
        subsystemTest_widgetGetData,
        nktrue,
        1,
        NK_VALUETYPE_OBJECTID, internalData->widgetTypeId);

    // Set up an object method.
    {
        NKVMExternalFunctionID extId;
        NKVMInternalFunctionID intId;
        extId = nkxVmSetupExternalFunction(
            vm, cs, "subsystemTest_widget_selfCallTest",
            subsystemTest_widget_selfCallTest,
            nkfalse, // Not global.
            1,
            NK_VALUETYPE_OBJECTID, internalData->widgetTypeId);

        // Important thing to note here: The internal function ID
        // we're setting up here will get overwritten on
        // deserialization, in subsystemTest_serialize(). But we want
        // that to happen. TL;DR: This part is optional if we're not
        // compiling.
        intId = nkxVmGetOrCreateInternalFunctionForExternalFunction(
            vm, extId);
        internalData->objectSelfCallTestId = intId;
    }
}



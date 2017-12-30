#include "subtest.h"

#include "nkx.h"

// FIXME: Remove this after we write the serialization functions.
#include "nkvm.h"

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
    struct SubsystemTest_MallocHeader *header =
        malloc(size + sizeof(struct SubsystemTest_MallocHeader));
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
        char *data = subsystemTest_mallocWrapper(
            strlen(str) + 1, description);
        memcpy(data, str, strlen(str) + 1);
        return data;
    }
    return NULL;
}

// free() replacement.
void subsystemTest_freeWrapper(void *data)
{
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

struct SubsystemTest_InternalData
{
    NKVMExternalDataTypeID widgetTypeId;
    NKVMExternalFunctionID widgetGCCallbackId;
    NKVMExternalFunctionID widgetSerializeCallbackId;

    char *testString;
};

struct SubsystemTest_WidgetData
{
    nkuint32_t data;
};

// ----------------------------------------------------------------------
// Widget-related functions

void subsystemTest_widgetCreate(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData;
    struct SubsystemTest_WidgetData *newData;

    nkxFunctionCallbackCheckArguments(
        data, "subsystemTest_widgetCreate", 0);

    internalData = nkxGetExternalSubsystemDataOrError(
        data->vm, "subsystemTest");

    if(nkxVmHasErrors(data->vm)) {
        return;
    }

    nkxCreateObject(data->vm, &data->returnValue);

    // Check for errors after the heap-allocating call to
    // nkxCreateObject. If an error occurred, then it's not safe to do
    // our own allocation and assign it to the object, because it's
    // going to leak.
    if(!nkxVmHasErrors(data->vm)) {

        newData = subsystemTest_mallocWrapper(
            sizeof(struct SubsystemTest_WidgetData),
            "subsystemTest_widgetCreate");

        // Totally arbitrary value for testing purposes.
        newData->data = 5678;

        // Set the type to our widget type ID we created in the init
        // function, set the external data to the newly allocated
        // data, set the garbage collection callback, and set the
        // serialization callback. None of these calls can fail due to
        // allocation failure, because none of them allocate memory,
        // so we don't have to worry about leaking the new data
        // anymore.
        nkxVmObjectSetExternalType(
            data->vm, &data->returnValue, internalData->widgetTypeId);
        nkxVmObjectSetExternalData(
            data->vm, &data->returnValue, newData);
        nkxVmObjectSetGarbageCollectionCallback(
            data->vm, &data->returnValue, internalData->widgetGCCallbackId);
        nkxVmObjectSetSerializationCallback(
            data->vm, &data->returnValue, internalData->widgetSerializeCallbackId);
    }
}

void subsystemTest_widgetSetData(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData;
    struct SubsystemTest_WidgetData *widgetData;

    nkxFunctionCallbackCheckArguments(
        data, "subsystemTest_widgetSetData", 2,
        NK_VALUETYPE_OBJECTID,
        NK_VALUETYPE_INT);

    internalData = nkxGetExternalSubsystemDataOrError(
        data->vm, "subsystemTest");

    widgetData = nkxFunctionCallbackGetExternalDataArgument(
        data, "subsystemTest_widgetSetData", 0, internalData->widgetTypeId);

    if(nkxVmHasErrors(data->vm)) {
        return;
    }

    widgetData->data = nkxValueToInt(data->vm, &data->arguments[1]);
}

void subsystemTest_widgetGetData(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData;
    struct SubsystemTest_WidgetData *widgetData;

    nkxFunctionCallbackCheckArguments(
        data, "subsystemTest_widgetGetData", 1,
        NK_VALUETYPE_OBJECTID);

    internalData = nkxGetExternalSubsystemDataOrError(
        data->vm, "subsystemTest");

    widgetData = nkxFunctionCallbackGetExternalDataArgument(
        data, "subsystemTest_widgetGetData", 0, internalData->widgetTypeId);

    if(nkxVmHasErrors(data->vm)) {
        return;
    }

    nkxValueSetInt(data->vm, &data->returnValue, widgetData->data);
}

void subsystemTest_widgetSerializeData(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData;
    struct SubsystemTest_WidgetData *widgetData;

    nkxFunctionCallbackCheckArguments(
        data, "subsystemTest_widgetSerializeData", 1,
        NK_VALUETYPE_OBJECTID);

    internalData = nkxGetExternalSubsystemDataOrError(
        data->vm, "subsystemTest");

    widgetData = nkxFunctionCallbackGetExternalDataArgument(
        data, "subsystemTest_widgetSerializeData", 0, internalData->widgetTypeId);

    if(nkxVmHasErrors(data->vm)) {
        return;
    }

    if(!widgetData) {

        widgetData = subsystemTest_mallocWrapper(
            sizeof(*widgetData),
            "subsystemTest_widgetSerializeData");

        widgetData->data = 0;
        nkxVmObjectSetExternalData(data->vm, &data->arguments[0], widgetData);
    }

    nkxSerializeData(data->vm, widgetData, sizeof(*widgetData));
}

void subsystemTest_widgetGCData(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData;
    struct SubsystemTest_WidgetData *widgetData;

    nkxFunctionCallbackCheckArguments(
        data, "subsystemTest_widgetGCData", 1,
        NK_VALUETYPE_OBJECTID);

    internalData = nkxGetExternalSubsystemDataOrError(
        data->vm, "subsystemTest");

    // This is a GC callback, so it can run in cases where maybe the
    // VM or subsystem data weren't entirely set up, so we have to be
    // careful about potential null dereferences.
    if(internalData) {

        widgetData = nkxFunctionCallbackGetExternalDataArgument(
            data, "subsystemTest_widgetGCData", 0, internalData->widgetTypeId);

        if(widgetData) {
            subsystemTest_freeWrapper(widgetData);
            nkxVmObjectSetExternalData(data->vm, &data->arguments[0], NULL);
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

    internalData = nkxGetExternalSubsystemDataOrError(
        data->vm, "subsystemTest");

    nkxFunctionCallbackCheckArguments(
        data, "subsystemTest_setTestString", 1,
        NK_VALUETYPE_STRING);

    if(nkxVmHasErrors(data->vm)) {
        return;
    }

    if(internalData->testString) {
        subsystemTest_freeWrapper(internalData->testString);
        internalData->testString = NULL;
    }

    internalData->testString = subsystemTest_strdupWrapper(
        nkxValueToString(data->vm, &data->arguments[0]),
        "subsystemTest_setTestString");
}

void subsystemTest_getTestString(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData;

    internalData = nkxGetExternalSubsystemDataOrError(
        data->vm, "subsystemTest");

    nkxFunctionCallbackCheckArguments(
        data, "subsystemTest_getTestString", 0);

    if(nkxVmHasErrors(data->vm)) {
        return;
    }

    nkxValueSetString(
        data->vm, &data->returnValue,
        internalData->testString);
}

void subsystemTest_printTestString(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData;

    internalData = nkxGetExternalSubsystemDataOrError(
        data->vm, "subsystemTest");

    nkxFunctionCallbackCheckArguments(
        data, "subsystemTest_printTestString", 0);

    if(nkxVmHasErrors(data->vm)) {
        return;
    }

    printf("subsystemTest_print: %s\n", internalData->testString);
}

// ----------------------------------------------------------------------
// Cleanup, init, serialization, etc

nkbool nkxGetNextObjectOfExternalType(
    struct NKVM *vm,
    struct NKVMExternalDataTypeID type,
    struct NKValue *outValue,
    nkuint32_t *startIndex)
{
    outValue->type = NK_VALUETYPE_OBJECTID;
    outValue->objectId = NK_INVALID_VALUE;

    // Incomplete VM setup?
    if(!vm->objectTable.objectTable) {
        return nkfalse;
    }

    while(*startIndex < vm->objectTable.capacity) {

        struct NKVMObject *ob = vm->objectTable.objectTable[*startIndex];

        if(ob) {
            if(ob->externalDataType.id == type.id) {
                outValue->objectId = *startIndex;
            }
        }

        (*startIndex)++;

        if(outValue->objectId != NK_INVALID_VALUE) {
            return nktrue;
        }
    }

    return nkfalse;
}

void subsystemTest_cleanup(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData;

    internalData = nkxGetExternalSubsystemDataOrError(
        data->vm, "subsystemTest");

    if(internalData) {

        struct NKValue ob;
        nkuint32_t index = 0;
        while(nkxGetNextObjectOfExternalType(data->vm, internalData->widgetTypeId, &ob, &index)) {
            // FIXME: Won't currently work for cleanup of OOMs,
            // because nkxVmObjectGetExternalData can throw errors,
            // and will refuse to start if the OOM flag is set.
            void *objectExternalData = nkxVmObjectGetExternalData(data->vm, &ob);
            printf("OBJECT FOUND FOR CLEANUP: " NK_PRINTF_UINT32 " - " NK_PRINTF_UINT32 " - %p\n",
                ob.objectId, index, objectExternalData);
        }

        // TODO: Free external data on all objects maintained by this
        // system. (This approach touches internal VM data directly,
        // and we need to make something a little nicer.)
        {
            nkuint32_t i;
            if(data->vm->objectTable.objectTable) {
                for(i = 0; i < data->vm->objectTable.capacity; i++) {
                    struct NKVMObject *ob = data->vm->objectTable.objectTable[i];
                    if(ob && ob->externalDataType.id == internalData->widgetTypeId.id) {
                        if(ob->externalData) {
                            subsystemTest_freeWrapper(ob->externalData);

                            printf("Free widgetData: %p\n", ob->externalData);

                            ob->externalData = NULL;
                            ob->externalDataType.id = NK_INVALID_VALUE;
                        }
                    }
                }
            }
        }

        // Free all of our internal data that's not attached to single
        // objects.
        subsystemTest_freeWrapper(internalData->testString);
        subsystemTest_freeWrapper(internalData);
    }

    // Note: Do NOT assert here on !subsystemTest_debugOnly_dataCount.
    // We may have multiple VM instances.
}

void subsystemTest_serialize(struct NKVMFunctionCallbackData *data)
{
    // TODO: Make a string serialization function.

    struct SubsystemTest_InternalData *internalData;

    internalData = nkxGetExternalSubsystemDataOrError(
        data->vm, "subsystemTest");

    if(internalData) {

        nkuint32_t len = internalData->testString ? strlen(internalData->testString) : 0;
        nkxSerializeData(data->vm, &len, sizeof(len));

        if(!nkxSerializerGetWriteMode(data->vm)) {
            if(internalData->testString) {
                subsystemTest_freeWrapper(internalData->testString);
            }
            if(len) {
                internalData->testString = subsystemTest_mallocWrapper(
                    1 + len,
                    nkxSerializerGetWriteMode(data->vm) ?
                    "subsystemTest_serialize (write)" :
                    "subsystemTest_serialize (read)");
                if(!internalData->testString) {
                    return;
                }
            } else {
                internalData->testString = NULL;
            }
        }

        if(internalData->testString) {
            nkxSerializeData(data->vm, internalData->testString, len);
            internalData->testString[len] = 0;
        }
    }
}

void subsystemTest_initLibrary(struct NKVM *vm, struct NKCompilerState *cs)
{
    // Order in this function is pretty important if you want to
    // maintain OOM condition safety. Cleanup functions must be in
    // place before any other allocations from the VM, so make sure
    // the cleanup operation is the first callback set.

    struct SubsystemTest_InternalData *internalData = NULL;

    printf("subsystemTest: Initializing on VM: %p\n", vm);

    if(nkxVmHasErrors(vm)) {
        return;
    }

    // Two most important things for cleanup purposes. Set these up
    // before anything. DO NOT set up the subsystemTest internalData,
    // unless these calls succeed, or it will be unable to deallocate
    // the internal data, and this system will be unable to remove the
    // dangling pointer from the system.
    //
    // TL;DR: Always setup cleanup functions before setting up data
    // that may need to be cleaned up.
    nkxSetExternalSubsystemCleanupCallback(vm, "subsystemTest", subsystemTest_cleanup);
    nkxSetExternalSubsystemSerializationCallback(vm, "subsystemTest", subsystemTest_serialize);

    internalData = nkxGetExternalSubsystemData(vm, "subsystemTest");

    if(!internalData) {

        // Internal data doesn't yet exist. Better create it.

        internalData = subsystemTest_mallocWrapper(
            sizeof(struct SubsystemTest_InternalData),
            "subsystemTest_initLibrary");

        internalData->widgetTypeId = nkxVmRegisterExternalType(vm, "subsystemTest_widget");
        internalData->testString = NULL;
        nkxSetExternalSubsystemData(vm, "subsystemTest", internalData);

        subsystemTest_registerExitCheck();

        // If we had an error setting the subsystem data, then we need
        // to clean up the allocated internalData right here.
        if(nkxVmHasErrors(vm)) {
            subsystemTest_freeWrapper(internalData);
            internalData = NULL;
            return;
        }
    }

    nkxVmRegisterExternalFunction(vm, "subsystemTest_setTestString", subsystemTest_setTestString);
    nkxVmRegisterExternalFunction(vm, "subsystemTest_getTestString", subsystemTest_getTestString);
    nkxVmRegisterExternalFunction(vm, "subsystemTest_printTestString", subsystemTest_printTestString);

    nkxVmRegisterExternalFunction(vm, "subsystemTest_widgetCreate", subsystemTest_widgetCreate);
    nkxVmRegisterExternalFunction(vm, "subsystemTest_widgetSetData", subsystemTest_widgetSetData);
    nkxVmRegisterExternalFunction(vm, "subsystemTest_widgetGetData", subsystemTest_widgetGetData);
    internalData->widgetSerializeCallbackId = nkxVmRegisterExternalFunction(
        vm, "subsystemTest_widgetSerializeData", subsystemTest_widgetSerializeData);
    internalData->widgetGCCallbackId = nkxVmRegisterExternalFunction(
        vm, "subsystemTest_widgetGCData", subsystemTest_widgetGCData);

    if(cs) {

        printf("subsystemTest: Registering variables: %p\n", cs);

        nkxCompilerCreateCFunctionVariable(cs, "subsystemTest_setTestString", subsystemTest_setTestString);
        nkxCompilerCreateCFunctionVariable(cs, "subsystemTest_getTestString", subsystemTest_getTestString);
        nkxCompilerCreateCFunctionVariable(cs, "subsystemTest_printTestString", subsystemTest_printTestString);

        nkxCompilerCreateCFunctionVariable(cs, "subsystemTest_widgetCreate", subsystemTest_widgetCreate);
        nkxCompilerCreateCFunctionVariable(cs, "subsystemTest_widgetSetData", subsystemTest_widgetSetData);
        nkxCompilerCreateCFunctionVariable(cs, "subsystemTest_widgetGetData", subsystemTest_widgetGetData);
    }
}



#include "subtest.h"

#include "nkx.h"

// FIXME: Remove this after we write the serialization functions.
#include "nkvm.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

// TODO: Move this.
nkbool nkxSerializeData(struct NKVM *vm, void *data, nkuint32_t size)
{
    return vm->serializationState.writer(
        data, size,
        vm->serializationState.userdata,
        vm->serializationState.writeMode);
}

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
// Malloc wrapper and memory leak tracking.

struct SubsystemTest_MallocHeader
{
    char *description;
    nkuint32_t size;
    struct SubsystemTest_MallocHeader *nextBlock;
    struct SubsystemTest_MallocHeader **prevPtr;
};

static struct SubsystemTest_MallocHeader *subsystemTest_allocationList = NULL;
static nkuint32_t subsystemTest_debugOnly_dataCount = 0;

void *subsystemTest_mallocWrapper(nkuint32_t size, const char *description)
{
    struct SubsystemTest_MallocHeader *header = malloc(size + sizeof(struct SubsystemTest_MallocHeader));
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

void subsystemTest_freeWrapper(void *data)
{
    struct SubsystemTest_MallocHeader *header = (struct SubsystemTest_MallocHeader *)data - 1;
    if(!data) {
        return;
    }

    // Remove from allocation list.
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
    printf("First header: %p\n", header);
    while(header) {
        printf("Block still allocated: %p: %s\n", header + 1, header->description);
        header = header->nextBlock;
    }

    printf(
        "Debug data count: " NK_PRINTF_UINT32 "\n",
        subsystemTest_debugOnly_dataCount);
    assert(subsystemTest_debugOnly_dataCount == 0);
}

// ----------------------------------------------------------------------
// Widget-related functions

void subsystemTest_widgetCreate(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData;

    nkxFunctionCallbackCheckArguments(
        data, "subsystemTest_widgetCreate", 0);

    internalData = nkxGetExternalSubsystemDataOrError(
        data->vm, "subsystemTest");

    if(nkxVmHasErrors(data->vm)) {
        return;
    }

    {
        struct SubsystemTest_WidgetData *newData = subsystemTest_mallocWrapper(
            sizeof(struct SubsystemTest_WidgetData),
            "subsystemTest_widgetCreate");

        newData->data = 5678;

        nkxCreateObject(data->vm, &data->returnValue);

        // Check for errors after the heap-allocating call to
        // nkxCreateObject.
        if(!nkxVmHasErrors(data->vm)) {
            nkxVmObjectSetExternalType(
                data->vm, &data->returnValue, internalData->widgetTypeId);
            nkxVmObjectSetExternalData(data->vm, &data->returnValue, newData);
            nkxVmObjectSetGarbageCollectionCallback(
                data->vm, &data->returnValue, internalData->widgetGCCallbackId);
            nkxVmObjectSetSerializationCallback(
                data->vm, &data->returnValue, internalData->widgetSerializeCallbackId);
        } else {
            subsystemTest_freeWrapper(newData);
        }
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

    internalData = nkxGetExternalSubsystemDataOrError(data->vm, "subsystemTest");

    widgetData = nkxFunctionCallbackGetExternalDataArgument(
        data, "subsystemTest_widgetGetData", 0, internalData->widgetTypeId);

    if(nkxVmHasErrors(data->vm)) {
        return;
    }

    if(widgetData) {
        nkxValueSetInt(data->vm, &data->returnValue, widgetData->data);
    }
}

void subsystemTest_widgetSerializeData(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData =
        nkxGetExternalSubsystemData(data->vm, "subsystemTest");
    if(!nkxFunctionCallbackCheckArgCount(data, 1, "subsystemTest_widgetSerializeData")) return;
    if(!internalData) {
        nkxAddError(data->vm, "subsystemTest data not registered.");
    }

    if(data->arguments[0].type != NK_VALUETYPE_OBJECTID) {
        nkxAddError(data->vm, "Expected an object in subsystemTest_widgetSerializeData.");
    }

    if(nkxVmObjectGetExternalType(data->vm, &data->arguments[0]).id != internalData->widgetTypeId.id) {
        nkxAddError(data->vm, "Expected a widget in subsystemTest_widgetSerializeData.");
    }

    {
        struct SubsystemTest_WidgetData *widgetData = nkxVmObjectGetExternalData(data->vm, &data->arguments[0]);

        if(!widgetData) {

            widgetData = subsystemTest_mallocWrapper(
                sizeof(*widgetData),
                "subsystemTest_widgetSerializeData");

            widgetData->data = 0;
            nkxVmObjectSetExternalData(data->vm, &data->arguments[0], widgetData);
        }

        nkxSerializeData(data->vm, widgetData, sizeof(*widgetData));
    }
}

void subsystemTest_widgetGCData(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData =
        nkxGetExternalSubsystemData(data->vm, "subsystemTest");
    if(!nkxFunctionCallbackCheckArgCount(data, 1, "subsystemTest_widgetSerializeData")) return;
    if(!internalData) {
        nkxAddError(data->vm, "subsystemTest data not registered.");
    }

    if(data->arguments[0].type != NK_VALUETYPE_OBJECTID) {
        nkxAddError(data->vm, "Expected an object in subsystemTest_widgetSerializeData.");
    }

    printf("data: %p\n", data);
    printf("data->vm: %p\n", data->vm);
    printf("data->arguments: %p\n", data->arguments);
    printf("internalData: %p\n", internalData);

    // if(nkxVmObjectGetExternalType(data->vm, &data->arguments[0]).id != internalData->widgetTypeId.id) {
    //     nkxAddError(data->vm, "Expected a widget in subsystemTest_widgetSerializeData.\n");
    // }

    {
        struct SubsystemTest_WidgetData *widgetData = nkxVmObjectGetExternalData(data->vm, &data->arguments[0]);
        subsystemTest_freeWrapper(widgetData);
        nkxVmObjectSetExternalData(data->vm, &data->arguments[0], NULL);
    }


    // FIXME: Remove this.
    assert(!data->vm->errorState.allocationFailure);
}

// ----------------------------------------------------------------------
// Functions callable from script code

// These functions all have a lot of common boilerplate to ensure that
// the internal data exists. Normally I would abstract that all away
// with a #define or something, but this is example code so it's
// better to leave it for the sake of demonstration.

void subsystemTest_setTestString(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData =
        nkxGetExternalSubsystemData(data->vm, "subsystemTest");
    if(!nkxFunctionCallbackCheckArgCount(data, 1, "subsystemTest_setTestString")) return;

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
    struct SubsystemTest_InternalData *internalData =
        nkxGetExternalSubsystemData(data->vm, "subsystemTest");
    if(!nkxFunctionCallbackCheckArgCount(data, 0, "subsystemTest_getTestString")) return;

    nkxValueSetString(data->vm, &data->returnValue, internalData->testString);
}

void subsystemTest_printTestString(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData =
        nkxGetExternalSubsystemData(data->vm, "subsystemTest");

    if(!nkxFunctionCallbackCheckArgCount(data, 0, "subsystemTest_printTestString")) return;

    printf("subsystemTest_print: %s\n", internalData->testString);
}

// ----------------------------------------------------------------------
// Cleanup, init, serialization, etc

void subsystemTest_cleanup(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData =
        nkxGetExternalSubsystemData(data->vm, "subsystemTest");

    printf(
        "subsystemTest: Running cleanup (" NK_PRINTF_UINT32 ") for %p\n",
        subsystemTest_debugOnly_dataCount,
        internalData);

    if(internalData) {

        // TODO: Free external data on all objects maintained by this
        // system.
        {
            nkuint32_t i;
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

        printf("Free internalData: %p\n", internalData);
        subsystemTest_freeWrapper(internalData->testString);
        subsystemTest_freeWrapper(internalData);

        printf(
            "subsystemTest: Datacount now: (" NK_PRINTF_UINT32 ") for %p\n",
            subsystemTest_debugOnly_dataCount,
            internalData);
    }

    printf("Remaining data: " NK_PRINTF_UINT32 "\n", subsystemTest_debugOnly_dataCount);

    // Note: Do NOT assert here on !subsystemTest_debugOnly_dataCount.
    // We may have multiple VM instances.
}

void subsystemTest_serialize(struct NKVMFunctionCallbackData *data)
{
    // TODO: Make a string serialization function.

    // TODO: Make a generic serialization function that keeps us from
    // having to write out all this junk every time. Then fix up
    // nksave.c to use it everywhere.

    struct SubsystemTest_InternalData *internalData =
        nkxGetExternalSubsystemData(data->vm, "subsystemTest");

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
    struct SubsystemTest_InternalData *internalData = NULL;

    printf("subsystemTest: Initializing on VM: %p\n", vm);

    // Internal data might already exist, because this function gets
    // called multiple times (once for setup that applies to
    // everything (both compiled and deserialized), and once for setup
    // that applies to the compiler.
    if(!nkxGetExternalSubsystemData(vm, "subsystemTest")) {
        internalData = subsystemTest_mallocWrapper(
            sizeof(struct SubsystemTest_InternalData),
            "subsystemTest_initLibrary");

        internalData->widgetTypeId = nkxVmRegisterExternalType(vm, "subsystemTest_widget");
        internalData->testString = NULL;
        nkxSetExternalSubsystemData(vm, "subsystemTest", internalData);

        if(nkxVmHasErrors(vm)) {
            nkxSetExternalSubsystemData(vm, "subsystemTest", NULL);
            subsystemTest_freeWrapper(internalData);
        }

        atexit(subsystemTest_debugOnly_exitCheck);
    }

    internalData = nkxGetExternalSubsystemData(vm, "subsystemTest");
    if(!internalData) return;

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
        // nkxCompilerCreateCFunctionVariable(cs, "subsystemTest_widgetSerializeData", subsystemTest_widgetSerializeData);
        // nkxCompilerCreateCFunctionVariable(cs, "subsystemTest_widgetGCData", subsystemTest_widgetGCData);
    }

    nkxSetExternalSubsystemCleanupCallback(vm, "subsystemTest", subsystemTest_cleanup);
    nkxSetExternalSubsystemSerializationCallback(vm, "subsystemTest", subsystemTest_serialize);
}



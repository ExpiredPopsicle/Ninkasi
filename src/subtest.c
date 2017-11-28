#include "subtest.h"

#include "nkx.h"

// FIXME: Remove this after we write the serialization functions.
#include "nkvm.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

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

// This function exists only to check that every piece of data we've
// created has been cleaned up, meaning that we're still able to clean
// up our data in case the VM has a catastrophic (allocation) error.
static nkuint32_t subsystemTest_debugOnly_dataCount = 0;
void subsystemTest_debugOnly_exitCheck(void)
{
    printf("Debug data count: " NK_PRINTF_UINT32 "\n", subsystemTest_debugOnly_dataCount);
    assert(subsystemTest_debugOnly_dataCount == 0);
}

// ----------------------------------------------------------------------
// Widget-related functions

void subsystemTest_widgetCreate(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData =
        nkxGetExternalSubsystemData(data->vm, "subsystemTest");
    if(!nkxFunctionCallbackCheckArgCount(data, 0, "subsystemTest_widgetCreate")) return;
    if(!internalData) {
        nkxAddError(data->vm, "subsystemTest data not registered.\n");
    }

    {
        struct SubsystemTest_WidgetData *newData = malloc(sizeof(struct SubsystemTest_WidgetData));
        newData->data = 5678;

        nkxCreateObject(data->vm, &data->returnValue);
        nkxVmObjectSetExternalType(
            data->vm, &data->returnValue, internalData->widgetTypeId);
        nkxVmObjectSetExternalData(data->vm, &data->returnValue, newData);
        nkxVmObjectSetGarbageCollectionCallback(
            data->vm, &data->returnValue, internalData->widgetGCCallbackId);
        nkxVmObjectSetSerializationCallback(
            data->vm, &data->returnValue, internalData->widgetSerializeCallbackId);
    }
}

void subsystemTest_widgetSetData(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData =
        nkxGetExternalSubsystemData(data->vm, "subsystemTest");
    if(!nkxFunctionCallbackCheckArgCount(data, 2, "subsystemTest_widgetSetData")) return;
    if(!internalData) {
        nkxAddError(data->vm, "subsystemTest data not registered.\n");
    }

    if(data->arguments[0].type != NK_VALUETYPE_OBJECTID) {
        nkxAddError(data->vm, "Expected an object in subsystemTest_widgetSetData.\n");
    }

    if(nkxVmObjectGetExternalType(data->vm, &data->arguments[0]).id != internalData->widgetTypeId.id) {
        nkxAddError(data->vm, "Expected a widget in subsystemTest_widgetSetData.\n");
    }

    {
        struct SubsystemTest_WidgetData *widgetData = nkxVmObjectGetExternalData(data->vm, &data->arguments[0]);
        widgetData->data = nkxValueToInt(data->vm, &data->arguments[1]);
    }
}

void subsystemTest_widgetGetData(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData =
        nkxGetExternalSubsystemData(data->vm, "subsystemTest");
    if(!nkxFunctionCallbackCheckArgCount(data, 1, "subsystemTest_widgetGetData")) return;
    if(!internalData) {
        nkxAddError(data->vm, "subsystemTest data not registered.\n");
    }

    if(data->arguments[0].type != NK_VALUETYPE_OBJECTID) {
        nkxAddError(data->vm, "Expected an object in subsystemTest_widgetGetData.\n");
    }

    if(nkxVmObjectGetExternalType(data->vm, &data->arguments[0]).id != internalData->widgetTypeId.id) {
        nkxAddError(data->vm, "Expected a widget in subsystemTest_widgetGetData.\n");
    }

    {
        struct SubsystemTest_WidgetData *widgetData = nkxVmObjectGetExternalData(data->vm, &data->arguments[0]);
        nkxValueSetInt(data->vm, &data->returnValue, widgetData->data);
    }
}

void subsystemTest_widgetSerializeData(struct NKVMFunctionCallbackData *data)
{
    // printf("subsystemTest: Widget serialization callback hit. Mode: %s\n",
    //     data->vm->serializationState.writeMode ? "write" : "read");

    struct SubsystemTest_InternalData *internalData =
        nkxGetExternalSubsystemData(data->vm, "subsystemTest");
    if(!nkxFunctionCallbackCheckArgCount(data, 1, "subsystemTest_widgetSerializeData")) return;
    if(!internalData) {
        nkxAddError(data->vm, "subsystemTest data not registered.\n");
    }

    if(data->arguments[0].type != NK_VALUETYPE_OBJECTID) {
        nkxAddError(data->vm, "Expected an object in subsystemTest_widgetSerializeData.\n");
    }

    if(nkxVmObjectGetExternalType(data->vm, &data->arguments[0]).id != internalData->widgetTypeId.id) {
        nkxAddError(data->vm, "Expected a widget in subsystemTest_widgetSerializeData.\n");
    }

    {
        struct SubsystemTest_WidgetData *widgetData = nkxVmObjectGetExternalData(data->vm, &data->arguments[0]);

        if(!widgetData) {
            widgetData = malloc(sizeof(*widgetData));
            widgetData->data = 0;
            nkxVmObjectSetExternalData(data->vm, &data->arguments[0], widgetData);
        }

        data->vm->serializationState.writer(
            widgetData, sizeof(*widgetData),
            data->vm->serializationState.userdata,
            data->vm->serializationState.writeMode);
    }
}

void subsystemTest_widgetGCData(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData =
        nkxGetExternalSubsystemData(data->vm, "subsystemTest");
    if(!nkxFunctionCallbackCheckArgCount(data, 1, "subsystemTest_widgetSerializeData")) return;
    if(!internalData) {
        nkxAddError(data->vm, "subsystemTest data not registered.\n");
    }

    if(data->arguments[0].type != NK_VALUETYPE_OBJECTID) {
        nkxAddError(data->vm, "Expected an object in subsystemTest_widgetSerializeData.\n");
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
        free(widgetData);
        nkxVmObjectSetExternalData(data->vm, &data->arguments[0], NULL);
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
    struct SubsystemTest_InternalData *internalData =
        nkxGetExternalSubsystemData(data->vm, "subsystemTest");
    if(!nkxFunctionCallbackCheckArgCount(data, 1, "subsystemTest_setTestString")) return;

    if(internalData->testString) {
        free(internalData->testString);
        internalData->testString = NULL;
    }

    internalData->testString = strdup(nkxValueToString(data->vm, &data->arguments[0]));
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

        free(internalData->testString);
        free(internalData);
        subsystemTest_debugOnly_dataCount--;

        printf(
            "subsystemTest: Datacount now: (" NK_PRINTF_UINT32 ") for %p\n",
            subsystemTest_debugOnly_dataCount,
            internalData);
    }
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
        data->vm->serializationState.writer(
            &len, sizeof(len),
            data->vm->serializationState.userdata,
            data->vm->serializationState.writeMode);

        if(!data->vm->serializationState.writeMode) {
            if(internalData->testString) {
                free(internalData->testString);
            }
            if(len) {
                internalData->testString = malloc(1 + len);
                if(!internalData->testString) {
                    return;
                }
            } else {
                internalData->testString = NULL;
            }
        }

        if(internalData->testString) {
            data->vm->serializationState.writer(
                internalData->testString, len,
                data->vm->serializationState.userdata,
                data->vm->serializationState.writeMode);
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
        internalData = malloc(sizeof(struct SubsystemTest_InternalData));
        subsystemTest_debugOnly_dataCount++;
        internalData->widgetTypeId = nkxVmRegisterExternalType(vm, "subsystemTest_widget");
        internalData->testString = NULL;
        nkxSetExternalSubsystemData(vm, "subsystemTest", internalData);
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



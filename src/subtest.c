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

    char *testString;
};

// This function exists only to check that every piece of data we've
// created has been cleaned up, meaning that we're still able to clean
// up our data in case the VM has a catastrophic (allocation) error.
static nkuint32_t subsystemTest_debugOnly_dataCount = 0;
void subsystemTest_debugOnly_exitCheck(void)
{
    assert(subsystemTest_debugOnly_dataCount == 0);
}

void subsystemTest_cleanup(struct NKVMFunctionCallbackData *data)
{
    struct SubsystemTest_InternalData *internalData =
        nkxGetExternalSubsystemData(data->vm, "subsystemTest");

    printf("subsystemTest: Running cleanup (" NK_PRINTF_UINT32 ")\n", subsystemTest_debugOnly_dataCount);

    if(internalData) {

        // TODO: Free external data on all objects maintained by this
        // system.

        free(internalData->testString);
        free(internalData);
        subsystemTest_debugOnly_dataCount--;
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

        data->vm->serializationState.writer(
            internalData->testString, len,
            data->vm->serializationState.userdata,
            data->vm->serializationState.writeMode);
    }
}

void subsystemTest_initLibrary(struct NKVM *vm, struct NKCompilerState *cs)
{
    printf("subsystemTest: Initializing on VM: %p\n", vm);

    if(!nkxGetExternalSubsystemData(vm, "subsystemTest")) {

        struct SubsystemTest_InternalData *internalData = malloc(sizeof(struct SubsystemTest_InternalData));

        subsystemTest_debugOnly_dataCount++;
        internalData->widgetTypeId = nkxVmRegisterExternalType(vm, "subsystemTest_widget");
        internalData->testString = NULL;

        if(cs) {
            // TODO: Set up variables for C funcs.
        }

        nkxSetExternalSubsystemData(vm, "subsystemTest", internalData);
        nkxSetExternalSubsystemCleanupCallback(vm, "subsystemTest", subsystemTest_cleanup);
        nkxSetExternalSubsystemSerializationCallback(vm, "subsystemTest", subsystemTest_serialize);

        atexit(subsystemTest_debugOnly_exitCheck);

    }
}



#include "subtest.h"

#include "nkx.h"

#include <stdlib.h>
#include <assert.h>

struct SubsystemTest_InternalData
{
    NKVMExternalDataTypeID widgetTypeId;
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

    if(internalData) {

        // TODO: Free external data on all objects maintained by this
        // system.

        free(internalData);
    }
}

void subsystemTest_serialize(struct NKVMFunctionCallbackData *data)
{
}

void subsystemTest_initLibrary(struct NKVM *vm)
{
    struct SubsystemTest_InternalData *internalData = malloc(sizeof(struct SubsystemTest_InternalData));
    internalData->widgetTypeId = nkxVmRegisterExternalType(vm, "subsystemTest_widget");

    nkxSetExternalSubsystemData(vm, "subsystemTest", internalData);
    nkxSetExternalSubsystemCleanupCallback(vm, "subsystemTest", subsystemTest_cleanup);
    nkxSetExternalSubsystemSerializationCallback(vm, "subsystemTest", subsystemTest_serialize);

    atexit(subsystemTest_debugOnly_exitCheck);
}



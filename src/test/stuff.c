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

#include "../nkx.h"
#include "subtest.h"
#include "stuff.h"

#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

void testHandle1(struct NKVMFunctionCallbackData *data)
{
    if(!nkxFunctionCallbackCheckArgCount(data, 1, "testHandle1")) return;

    nkxVmObjectAcquireHandle(data->vm, &data->arguments[0]);
}

void testHandle2(struct NKVMFunctionCallbackData *data)
{
    if(!nkxFunctionCallbackCheckArgCount(data, 1, "testHandle2")) return;

    nkxVmObjectReleaseHandle(data->vm, &data->arguments[0]);
}


void vmFuncPrint(struct NKVMFunctionCallbackData *data)
{
    nkuint32_t i;

    for(i = 0; i < data->argumentCount; i++) {
        // printf("\033[1m%s\033[0m", nkxValueToString(data->vm, &data->arguments[i]));
        printf("%s", nkxValueToString(data->vm, &data->arguments[i]));
    }
}

// // This should probably be tracked separately for each VM.
// static NKVMExternalFunctionID doGCCallbackThing_id;

void doGCCallbackThing(struct NKVMFunctionCallbackData *data)
{
    // TODO: Add standard argument count handler.
    if(data->argumentCount != 1) {
        nkxAddError(
            data->vm,
            "Bad argument count in doGCCallbackThing.");
        return;
    }

    // {
    //     struct NKVMExternalDataTypeID id = nkxVmObjectGetExternalType(data->vm, &data->arguments[0]);
    //     if(id.id != NK_INVALID_VALUE) {
    //         printf("This thing isn't a basic object!\n");
    //         return;
    //     }
    // }

    assert(data->argumentCount == 1);
    printf("GC callback called.\n");
    {
        NKVMExternalDataTypeID id = nkxVmObjectGetExternalType(data->vm, &data->arguments[0]);
        // if(strcmp(nkxVmGetExternalTypeName(vm, id), "footype")) {
        //     printf("This thing isn't a basic object!\n");
        //     return;
        // }
        printf("GCing external of type %s\n", nkxVmGetExternalTypeName(data->vm, id));
    }

    {
        char *externalData = (char*)nkxVmObjectGetExternalData(data->vm, &data->arguments[0]);
        printf("GCing external data: %s\n", externalData);
        // free(externalData);
    }
}

void setGCCallbackThing(struct NKVMFunctionCallbackData *data)
{
    // if(data->argumentCount != 1) {
    //     nkiAddError(
    //         data->vm,
    //         -1, "Bad argument count in setGCCallbackThing.");
    //     return;
    // }

    // assert(data->argumentCount == 1);
    // printf("Setting GC callback.\n");

    // doGCCallbackThing_id =
    //     nkxVmRegisterExternalFunction(data->vm, "doGCCallbackThing", doGCCallbackThing);

    // {
    //     struct NKVMExternalDataTypeID id = nkxVmObjectGetExternalType(data->vm, &data->arguments[0]);
    //     if(id.id != NK_INVALID_VALUE) {
    //         printf("This thing isn't a basic object!\n");
    //         return;
    //     }
    // }

    // nkxVmObjectSetGarbageCollectionCallback(
    //     data->vm, &data->arguments[0], doGCCallbackThing_id);

    // nkxVmObjectSetExternalData(data->vm, &data->arguments[0], "butts");

    // {
    //     NKVMExternalFunctionID serializationCallbackId = nkxVmRegisterExternalFunction(
    //         data->vm, "doSerializationCallbackThing", doSerializationCallbackThing);
    //     nkxVmObjectSetSerializationCallback(
    //         data->vm, &data->arguments[0], serializationCallbackId);
    // }

    // {
    //     NKVMExternalDataTypeID id = nkxVmRegisterExternalType(data->vm, "footype");
    //     printf("Registered external type %s\n", nkxVmGetExternalTypeName(data->vm, id));
    //     nkxVmObjectSetExternalType(data->vm, &data->arguments[0], id);
    // }
}

void testSubsystemCleanup(struct NKVMFunctionCallbackData *data)
{
}

void testVMFunc(struct NKVMFunctionCallbackData *data)
{
    // Thanks AFL!
    static nkuint32_t recursionCounter = 0;
    if(recursionCounter > 32) {
        return;
    }
    recursionCounter++;

    // nkuint32_t i;
    printf("testVMFunc hit!\n");
    // for(i = 0; i < data->argumentCount; i++) {
    //     printf("Argument %d: %s\n", i,
    //         nkxValueToString(data->vm, &data->arguments[i]));
    // }

    data->returnValue.intData = 565656;

    if(data->argumentCount != 1) {
        nkxAddError(
            data->vm,
            "Bad argument count in testVMFunc.");
        return;
    }

    nkxVmCallFunction(data->vm, &data->arguments[0], 0, NULL, &data->returnValue);

    printf("Got data back from VM: %s\n", nkxValueToString(data->vm, &data->returnValue));
}

void testVMCatastrophe(struct NKVMFunctionCallbackData *data)
{
    nkxForceCatastrophicFailure(data->vm);
}

void getHash(struct NKVMFunctionCallbackData *data)
{
    if(!nkxFunctionCallbackCheckArgCount(data, 1, "getHash")) return;

    nkiValueSetInt(
        data->vm,
        &data->returnValue,
        nkiValueHash(data->vm, &data->arguments[0]));
}

void initInternalFunctions(struct NKVM *vm, struct NKCompilerState *cs)
{
    subsystemTest_initLibrary(vm, cs);

    nkxVmRegisterExternalFunction(vm, "cfunc", testVMFunc);
    nkxVmRegisterExternalFunction(vm, "cfunc", testVMFunc);
    nkxVmRegisterExternalFunction(vm, "cfunc", testVMFunc);
    nkxVmRegisterExternalFunction(vm, "cfunc", testVMFunc);
    nkxVmRegisterExternalFunction(vm, "cfunc", testVMFunc);
    nkxVmRegisterExternalFunction(vm, "catastrophe", testVMCatastrophe);
    nkxVmRegisterExternalFunction(vm, "print", vmFuncPrint);
    nkxVmRegisterExternalFunction(vm, "hash", getHash);
    nkxVmRegisterExternalFunction(vm, "hash2", getHash);
    nkxVmRegisterExternalFunction(vm, "testHandle1", testHandle1);
    nkxVmRegisterExternalFunction(vm, "testHandle2", testHandle2);
    nkxVmRegisterExternalFunction(vm, "setGCCallbackThing", setGCCallbackThing);

    // FIXME: FIX THE DEMO. DON'T REGISTER THIS LATE.
    nkxVmRegisterExternalFunction(vm, "doGCCallbackThing", doGCCallbackThing);

    nkxVmRegisterExternalType(vm, "footype", NULL, NULL, NULL);

    // nkxSetExternalSubsystemCleanupCallback(vm, "testSubsystem", testSubsystemCleanup);
    // nkxSetExternalSubsystemSerializationCallback(vm, "testSubsystem", testSubsystemSerialize);
    // nkxSetExternalSubsystemData(vm, "testSubsystem", "ASDFASDFASDFASDFASDFASDF");

    if(cs) {

        nkxCompilerCreateCFunctionVariable(cs, "cfunc", testVMFunc);
        // nkxCompilerCreateCFunctionVariable(cs, "cfunc", testVMFunc);
        // nkxCompilerCreateCFunctionVariable(cs, "cfunc", testVMFunc);
        // nkxCompilerCreateCFunctionVariable(cs, "cfunc", testVMFunc);
        // nkxCompilerCreateCFunctionVariable(cs, "cfunc", testVMFunc);
        nkxCompilerCreateCFunctionVariable(cs, "catastrophe", testVMCatastrophe);
        nkxCompilerCreateCFunctionVariable(cs, "print", vmFuncPrint);
        nkxCompilerCreateCFunctionVariable(cs, "hash", getHash);
        nkxCompilerCreateCFunctionVariable(cs, "hash2", getHash);
        nkxCompilerCreateCFunctionVariable(cs, "testHandle1", testHandle1);
        nkxCompilerCreateCFunctionVariable(cs, "testHandle2", testHandle2);
        nkxCompilerCreateCFunctionVariable(cs, "setGCCallbackThing", setGCCallbackThing);

    }
}

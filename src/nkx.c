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

#include "nkcommon.h"
#include "nkx.h"

void *nkiDefaultMalloc(nkuint32_t size, void *userData)
{
    return malloc(size);
}

void nkiDefaultFree(void *ptr, void *userData)
{
    free(ptr);
}

struct NKVM *nkxVmCreateEx(
    struct NKVMCreateParams *params)
{
    NK_FAILURE_RECOVERY_DECL();

    struct NKVM *vm = params->mallocReplacement(
        sizeof(struct NKVM),
        params->mallocAndFreeReplacementUserData);

    if(!vm) {
        return NULL;
    }

    memset(vm, 0, sizeof(*vm));
    vm->mallocReplacement = params->mallocReplacement;
    vm->freeReplacement = params->freeReplacement;

    NK_SET_FAILURE_RECOVERY(vm);

    nkiVmInit(vm);

    NK_CLEAR_FAILURE_RECOVERY();

    return vm;
}

struct NKVM *nkxVmCreate(void)
{
    struct NKVMCreateParams params;
    memset(&params, 0, sizeof(params));
    params.mallocReplacement = nkiDefaultMalloc;
    params.freeReplacement = nkiDefaultFree;
    return nkxVmCreateEx(&params);
}

void nkxVmDelete(struct NKVM *vm)
{
    if(vm) {

        // FIXME: Remove this.
        nkiVmObjectTableSanityCheck(vm);

        nkiVmDestroy(vm);
        vm->freeReplacement(
            vm, vm->mallocAndFreeReplacementUserData);
    }
}

nkbool nkxVmExecuteProgram(struct NKVM *vm)
{
    nkbool ret;
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(nkfalse);
    ret = nkiVmExecuteProgram(vm);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

nkuint32_t nkxGetErrorCount(struct NKVM *vm)
{
    nkuint32_t ret;
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(1);
    ret = nkiGetErrorCount(vm);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

nkbool nkxVmHasErrors(struct NKVM *vm)
{
    return nkiVmHasErrors(vm);
}

void nkxVmIterate(struct NKVM *vm, nkuint32_t count)
{
    NK_FAILURE_RECOVERY_DECL();
    nkuint32_t i;

    NK_SET_FAILURE_RECOVERY_VOID();
    for(i = 0; i < count; i++) {

        // Check for end-of-program.
        if(vm->instructions[
                vm->instructionPointer &
                vm->instructionAddressMask].opcode == NK_OP_END)
        {
            break;
        }

        // Check for errors.
        if(vm->errorState.firstError || vm->errorState.allocationFailure) {
            break;
        }

        nkiVmIterate(vm);

    }
    NK_CLEAR_FAILURE_RECOVERY();
}

void nkxVmGarbageCollect(struct NKVM *vm)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiVmGarbageCollect(vm);
    NK_CLEAR_FAILURE_RECOVERY();
}

// FIXME: Add max iteration count param.
void nkxVmCallFunction(
    struct NKVM *vm,
    struct NKValue *functionValue,
    nkuint32_t argumentCount,
    struct NKValue *arguments,
    struct NKValue *returnValue)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiVmCallFunction(vm, functionValue, argumentCount, arguments, returnValue);
    NK_CLEAR_FAILURE_RECOVERY();
}

struct NKValue *nkxVmFindGlobalVariable(
    struct NKVM *vm, struct NKCompilerState *cs, const char *name)
{
    if(cs) {
        NK_FAILURE_RECOVERY_DECL();
        struct NKValue *ret;
        NK_SET_FAILURE_RECOVERY(NULL);
        {
            struct NKCompilerStateContextVariable *var =
                nkiCompilerLookupVariable(cs, name);
            if(var) {
                ret = &vm->staticSpace[var->position & vm->staticAddressMask];
            }
        }
        NK_CLEAR_FAILURE_RECOVERY();
        return ret;
    } else {
        return nkiVmFindGlobalVariable(vm, name);
    }
}

struct NKValue *nkxCompilerCreateGlobalVariable(
    struct NKCompilerState *cs,
    const char *name)
{
    NK_FAILURE_RECOVERY_DECL();
    struct NKVM *vm = cs->vm;
    struct NKValue *ret;
    NK_SET_FAILURE_RECOVERY(NULL);
    ret = nkiCompilerCreateGlobalVariable(cs, name);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

struct NKCompilerState *nkxCompilerCreate(
    struct NKVM *vm)
{
    NK_FAILURE_RECOVERY_DECL();
    struct NKCompilerState *ret;
    NK_SET_FAILURE_RECOVERY(NULL);
    ret = nkiCompilerCreate(vm);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

void nkxCompilerCreateCFunctionVariable(
    struct NKCompilerState *cs,
    const char *name,
    NKVMFunctionCallback func)
{
    NK_FAILURE_RECOVERY_DECL();
    struct NKVM *vm = cs->vm;
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiCompilerCreateCFunctionVariable(cs, name, func);
    NK_CLEAR_FAILURE_RECOVERY();
}

nkbool nkxCompilerCompileScript(
    struct NKCompilerState *cs,
    const char *script)
{
    NK_FAILURE_RECOVERY_DECL();
    struct NKVM *vm = cs->vm;
    nkbool ret;
    NK_SET_FAILURE_RECOVERY(nkfalse);
    ret = nkiCompilerCompileScript(cs, script);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

nkbool nkxCompilerCompileScriptFile(
    struct NKCompilerState *cs,
    const char *scriptFilename)
{
    struct NKVM *vm = cs->vm;
    FILE *in = fopen(scriptFilename, "rb");
    nkuint32_t len;
    char *buf;
    nkbool success;

    if(!in) {
        NK_FAILURE_RECOVERY_DECL();
        NK_SET_FAILURE_RECOVERY(nkfalse);
        nkiCompilerAddError(cs, "Cannot open script file.");
        NK_CLEAR_FAILURE_RECOVERY();
        return nkfalse;
    }

    fseek(in, 0, SEEK_END);
    len = ftell(in);
    fseek(in, 0, SEEK_SET);

    buf = malloc(len + 1);
    if(!buf) {
        fclose(in);
        nkiErrorStateSetAllocationFailFlag(vm);
        return nkfalse;
    }
    fread(buf, len, 1, in);
    buf[len] = 0;

    fclose(in);
    success = nkxCompilerCompileScript(cs, buf);
    free(buf);

    return success;
}

void nkxCompilerFinalize(
    struct NKCompilerState *cs)
{
    NK_FAILURE_RECOVERY_DECL();
    struct NKVM *vm = cs->vm;
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiCompilerFinalize(cs);
    NK_CLEAR_FAILURE_RECOVERY();
}

const char *nkxValueToString(struct NKVM *vm, struct NKValue *value)
{
    NK_FAILURE_RECOVERY_DECL();
    const char *ret = NULL;
    NK_SET_FAILURE_RECOVERY(NULL);
    ret = nkiValueToString(vm, value);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

nkint32_t nkxValueToInt(struct NKVM *vm, struct NKValue *value)
{
    NK_FAILURE_RECOVERY_DECL();
    nkint32_t ret = 0;
    NK_SET_FAILURE_RECOVERY(ret);
    ret = nkiValueToInt(vm, value);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

nkint32_t nkxValueToFloat(struct NKVM *vm, struct NKValue *value)
{
    NK_FAILURE_RECOVERY_DECL();
    float ret = 0;
    NK_SET_FAILURE_RECOVERY(ret);
    ret = nkiValueToFloat(vm, value);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

void nkxForceCatastrophicFailure(struct NKVM *vm)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    NK_CATASTROPHE();
    NK_CLEAR_FAILURE_RECOVERY();
}

nkbool nkxFunctionCallbackCheckArgCount(
    struct NKVMFunctionCallbackData *data,
    nkuint32_t argCount,
    const char *functionName)
{
    nkbool ret = nktrue;
    struct NKVM *vm = data->vm;
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(nkfalse);

    if(data->argumentCount != argCount) {
        struct NKDynString *dynStr = nkiDynStrCreate(
            data->vm, "Bad argument count in ");
        nkiDynStrAppend(dynStr, functionName);
        nkiAddError(
            data->vm, -1, dynStr->data);
        nkiDynStrDelete(dynStr);
        ret = nkfalse;
    }

    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

void nkxVmObjectAcquireHandle(struct NKVM *vm, struct NKValue *value)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiVmObjectAcquireHandle(vm, value);
    NK_CLEAR_FAILURE_RECOVERY();
}

void nkxVmObjectReleaseHandle(struct NKVM *vm, struct NKValue *value)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiVmObjectReleaseHandle(vm, value);
    NK_CLEAR_FAILURE_RECOVERY();
}

void nkxSetUserData(struct NKVM *vm, void *userData)
{
    vm->userData = userData;
}

void *nkxGetUserData(struct NKVM *vm)
{
    return vm->userData;
}

NKVMExternalFunctionID nkxVmRegisterExternalFunction(
    struct NKVM *vm,
    const char *name,
    NKVMFunctionCallback func)
{
    NKVMExternalFunctionID ret = { NK_INVALID_VALUE };
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(ret);
    ret = nkiVmRegisterExternalFunction(vm, name, func);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

NKVMInternalFunctionID nkxVmGetOrCreateInternalFunctionForExternalFunction(
    struct NKVM *vm, NKVMExternalFunctionID externalFunctionId)
{
    NKVMInternalFunctionID ret = { NK_INVALID_VALUE };
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(ret);
    ret = nkiVmGetOrCreateInternalFunctionForExternalFunction(vm, externalFunctionId);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

NKVMExternalDataTypeID nkxVmRegisterExternalType(
    struct NKVM *vm, const char *name,
    NKVMExternalObjectSerializationCallback serializationCallback,
    NKVMExternalObjectCleanupCallback cleanupCallback)
{
    NKVMExternalDataTypeID ret = { NK_INVALID_VALUE };
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(ret);
    ret = nkiVmRegisterExternalType(
        vm, name, serializationCallback, cleanupCallback);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

NKVMExternalDataTypeID nkxVmFindExternalType(
    struct NKVM *vm, const char *name)
{
    return nkiVmFindExternalType(vm, name);
}

const char *nkxVmGetExternalTypeName(
    struct NKVM *vm, NKVMExternalDataTypeID id)
{
    return nkiVmGetExternalTypeName(vm, id);
}

void nkxVmObjectSetExternalType(
    struct NKVM *vm,
    struct NKValue *object,
    NKVMExternalDataTypeID externalType)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiVmObjectSetExternalType(vm, object, externalType);
    NK_CLEAR_FAILURE_RECOVERY();
}

NKVMExternalDataTypeID nkxVmObjectGetExternalType(
    struct NKVM *vm,
    struct NKValue *object)
{
    NKVMExternalDataTypeID ret = { NK_INVALID_VALUE };
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(ret);
    ret = nkiVmObjectGetExternalType(vm, object);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

void nkxVmObjectSetExternalData(
    struct NKVM *vm,
    struct NKValue *object,
    void *data)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiVmObjectSetExternalData(vm, object, data);
    NK_CLEAR_FAILURE_RECOVERY();
}

// No error handler, and no internal allocations. This is a
// cleanup-safe call.
void *nkxVmObjectGetExternalData(
    struct NKVM *vm,
    struct NKValue *object)
{
    return nkiVmObjectGetExternalData(vm, object);
}

nkbool nkxVmSerialize(struct NKVM *vm, NKVMSerializationWriter writer, void *userdata, nkbool writeMode)
{
    NK_FAILURE_RECOVERY_DECL();
    nkbool ret = nkfalse;
    NK_SET_FAILURE_RECOVERY(ret);
    ret = nkiVmSerialize(vm, writer, userdata, writeMode);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

void nkxDbgDumpState(struct NKVM *vm, FILE *stream)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiDbgDumpState(vm, stream);
    NK_CLEAR_FAILURE_RECOVERY();
}

void nkxAddError(
    struct NKVM *vm,
    const char *str)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiAddError(vm, -1, str);
    NK_CLEAR_FAILURE_RECOVERY();
}

nkuint32_t nkxGetErrorLength(struct NKVM *vm)
{
    // Makes no allocations. No need for wrapper.
    return nkiGetErrorLength(vm);
}

void nkxGetErrorText(struct NKVM *vm, char *buffer)
{
    // Makes no allocations. No need for wrapper.
    nkiGetErrorText(vm, buffer);
}

void nkxSetRemainingInstructionLimit(struct NKVM *vm, nkuint32_t count)
{
    vm->instructionsLeftBeforeTimeout = count;
}

nkuint32_t nkxGetRemainingInstructionLimit(struct NKVM *vm)
{
    return vm->instructionsLeftBeforeTimeout;
}

void nkxSetMaxStackSize(struct NKVM *vm, nkuint32_t maxStackSize)
{
    vm->limits.maxStackSize = maxStackSize;
}

nkuint32_t nkxGetMaxStackSize(struct NKVM *vm)
{
    return vm->limits.maxStackSize;
}

void nkxSetMaxFieldsPerObject(struct NKVM *vm, nkuint32_t maxFieldsPerObject)
{
    vm->limits.maxFieldsPerObject = maxFieldsPerObject;
}

nkuint32_t nkxGetMaxFieldsPerObject(struct NKVM *vm)
{
    return vm->limits.maxFieldsPerObject;
}

void nkxSetMaxAllocatedMemory(struct NKVM *vm, nkuint32_t maxAllocatedMemory)
{
    vm->limits.maxAllocatedMemory = maxAllocatedMemory;
}

nkuint32_t nkxGetMaxAllocatedMemory(struct NKVM *vm)
{
    return vm->limits.maxAllocatedMemory;
}

void nkxSetGarbageCollectionInterval(struct NKVM *vm, nkuint32_t gcInterval)
{
    if(vm->gcInfo.gcCountdown > gcInterval) {
        vm->gcInfo.gcCountdown = gcInterval;
    }
    vm->gcInfo.gcInterval = gcInterval;
}

nkuint32_t nkxGetGarbageCollectionInterval(struct NKVM *vm)
{
    return vm->gcInfo.gcInterval;
}

void nkxSetGarbageCollectionNewObjectInterval(struct NKVM *vm, nkuint32_t gcNewObjectInterval)
{
    if(vm->gcInfo.gcNewObjectCountdown > gcNewObjectInterval) {
        vm->gcInfo.gcNewObjectCountdown = gcNewObjectInterval;
    }
    vm->gcInfo.gcNewObjectInterval = gcNewObjectInterval;
}

nkuint32_t nkxGetGarbageCollectionNewObjectInterval(struct NKVM *vm)
{
    return vm->gcInfo.gcNewObjectInterval;
}

void nkxVmShrink(struct NKVM *vm)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiVmShrink(vm);
    NK_CLEAR_FAILURE_RECOVERY();
}

void *nkxGetExternalSubsystemData(
    struct NKVM *vm,
    const char *name)
{
    // This one needs to stay as non-errorable, even in case of
    // allocation failure. Otherwise subsystems can't clean up their
    // data.

    // NK_FAILURE_RECOVERY_DECL();
    void *ret = NULL;
    // NK_SET_FAILURE_RECOVERY(ret);
    ret = nkiGetExternalSubsystemData(vm, name);
    // NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

// void nkxSetExternalSubsystemData(
//     struct NKVM *vm,
//     const char *name,
//     void *data)
// {
//     NK_FAILURE_RECOVERY_DECL();
//     NK_SET_FAILURE_RECOVERY_VOID();
//     nkiSetExternalSubsystemData(vm, name, data);
//     NK_CLEAR_FAILURE_RECOVERY();
// }

// void nkxSetExternalSubsystemSerializationCallback(
//     struct NKVM *vm,
//     const char *name,
//     NKVMFunctionCallback serializationCallback)
// {
//     NK_FAILURE_RECOVERY_DECL();
//     NK_SET_FAILURE_RECOVERY_VOID();
//     nkiSetExternalSubsystemSerializationCallback(vm, name, serializationCallback);
//     NK_CLEAR_FAILURE_RECOVERY();
// }

// void nkxSetExternalSubsystemCleanupCallback(
//     struct NKVM *vm,
//     const char *name,
//     NKVMFunctionCallback cleanupCallback)
// {
//     NK_FAILURE_RECOVERY_DECL();
//     NK_SET_FAILURE_RECOVERY_VOID();
//     nkiSetExternalSubsystemCleanupCallback(vm, name, cleanupCallback);
//     NK_CLEAR_FAILURE_RECOVERY();
// }

void nkxValueSetInt(struct NKVM *vm, struct NKValue *value, nkint32_t intData)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiValueSetInt(vm, value, intData);
    NK_CLEAR_FAILURE_RECOVERY();
}

void nkxValueSetFloat(struct NKVM *vm, struct NKValue *value, float floatData)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiValueSetFloat(vm, value, floatData);
    NK_CLEAR_FAILURE_RECOVERY();
}

void nkxValueSetString(struct NKVM *vm, struct NKValue *value, const char *str)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiValueSetString(vm, value, str ? str : "");
    NK_CLEAR_FAILURE_RECOVERY();
}

void nkxCreateObject(struct NKVM *vm, struct NKValue *outValue)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    outValue->type = NK_VALUETYPE_OBJECTID;
    outValue->objectId = nkiVmObjectTableCreateObject(vm);
    NK_CLEAR_FAILURE_RECOVERY();
}

nkbool nkxFunctionCallbackCheckArguments(
    struct NKVMFunctionCallbackData *data,
    const char *functionName,
    nkuint32_t expectedArgumentCount,
    ...)
{
    struct NKVM *vm = data->vm;
    NK_FAILURE_RECOVERY_DECL();
    va_list args;
    nkuint32_t i;
    nkbool ret = nktrue;

    NK_SET_FAILURE_RECOVERY(nkfalse);

    // Check argument count first.
    if(expectedArgumentCount != data->argumentCount) {

        struct NKDynString *dynStr = nkiDynStrCreate(
            data->vm, functionName);
        nkiDynStrAppend(dynStr, ": Expected ");
        nkiDynStrAppendUint32(dynStr, expectedArgumentCount);
        nkiDynStrAppend(dynStr, " arguments, but passed ");
        nkiDynStrAppendUint32(dynStr, data->argumentCount);
        nkiDynStrAppend(dynStr, ".");
        nkiAddError(
            data->vm, -1, dynStr->data);
        nkiDynStrDelete(dynStr);

        ret = nkfalse;

    } else {

        va_start(args, expectedArgumentCount);

        for(i = 0; i < expectedArgumentCount; i++) {

            enum NKValueType thisType = va_arg(args, enum NKValueType);

            if(data->arguments[i].type != thisType) {

                struct NKDynString *dynStr = nkiDynStrCreate(
                    data->vm, functionName);
                nkiDynStrAppend(dynStr, ": Argument ");
                nkiDynStrAppendUint32(dynStr, i);
                nkiDynStrAppend(dynStr, ": Expected ");
                nkiDynStrAppend(dynStr, nkiValueTypeGetName(thisType));
                nkiDynStrAppend(dynStr, " but passed ");
                nkiDynStrAppend(dynStr, nkiValueTypeGetName(data->arguments[i].type));
                nkiDynStrAppend(dynStr, ".");
                nkiAddError(
                    data->vm, -1, dynStr->data);
                nkiDynStrDelete(dynStr);

                ret = nkfalse;
                break;
            }

        }

        va_end(args);
    }

    NK_CLEAR_FAILURE_RECOVERY();

    return ret;
}

void *nkxGetExternalSubsystemDataOrError(
    struct NKVM *vm,
    const char *name)
{
    void *ret = nkxGetExternalSubsystemData(vm, name);

    if(!ret) {

        struct NKDynString *dynStr = NULL;

        NK_FAILURE_RECOVERY_DECL();
        NK_SET_FAILURE_RECOVERY(NULL);

        dynStr = nkiDynStrCreate(
            vm, "Could not find subsystem data: ");
        nkiDynStrAppend(dynStr, name);
        nkiAddError(
            vm, -1, dynStr->data);
        nkiDynStrDelete(dynStr);

        NK_CLEAR_FAILURE_RECOVERY();

        return NULL;
    }

    return ret;
}

void *nkxFunctionCallbackGetExternalDataArgument(
    struct NKVMFunctionCallbackData *data,
    const char *functionName,
    nkuint32_t argumentNumber,
    NKVMExternalDataTypeID externalDataType)
{
    struct NKVM *vm = data->vm;
    void *ret = NULL;

    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(NULL);

    if(argumentNumber >= data->argumentCount) {

        struct NKDynString *dynStr = nkiDynStrCreate(
            data->vm, functionName);
        nkiDynStrAppend(dynStr, ": Tried to decode an external object that is beyond the end of the arguments list.");
        nkiAddError(
            data->vm, -1, dynStr->data);
        nkiDynStrDelete(dynStr);

    } else if(data->arguments[argumentNumber].type != NK_VALUETYPE_OBJECTID) {

        struct NKDynString *dynStr = nkiDynStrCreate(
            data->vm, functionName);
        nkiDynStrAppend(dynStr, ": Tried to decode an external object that is not even an object for argument ");
        nkiDynStrAppendUint32(dynStr, argumentNumber);
        nkiDynStrAppend(dynStr, ".");
        nkiAddError(
            data->vm, -1, dynStr->data);
        nkiDynStrDelete(dynStr);

    } else if(data->vm->externalTypeCount <= externalDataType.id) {

        struct NKDynString *dynStr = nkiDynStrCreate(
            data->vm, functionName);
        nkiDynStrAppend(dynStr, ": Argument ");
        nkiDynStrAppendUint32(dynStr, argumentNumber);
        nkiDynStrAppend(dynStr, ": Bad type specified in external function.");
        nkiAddError(
            data->vm, -1, dynStr->data);
        nkiDynStrDelete(dynStr);

    } else if(nkxVmObjectGetExternalType(data->vm, &data->arguments[0]).id != externalDataType.id) {

        struct NKDynString *dynStr = nkiDynStrCreate(
            data->vm, functionName);
        nkiDynStrAppend(dynStr, ": Argument ");
        nkiDynStrAppendUint32(dynStr, argumentNumber);
        nkiDynStrAppend(dynStr, ": Type mismatch. Expected ");
        nkiDynStrAppend(dynStr,
            data->vm->externalTypes ? data->vm->externalTypes[externalDataType.id].name : "badtype");
        nkiDynStrAppend(dynStr, ".");
        nkiAddError(
            data->vm, -1, dynStr->data);
        nkiDynStrDelete(dynStr);

    } else {

        ret = nkxVmObjectGetExternalData(data->vm, &data->arguments[argumentNumber]);

    }

    NK_CLEAR_FAILURE_RECOVERY();

    return ret;
}

nkbool nkxSerializerGetWriteMode(struct NKVM *vm)
{
    return vm->serializationState.writeMode;
}

nkbool nkxSerializeData(struct NKVM *vm, void *data, nkuint32_t size)
{
    if(!vm->serializationState.writer) {
        nkiAddError(vm, -1, "Attempted to access serialization writer outside of serialization.");
        return nkfalse;
    }

    return vm->serializationState.writer(
        data, size,
        vm->serializationState.userdata,
        vm->serializationState.writeMode);
}


nkbool nkxInitSubsystem(
    struct NKVM *vm, struct NKCompilerState *cs,
    const char *name,
    void *internalData,
    NKVMSubsystemCleanupCallback cleanupCallback,
    NKVMSubsystemSerializationCallback serializationCallback)
{
    struct NKVMExternalSubsystemData *subsystemData = NULL;
    nkbool ret = nktrue;

    if(nkxVmHasErrors(vm)) {

        nkxAddError(vm, "Attempting to initialize a subsystem on a broken VM.");
        return nkfalse;

    } else {

        NK_FAILURE_RECOVERY_DECL();
        NK_SET_FAILURE_RECOVERY(nkfalse);

        subsystemData = nkiFindExternalSubsystemData(
            vm, name, nktrue);

        NK_CLEAR_FAILURE_RECOVERY();

        if(subsystemData) {
            subsystemData->serializationCallback = serializationCallback;
            subsystemData->cleanupCallback = cleanupCallback;
            subsystemData->data = internalData;
        }
    }

    return ret;
}

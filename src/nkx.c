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

#include "nkcommon.h"
#include "nkx.h"

void *nkiDefaultMalloc(nkuint32_t size, void *userData)
{
    // Thanks, continuous integration with DOSBox! Some platforms have
    // a 16-bit size_t, so we can't even express the actual allocation
    // to the underlying OS.
    if(size > ~(size_t)0) {
        return NULL;
    }

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

    struct NKVM *vm =
        (struct NKVM *)params->mallocReplacement(
            sizeof(struct NKVM),
            params->mallocAndFreeReplacementUserData);

    if(!vm) {
        return NULL;
    }

    nkiMemset(vm, 0, sizeof(*vm));
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
    nkiMemset(&params, 0, sizeof(params));
    params.mallocReplacement = nkiDefaultMalloc;
    params.freeReplacement = nkiDefaultFree;
    return nkxVmCreateEx(&params);
}

void nkxVmDelete(struct NKVM *vm)
{
    if(vm) {
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
                vm->currentExecutionContext->instructionPointer &
                vm->instructionAddressMask].opcode == NK_OP_END)
        {
            nkiVmIterate(vm);
            break;
        }

        // Check for errors.
        if(nkxVmHasErrors(vm)) {
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
        struct NKValue *ret = NULL;
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
    const char *script,
    const char *filename)
{
    NK_FAILURE_RECOVERY_DECL();
    struct NKVM *vm = cs->vm;
    nkbool ret;
    NK_SET_FAILURE_RECOVERY(nkfalse);
    ret = nkiCompilerCompileScript(cs, script, filename);
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

    // Read file length.
    fseek(in, 0, SEEK_END);
    len = ftell(in);
    fseek(in, 0, SEEK_SET);

    buf = (char *)malloc(len + 1);
    if(!buf) {
        fclose(in);
        nkiErrorStateSetAllocationFailFlag(vm);
        return nkfalse;
    }
    fread(buf, len, 1, in);
    buf[len] = 0;

    fclose(in);
    success = nkxCompilerCompileScript(cs, buf, scriptFilename);
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

void nkxCompilerPartiallyFinalize(
    struct NKCompilerState *cs)
{
    NK_FAILURE_RECOVERY_DECL();
    struct NKVM *vm = cs->vm;
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiCompilerPartiallyFinalize(cs);
    NK_CLEAR_FAILURE_RECOVERY();
}

nkbool nkxCompilerIsAtRootContext(
    struct NKCompilerState *cs)
{
    return nkiCompilerIsAtRootContext(cs);
}

void nkxCompilerClearReplErrorState(
    struct NKCompilerState *cs,
    nkuint32_t instructionPointerReset)
{
    NK_FAILURE_RECOVERY_DECL();
    struct NKVM *vm = cs->vm;
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiCompilerClearReplErrorState(
        cs,
        instructionPointerReset);

    NK_CLEAR_FAILURE_RECOVERY();
}

nkuint32_t nkxCompilerGetInstructionWriteIndex(
    struct NKCompilerState *cs)
{
    return cs->instructionWriteIndex;
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

float nkxValueToFloat(struct NKVM *vm, struct NKValue *value)
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
            data->vm, dynStr->data);
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

nkuint32_t nkxVmObjectGetExternalHandleCount(struct NKVM *vm, struct NKValue *value)
{
    return nkiVmObjectGetExternalHandleCount(vm, value);
}

void nkxSetUserData(struct NKVM *vm, void *userData)
{
    vm->userData = userData;
}

void *nkxGetUserData(struct NKVM *vm)
{
    return vm->userData;
}

// We split this off into its own function so we can handle the malloc
// errors using the normal nkx wrapper.
static void nkiSetupExternalFunction_addArgs(
    struct NKVM *vm,
    struct NKVMExternalFunction *func,
    nkuint32_t argumentCount)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();

    if(func->argTypes) {
        nkiFree(vm, func->argTypes);
    }

    func->argTypes = (enum NKValueType *)nkiMallocArray(
        vm,
        sizeof(*func->argTypes),
        argumentCount);

    func->argExternalTypes = (NKVMExternalDataTypeID *)nkiMallocArray(
        vm,
        sizeof(*func->argExternalTypes),
        argumentCount);

    // Clear out the value types to wildcards that just accept
    // anything.
    {
        nkuint32_t i;
        for(i = 0; i < argumentCount; i++) {
            func->argTypes[i] = NK_VALUETYPE_NIL;
            func->argExternalTypes[i].id = NK_INVALID_VALUE;
        }
    }

    NK_CLEAR_FAILURE_RECOVERY();
}

NKVMExternalFunctionID nkxVmSetupExternalFunction(
    struct NKVM *vm, struct NKCompilerState *cs,
    const char *name, NKVMFunctionCallback func,
    nkbool setupGlobalVariable,
    nkuint32_t argumentCount, ...)
{
    va_list args;
    NKVMExternalFunctionID id = nkxVmRegisterExternalFunction(vm, name, func);
    struct NKVMExternalFunction *funcOb = NULL;

    if(id.id == NK_INVALID_VALUE) {
        return id;
    }

    if(cs && setupGlobalVariable) {
        nkxCompilerCreateCFunctionVariable(cs, name, func);
    }

    funcOb = &vm->externalFunctionTable[id.id];
    funcOb->argumentCount = argumentCount;

    if(argumentCount != NK_INVALID_VALUE) {

        nkiSetupExternalFunction_addArgs(vm,
            funcOb,
            argumentCount);

        if(funcOb->argTypes && funcOb->argExternalTypes) {

            nkuint32_t i;

            va_start(args, argumentCount);

            for(i = 0; i < argumentCount; i++) {

                enum NKValueType t = (enum NKValueType)va_arg(args, int);
                funcOb->argTypes[i] = t;

                if(t == NK_VALUETYPE_OBJECTID) {
                    NKVMExternalDataTypeID objectExternalTypeId;
                    objectExternalTypeId = va_arg(args, NKVMExternalDataTypeID);
                    funcOb->argExternalTypes[i] = objectExternalTypeId;
                }
            }

            va_end(args);
        }
    }

    return id;
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
    NKVMExternalObjectCleanupCallback cleanupCallback,
    NKVMExternalObjectGCMarkCallback gcMarkCallback)
{
    NKVMExternalDataTypeID ret = { NK_INVALID_VALUE };
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(ret);
    ret = nkiVmRegisterExternalType(
        vm, name, serializationCallback, cleanupCallback,
        gcMarkCallback);
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

nkbool nkxVmObjectSetExternalType(
    struct NKVM *vm,
    struct NKValue *object,
    NKVMExternalDataTypeID externalType)
{
    nkbool ret = nkfalse;
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(ret);
    ret = nkiVmObjectSetExternalType(vm, object, externalType);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
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

nkbool nkxVmObjectSetExternalData(
    struct NKVM *vm,
    struct NKValue *object,
    void *data)
{
    nkbool ret = nkfalse;
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(ret);
    ret = nkiVmObjectSetExternalData(vm, object, data);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
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

void nkxDbgDumpState(struct NKVM *vm, const char *script, FILE *stream)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiDbgDumpState(vm, script, stream);
    NK_CLEAR_FAILURE_RECOVERY();
}

void nkxAddError(
    struct NKVM *vm,
    const char *str)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiAddError(vm, str);
    NK_CLEAR_FAILURE_RECOVERY();
}

void nkxAddErrorEx(
    struct NKVM *vm,
    const char *str,
    const char *filename,
    nkuint32_t lineNumber)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    {
        nkuint32_t fileIndex = nkiVmAddSourceFile(vm, filename);
        nkiAddErrorEx(vm, lineNumber, fileIndex, str);
    }
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

void nkxValueSetNil(struct NKVM *vm, struct NKValue *value)
{
    value->type = NK_VALUETYPE_NIL;
    value->intData = 0;
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
            data->vm, dynStr->data);
        nkiDynStrDelete(dynStr);

        ret = nkfalse;

    } else {

        va_start(args, expectedArgumentCount);

        for(i = 0; i < expectedArgumentCount; i++) {

            enum NKValueType thisType = (enum NKValueType)va_arg(args, int);

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
                    data->vm, dynStr->data);
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
            vm, dynStr->data);
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
            data->vm, dynStr->data);
        nkiDynStrDelete(dynStr);

    } else if(data->arguments[argumentNumber].type != NK_VALUETYPE_OBJECTID) {

        struct NKDynString *dynStr = nkiDynStrCreate(
            data->vm, functionName);
        nkiDynStrAppend(dynStr, ": Tried to decode an external object that is not even an object for argument ");
        nkiDynStrAppendUint32(dynStr, argumentNumber);
        nkiDynStrAppend(dynStr, ".");
        nkiAddError(
            data->vm, dynStr->data);
        nkiDynStrDelete(dynStr);

    } else if(data->vm->externalTypeCount <= externalDataType.id) {

        struct NKDynString *dynStr = nkiDynStrCreate(
            data->vm, functionName);
        nkiDynStrAppend(dynStr, ": Argument ");
        nkiDynStrAppendUint32(dynStr, argumentNumber);
        nkiDynStrAppend(dynStr, ": Bad type specified in external function.");
        nkiAddError(
            data->vm, dynStr->data);
        nkiDynStrDelete(dynStr);

    } else if(nkxVmObjectGetExternalType(data->vm, &data->arguments[argumentNumber]).id != externalDataType.id) {

        struct NKDynString *dynStr = nkiDynStrCreate(
            data->vm, functionName);
        nkiDynStrAppend(dynStr, ": Argument ");
        nkiDynStrAppendUint32(dynStr, argumentNumber);
        nkiDynStrAppend(dynStr, ": Type mismatch. Expected ");
        nkiDynStrAppend(dynStr,
            data->vm->externalTypes ? data->vm->externalTypes[externalDataType.id].name : "badtype");
        nkiDynStrAppend(dynStr, ". Got ");
        nkiDynStrAppend(dynStr, nkiValueGetTypeNameOfValue(data->vm, &data->arguments[argumentNumber]));
        nkiDynStrAppend(dynStr, ".");
        nkiAddError(
            data->vm, dynStr->data);
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
        nkiAddError(vm, "Attempted to access serialization writer outside of serialization.");
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

void nkxVmGarbageCollect_markValue(
    struct NKVM *vm,
    struct NKVMGCState *gcState,
    struct NKValue *value)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiVmGarbageCollect_markValue(gcState, value);
    NK_CLEAR_FAILURE_RECOVERY();
}

void nkxVmObjectClearEntry(
    struct NKVM *vm,
    struct NKValue *objectId,
    struct NKValue *key)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiVmObjectClearEntry_public(vm, objectId, key);
    NK_CLEAR_FAILURE_RECOVERY();
}

struct NKValue *nkxVmObjectFindOrAddEntry(
    struct NKVM *vm,
    struct NKValue *objectId,
    struct NKValue *key,
    nkbool noAdd)
{
    struct NKValue *ret = NULL;
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(NULL);
    ret = nkiVmObjectFindOrAddEntry_public(vm, objectId, key, noAdd);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

nkuint32_t nkxVmObjectGetSize(
    struct NKVM *vm,
    struct NKValue *objectId)
{
    nkuint32_t ret = 0;
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(0);
    ret = nkiVmObjectGetSize(vm, objectId);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

void nkxValueSetFunction(struct NKVM *vm, struct NKValue *value, NKVMInternalFunctionID id)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiValueSetFunction(vm, value, id);
    NK_CLEAR_FAILURE_RECOVERY();
}

nkbool nkxVmProgramHasEnded(struct NKVM *vm)
{
    return vm->instructions[
        vm->currentExecutionContext->instructionPointer &
        vm->instructionAddressMask].opcode == NK_OP_END;
}

nkbool nkxVmHasAllocationFailure(struct NKVM *vm)
{
    return vm->errorState.allocationFailure;
}

nkuint32_t nkxVmGetCurrentMemoryUsage(struct NKVM *vm)
{
    return vm->currentMemoryUsage;
}

nkuint32_t nkxVmGetPeakMemoryUsage(struct NKVM *vm)
{
    return vm->peakMemoryUsage;
}

// ----------------------------------------------------------------------
// Memory stuff

void *nkxMalloc(
    struct NKVM *vm, nkuint32_t size)
{
    void *ret = NULL;
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(ret);
    ret = nkiMalloc(vm, size);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

void *nkxMallocArray(
    struct NKVM *vm, nkuint32_t size, nkuint32_t count)
{
    void *ret = NULL;
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(ret);
    ret = nkiMallocArray(vm, size, count);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

void nkxFree(struct NKVM *vm, void *data)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiFree(vm, data);
    NK_CLEAR_FAILURE_RECOVERY();
}

void *nkxRealloc(struct NKVM *vm, void *data, nkuint32_t size)
{
    void *ret = NULL;
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(ret);
    ret = nkiRealloc(vm, data, size);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

void *nkxReallocArray(
    struct NKVM *vm, void *data, nkuint32_t size, nkuint32_t count)
{
    void *ret = NULL;
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(ret);
    ret = nkiReallocArray(vm, data, size, count);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

char *nkxStrdup(struct NKVM *vm, const char *str)
{
    char *ret = NULL;
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(ret);
    ret = nkiStrdup(vm, str);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

// ----------------------------------------------------------------------
// "protected" implementation
//
// This is all stuff that technically exists on the public API side.

void nkxVmInitExecutionContext(
    struct NKVM *vm,
    struct NKVMExecutionContext *context)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiVmInitExecutionContext(vm, context);
    NK_CLEAR_FAILURE_RECOVERY();
}

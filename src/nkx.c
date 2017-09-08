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
        nkiVmDestroy(vm);
    }
    free(vm);
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

nkuint32_t nkxVmGetErrorCount(struct NKVM *vm)
{
    nkuint32_t ret;
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(1);
    ret = nkiVmGetErrorCount(vm);
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
    struct NKVM *vm, const char *name)
{
    // NK_FAILURE_RECOVERY_DECL();
    // struct NKValue *ret;
    // NK_SET_FAILURE_RECOVERY(NULL);
    // ret = nkiVmFindGlobalVariable(vm, name);
    // NK_CLEAR_FAILURE_RECOVERY();
    // return ret;
    return nkiVmFindGlobalVariable(vm, name);
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

    if(data->argumentCount != 1) {
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

void nkxVmObjectSetGarbageCollectionCallback(
    struct NKVM *vm,
    struct NKValue *object,
    NKVMExternalFunctionID callbackFunction)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiVmObjectSetGarbageCollectionCallback(vm, object, callbackFunction);
    NK_CLEAR_FAILURE_RECOVERY();
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
    struct NKVM *vm, const char *name)
{
    NKVMExternalDataTypeID ret = { NK_INVALID_VALUE };
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(ret);
    ret = nkiVmRegisterExternalType(vm, name);
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

void *nkxVmObjectGetExternalData(
    struct NKVM *vm,
    struct NKValue *object)
{
    void *ret = NULL;
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(ret);
    ret = nkiVmObjectGetExternalData(vm, object);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}


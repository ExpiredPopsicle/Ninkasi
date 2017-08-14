#include "nkcommon.h"
#include "nkx.h"

struct NKVM *nkxVmCreate(void)
{
    NK_FAILURE_RECOVERY_DECL();

    // FIXME: Use specified malloc replacement here.
    struct NKVM *vm = malloc(sizeof(struct NKVM));

    if(!vm) {
        return NULL;
    }

    memset(vm, 0, sizeof(*vm));

    NK_SET_FAILURE_RECOVERY(vm);

    nkiVmInit(vm);

    NK_CLEAR_FAILURE_RECOVERY();

    return vm;
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

void nkxVmCreateCFunction(
    struct NKVM *vm,
    VMFunctionCallback func,
    struct NKValue *output)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiVmCreateCFunction(vm, func, output);
    NK_CLEAR_FAILURE_RECOVERY();
}

struct NKValue *nkxVmFindGlobalVariable(
    struct NKVM *vm, const char *name)
{
    NK_FAILURE_RECOVERY_DECL();
    struct NKValue *ret;
    NK_SET_FAILURE_RECOVERY(NULL);
    ret = nkiVmFindGlobalVariable(vm, name);
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
    VMFunctionCallback func,
    void *userData)
{
    NK_FAILURE_RECOVERY_DECL();
    struct NKVM *vm = cs->vm;
    NK_SET_FAILURE_RECOVERY_VOID();
    nkiCompilerCreateCFunctionVariable(cs, name, func, userData);
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


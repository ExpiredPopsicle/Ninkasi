#include "common.h"
#include "nkx.h"

struct NKVM *nkxVmCreate(void)
{
    NK_FAILURE_RECOVERY_DECL();

    struct NKVM *vm = malloc(sizeof(struct NKVM));

    if(!vm) {
        return NULL;
    }

    memset(vm, 0, sizeof(*vm));

    NK_SET_FAILURE_RECOVERY(vm);

    vmInit(vm);

    NK_CLEAR_FAILURE_RECOVERY();

    return vm;
}

void nkxVmDelete(struct NKVM *vm)
{
    if(vm) {
        vmDestroy(vm);
    }
    free(vm);
}

bool nkxVmExecuteProgram(struct NKVM *vm)
{
    bool ret;
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(false);
    ret = vmExecuteProgram(vm);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

uint32_t nkxVmGetErrorCount(struct NKVM *vm)
{
    uint32_t ret;
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(1);
    ret = vmGetErrorCount(vm);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

void nkxVmIterate(struct NKVM *vm, uint32_t count)
{
    NK_FAILURE_RECOVERY_DECL();
    uint32_t i;

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

        vmIterate(vm);

    }
    NK_CLEAR_FAILURE_RECOVERY();
}

void nkxVmGarbageCollect(struct NKVM *vm)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    vmGarbageCollect(vm);
    NK_CLEAR_FAILURE_RECOVERY();
}

// FIXME: Add max iteration count param.
void nkxVmCallFunction(
    struct NKVM *vm,
    struct NKValue *functionValue,
    uint32_t argumentCount,
    struct NKValue *arguments,
    struct NKValue *returnValue)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    vmCallFunction(vm, functionValue, argumentCount, arguments, returnValue);
    NK_CLEAR_FAILURE_RECOVERY();
}

void nkxVmCreateCFunction(
    struct NKVM *vm,
    VMFunctionCallback func,
    struct NKValue *output)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    vmCreateCFunction(vm, func, output);
    NK_CLEAR_FAILURE_RECOVERY();
}

struct NKValue *nkxVmFindGlobalVariable(
    struct NKVM *vm, const char *name)
{
    NK_FAILURE_RECOVERY_DECL();
    struct NKValue *ret;
    NK_SET_FAILURE_RECOVERY(NULL);
    ret = vmFindGlobalVariable(vm, name);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

struct NKCompilerState *nkxVmCompilerCreate(
    struct NKVM *vm)
{
    NK_FAILURE_RECOVERY_DECL();
    struct NKCompilerState *ret;
    NK_SET_FAILURE_RECOVERY(NULL);
    ret = vmCompilerCreate(vm);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

void nkxVmCompilerCreateCFunctionVariable(
    struct NKCompilerState *cs,
    const char *name,
    VMFunctionCallback func,
    void *userData)
{
    NK_FAILURE_RECOVERY_DECL();
    struct NKVM *vm = cs->vm;
    NK_SET_FAILURE_RECOVERY_VOID();
    vmCompilerCreateCFunctionVariable(cs, name, func, userData);
    NK_CLEAR_FAILURE_RECOVERY();
}

bool nkxVmCompilerCompileScript(
    struct NKCompilerState *cs,
    const char *script)
{
    NK_FAILURE_RECOVERY_DECL();
    struct NKVM *vm = cs->vm;
    bool ret;
    NK_SET_FAILURE_RECOVERY(false);
    ret = vmCompilerCompileScript(cs, script);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

bool nkxVmCompilerCompileScriptFile(
    struct NKCompilerState *cs,
    const char *scriptFilename)
{
    struct NKVM *vm = cs->vm;
    FILE *in = fopen(scriptFilename, "rb");
    uint32_t len;
    char *buf;
    bool success;

    if(!in) {
        NK_FAILURE_RECOVERY_DECL();
        NK_SET_FAILURE_RECOVERY(false);
        nkiCompilerAddError(cs, "Cannot open script file.");
        NK_CLEAR_FAILURE_RECOVERY();
        return false;
    }

    fseek(in, 0, SEEK_END);
    len = ftell(in);
    fseek(in, 0, SEEK_SET);

    buf = malloc(len + 1);
    if(!buf) {
        fclose(in);
        nkiErrorStateSetAllocationFailFlag(vm);
        return false;
    }
    fread(buf, len, 1, in);
    buf[len] = 0;

    fclose(in);
    success = nkxVmCompilerCompileScript(cs, buf);
    free(buf);

    return success;
}

void nkxVmCompilerFinalize(
    struct NKCompilerState *cs)
{
    NK_FAILURE_RECOVERY_DECL();
    struct NKVM *vm = cs->vm;
    NK_SET_FAILURE_RECOVERY_VOID();
    vmCompilerFinalize(cs);
    NK_CLEAR_FAILURE_RECOVERY();
}

const char *nkxValueToString(struct NKVM *vm, struct NKValue *value)
{
    NK_FAILURE_RECOVERY_DECL();
    const char *ret = NULL;
    NK_SET_FAILURE_RECOVERY(NULL);
    ret = valueToString(vm, value);
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

bool nkxFunctionCallbackCheckArgCount(
    struct NKVMFunctionCallbackData *data,
    uint32_t argCount,
    const char *functionName)
{
    bool ret = true;
    struct NKVM *vm = data->vm;
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(false);

    if(data->argumentCount != 1) {
        struct NKDynString *dynStr = nkiDynStrCreate(
            data->vm, "Bad argument count in ");
        nkiDynStrAppend(dynStr, functionName);
        nkiAddError(
            data->vm, -1, dynStr->data);
        nkiDynStrDelete(dynStr);
        ret = false;
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


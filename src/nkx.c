#include "common.h"
#include "nkx.h"

struct VM *nkxVmCreate(void)
{
    NK_FAILURE_RECOVERY_DECL();

    struct VM *vm = malloc(sizeof(struct VM));

    if(!vm) {
        return NULL;
    }

    memset(vm, 0, sizeof(*vm));

    NK_SET_FAILURE_RECOVERY(vm);

    vmInit(vm);

    NK_CLEAR_FAILURE_RECOVERY();

    return vm;
}

void nkxVmDelete(struct VM *vm)
{
    if(vm) {
        vmDestroy(vm);
    }
    free(vm);
}

bool nkxVmExecuteProgram(struct VM *vm)
{
    bool ret;
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(false);
    ret = vmExecuteProgram(vm);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

uint32_t nkxVmGetErrorCount(struct VM *vm)
{
    uint32_t ret;
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY(1);
    ret = vmGetErrorCount(vm);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

void nkxVmIterate(struct VM *vm, uint32_t count)
{
    NK_FAILURE_RECOVERY_DECL();
    uint32_t i;

    NK_SET_FAILURE_RECOVERY_VOID();
    for(i = 0; i < count; i++) {

        // Check for end-of-program.
        if(vm->instructions[
                vm->instructionPointer &
                vm->instructionAddressMask].opcode == OP_END)
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

void nkxVmGarbageCollect(struct VM *vm)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    vmGarbageCollect(vm);
    NK_CLEAR_FAILURE_RECOVERY();
}

// FIXME: Add max iteration count param.
void nkxVmCallFunction(
    struct VM *vm,
    struct Value *functionValue,
    uint32_t argumentCount,
    struct Value *arguments,
    struct Value *returnValue)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    vmCallFunction(vm, functionValue, argumentCount, arguments, returnValue);
    NK_CLEAR_FAILURE_RECOVERY();
}

void nkxVmCreateCFunction(
    struct VM *vm,
    VMFunctionCallback func,
    struct Value *output)
{
    NK_FAILURE_RECOVERY_DECL();
    NK_SET_FAILURE_RECOVERY_VOID();
    vmCreateCFunction(vm, func, output);
    NK_CLEAR_FAILURE_RECOVERY();
}

struct Value *nkxVmFindGlobalVariable(
    struct VM *vm, const char *name)
{
    NK_FAILURE_RECOVERY_DECL();
    struct Value *ret;
    NK_SET_FAILURE_RECOVERY(NULL);
    ret = vmFindGlobalVariable(vm, name);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

struct CompilerState *nkxVmCompilerCreate(
    struct VM *vm)
{
    NK_FAILURE_RECOVERY_DECL();
    struct CompilerState *ret;
    NK_SET_FAILURE_RECOVERY(NULL);
    ret = vmCompilerCreate(vm);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

void nkxVmCompilerCreateCFunctionVariable(
    struct CompilerState *cs,
    const char *name,
    VMFunctionCallback func,
    void *userData)
{
    NK_FAILURE_RECOVERY_DECL();
    struct VM *vm = cs->vm;
    NK_SET_FAILURE_RECOVERY_VOID();
    vmCompilerCreateCFunctionVariable(cs, name, func, userData);
    NK_CLEAR_FAILURE_RECOVERY();
}

bool nkxVmCompilerCompileScript(
    struct CompilerState *cs,
    const char *script)
{
    NK_FAILURE_RECOVERY_DECL();
    struct VM *vm = cs->vm;
    bool ret;
    NK_SET_FAILURE_RECOVERY(false);
    ret = vmCompilerCompileScript(cs, script);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

bool nkxVmCompilerCompileScriptFile(
    struct CompilerState *cs,
    const char *scriptFilename)
{
    struct VM *vm = cs->vm;
    FILE *in = fopen(scriptFilename, "rb");
    uint32_t len;
    char *buf;
    bool success;

    if(!in) {
        NK_FAILURE_RECOVERY_DECL();
        NK_SET_FAILURE_RECOVERY(false);
        vmCompilerAddError(cs, "Cannot open script file.");
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
    struct CompilerState *cs)
{
    NK_FAILURE_RECOVERY_DECL();
    struct VM *vm = cs->vm;
    NK_SET_FAILURE_RECOVERY_VOID();
    vmCompilerFinalize(cs);
    NK_CLEAR_FAILURE_RECOVERY();
}

const char *nkxValueToString(struct VM *vm, struct Value *value)
{
    NK_FAILURE_RECOVERY_DECL();
    const char *ret = NULL;
    NK_SET_FAILURE_RECOVERY(NULL);
    ret = valueToString(vm, value);
    NK_CLEAR_FAILURE_RECOVERY();
    return ret;
}

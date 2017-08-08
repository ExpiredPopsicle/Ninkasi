#include "public.h"
#include "nkx.h"

#include <assert.h>
#include <malloc.h>
#include <string.h>

char **splitLines(const char *str, uint32_t *lineCount)
{
    char **lines = NULL;

    uint32_t len = strlen(str);
    char *workStr = strdup(str);

    uint32_t i = 0;
    uint32_t lineStart = 0;

    *lineCount = 0;

    for(i = 0; i < len + 1; i++) {

        if(workStr[i] == '\n' || workStr[i] == 0) {
            workStr[i] = 0;

            (*lineCount)++;
            lines = realloc(lines, sizeof(char*) * (*lineCount));
            lines[(*lineCount)-1] = &workStr[lineStart];

            lineStart = i + 1;
        }
    }

    return lines;
}

void dumpListing(struct NKVM *vm, const char *script)
{
    uint32_t i;

  #if VM_DEBUG
    uint32_t lastLine = 0;
  #endif

    uint32_t lineCount = 0;
    char **lines = script ? splitLines(script, &lineCount) : NULL;

    if(vm->instructions) {
        for(i = 0; i <= vm->instructionAddressMask; i++) {

            enum NKOpcode opcode = vm->instructions[i].opcode;
            struct NKInstruction *maybeParams =
                &vm->instructions[(i+1) & vm->instructionAddressMask];

            if(opcode == NK_OP_END) {
                break;
            }

          #if VM_DEBUG

            // while(lastLine < vm->instructions[i].lineNumber && lastLine < lineCount) {
            //     printf("%4u     :                                         ; %s\n", lastLine, lines[lastLine]);
            //     lastLine++;
            // }

            // // Output opcode.
            // printf("%4u %.4u: %s", vm->instructions[i].lineNumber, i, vmGetOpcodeName(opcode));

            // Line-number-less version for diff.
            if(lines) {
                while(lastLine < vm->instructions[i].lineNumber && lastLine < lineCount) {
                    printf("                                         ; %s\n", lines[lastLine]);
                    lastLine++;
                }
            }
            printf("%s %.4d %s", (i == vm->instructionPointer ? ">" : " "), i, vmGetOpcodeName(opcode));

          #else

            // Output opcode.
            printf("%.4u: %s", i, vmGetOpcodeName(opcode));

          #endif

            // Output parameters.
            if(vm->instructions[i].opcode == NK_OP_PUSHLITERAL_INT) {
                i++;
                printf(" %d", maybeParams->opData_int);
            } else if(vm->instructions[i].opcode == NK_OP_PUSHLITERAL_FLOAT) {
                i++;
                printf(" %f", maybeParams->opData_float);
            } else if(vm->instructions[i].opcode == NK_OP_PUSHLITERAL_FUNCTIONID) {
                i++;
                printf(" %u", maybeParams->opData_functionId);
            } else if(vm->instructions[i].opcode == NK_OP_PUSHLITERAL_STRING) {
                const char *str = nkiVmStringTableGetStringById(&vm->stringTable, maybeParams->opData_string);
                i++;
                // TODO: Escape string before showing here.
                printf(" %d:\"%s\"", maybeParams->opData_string, str ? str : "<bad string>");
            }

            printf("\n");
        }
    }

    if(lines) {
        free(lines[0]);
        free(lines);
    }
}

char *loadScript(const char *filename)
{
    FILE *in = fopen(filename, "rb");
    uint32_t len;
    char *buf;

    if(!in) {
        return NULL;
    }

    fseek(in, 0, SEEK_END);
    len = ftell(in);
    fseek(in, 0, SEEK_SET);

    buf = malloc(len + 1);
    fread(buf, len, 1, in);
    buf[len] = 0;

    fclose(in);

    return buf;
}

void testVMFunc(struct NKVMFunctionCallbackData *data)
{
    // uint32_t i;
    printf("testVMFunc hit!\n");
    // for(i = 0; i < data->argumentCount; i++) {
    //     printf("Argument %d: %s\n", i,
    //         nkxValueToString(data->vm, &data->arguments[i]));
    // }

    data->returnValue.intData = 565656;

    if(data->argumentCount != 1) {
        nkiAddError(
            data->vm,
            -1, "Bad argument count in testVMFunc.");
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

    vmValueSetInt(
        data->vm,
        &data->returnValue,
        valueHash(data->vm, &data->arguments[0]));
}

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
    uint32_t i;

    for(i = 0; i < data->argumentCount; i++) {
        printf("\033[1m%s\033[0m", nkxValueToString(data->vm, &data->arguments[i]));
    }

    (*(int*)data->userData)++;
}


int main(int argc, char *argv[])
{
    char *script = loadScript("test.txt");
    int shitCounter = 0;
    uint32_t maxRam = 19880;
    uint32_t maxMaxRam = 1024*1024;
    maxRam = 60522;
    maxRam = 61818;
    maxRam = 89049;
    maxRam = 90490;
    maxRam = 115200;
    maxRam = 130000;
    maxRam = 1;
    // maxRam = 158000;
    maxRam = 15800000;
    maxMaxRam = maxRam + 100;

    while(strlen(script) && maxRam < maxMaxRam) // && maxRam < 512)
    {
        uint32_t instructionCountMax = 1024*1024*1024;
        struct NKVM *vm = nkxVmCreate();
        if(!vm) continue;
        if(vmGetErrorCount(vm)) {
            nkxVmDelete(vm);
            continue;
        }

        uint32_t lineCount = 0;
        char **lines = splitLines(script, &lineCount);
        assert(script);

        // vmInit(&vm);
        // vm->limits.maxStrings = 256;
        // vm->limits.maxStringLength = 10;
        // vm->limits.maxStacksize = 5;
        // vm->limits.maxObjects = 1;
        // vm->limits.maxFieldsPerObject = 2;
        vm->limits.maxAllocatedMemory = maxRam;

        {
            struct NKCompilerState *cs = nkxCompilerCreate(vm);
            if(cs) {
                nkxCompilerCreateCFunctionVariable(cs, "cfunc", testVMFunc, NULL);
                nkxCompilerCreateCFunctionVariable(cs, "catastrophe", testVMCatastrophe, NULL);
                nkxCompilerCreateCFunctionVariable(cs, "print", vmFuncPrint, &shitCounter);
                nkxCompilerCreateCFunctionVariable(cs, "hash", getHash, NULL);
                nkxCompilerCreateCFunctionVariable(cs, "hash2", getHash, NULL);
                nkxCompilerCreateCFunctionVariable(cs, "testHandle1", testHandle1, NULL);
                nkxCompilerCreateCFunctionVariable(cs, "testHandle2", testHandle2, NULL);
                // vmCompilerCompileScript(cs, script);
                nkxCompilerCompileScriptFile(cs, "test.txt");
                nkxCompilerFinalize(cs);
            }

            // Dump errors.
            if(vmGetErrorCount(vm)) {
                struct NKError *err = vm->errorState.firstError;
                while(err) {
                    printf("error: %s\n", err->errorText);
                    err = err->next;
                }
            }
        }

        if(!vm->errorState.firstError) {

            // printf("----------------------------------------------------------------------\n");
            // printf("  Original script\n");
            // printf("----------------------------------------------------------------------\n");

            // {
            //     uint32_t i;
            //     for(i = 0; i < lineCount; i++) {
            //         printf("%4u : %s\n", i, lines[i]);
            //     }
            // }

            // printf("----------------------------------------------------------------------\n");
            // printf("  Dump\n");
            // printf("----------------------------------------------------------------------\n");

            // dumpListing(vm, script);

            printf("----------------------------------------------------------------------\n");
            printf("  Execution\n");
            printf("----------------------------------------------------------------------\n");

            if(!vmGetErrorCount(vm)) {

                // vmExecuteProgram(vm);

                while(
                    vm->instructions[
                        vm->instructionPointer &
                        vm->instructionAddressMask].opcode != NK_OP_END &&
                    vm->instructions[
                        vm->instructionPointer &
                        vm->instructionAddressMask].opcode != NK_OP_NOP &&
                    instructionCountMax)
                {
                    nkxVmIterate(vm, 2000);

                    // printf("\n\n\n\n");
                    // printf("----------------------------------------------------------------------\n");
                    // printf("PC: %u\n", vm->instructionPointer);
                    // printf("Stack...\n");
                    // printf("----------------------------------------------------------------------\n");
                    // vmStackDump(vm);
                    // printf("\n");
                    // dumpListing(vm, script);
                    // getchar();

                    if(nkxVmGetErrorCount(vm)) {
                        break;
                    }

                    instructionCountMax--;
                }



                {
                    struct NKValue *v = nkxVmFindGlobalVariable(vm, "readMeFromC");
                    if(v) {
                        printf("Value found: %s\n", nkxValueToString(vm, v));
                    } else {
                        printf("Value NOT found.\n");
                    }
                }
            }

            // while(vm->instructions[vm->instructionPointer].opcode != NK_OP_END) {

            //     // printf("instruction %d: %d\n", vm->instructionPointer, vm->instructions[vm->instructionPointer].opcode);
            //     // printf("  %d\n", vm->instructions[vm->instructionPointer].opcode);
            //     vmIterate(vm);

            if(vm->errorState.firstError) {
                struct NKError *err = vm->errorState.firstError;
                while(err) {
                    printf("error: %s\n", err->errorText);
                    err = err->next;
                }
            }

            //     vmStackDump(vm);
            //     // nkxVmGarbageCollect(vm);

            //     printf("next at %d\n",
            //         vm->instructionPointer);
            //     printf("next instruction %d: %d = %s\n",
            //         vm->instructionPointer,
            //         vm->instructions[vm->instructionPointer].opcode,
            //         vmGetOpcodeName(vm->instructions[vm->instructionPointer].opcode));
            // }

            // // Function call test.
            // if(!vm->errorState.firstError) {
            //     struct NKValue retVal;
            //     memset(&retVal, 0, sizeof(retVal));
            //     vmCallFunctionByName(&cs, "callMeFromC", 0, NULL, &retVal);
            // }
        }

        printf("----------------------------------------------------------------------\n");
        printf("  Finish\n");
        printf("----------------------------------------------------------------------\n");

        printf("Final stack...\n");
        vmStackDump(vm);
        nkxVmGarbageCollect(vm);
        printf("Final stack again...\n");
        vmStackDump(vm);

        if(0) {

            // printf("----------------------------------------------------------------------\n");
            // printf("  String table crap\n");
            // printf("----------------------------------------------------------------------\n");

            // nkiVmStringTableFindOrAddString(
            //     vm->stringTable, "sadf");
            // nkiVmStringTableFindOrAddString(
            //     vm->stringTable, "sadf");
            // nkiVmStringTableFindOrAddString(
            //     vm->stringTable, "sadf");
            // nkiVmStringTableFindOrAddString(
            //     vm->stringTable, "sadf");
            // nkiVmStringTableFindOrAddString(
            //     vm->stringTable, "sadf");

            // nkiVmStringTableFindOrAddString(
            //     vm->stringTable, "bladgh");
            // nkiVmStringTableFindOrAddString(
            //     vm->stringTable, "foom");
            // nkiVmStringTableFindOrAddString(
            //     vm->stringTable, "dicks");
            // nkiVmStringTableFindOrAddString(
            //     vm->stringTable, "sadf");

            // nkiVmStringTableDump(vm->stringTable);

            // nkiVmStringTableGetEntryById(
            //     vm->stringTable,
            //     nkiVmStringTableFindOrAddString(vm->stringTable, "sadf"))->lastGCPass = 1234;

            // nkiVmStringTableCleanOldStrings(vm->stringTable, 1234);

            // nkiVmStringTableDump(vm->stringTable);

            // vmRescanProgramStrings(vm);


            // vmIterate(vm);
            // vmIterate(vm);
            // vmIterate(vm);
            // vmIterate(vm);
            printf("Final stack dump...\n");
            vmStackDump(vm);

        }

        nkxVmGarbageCollect(vm);
        printf("Final object table dump...\n");
        vmObjectTableDump(vm);

        printf("Peak memory usage:    %u\n", vm->peakMemoryUsage);
        printf("Current memory usage: %u\n", vm->currentMemoryUsage);

        // vmDestroy(&vm);
        nkxVmDelete(vm);

        // printf("Post-cleanup memory usage: %u\n", vm->currentMemoryUsage);

        free(lines[0]);
        free(lines);



        // script[strlen(script) - 1] = 0;
        maxRam++;
        printf("maxRam: %u\n", maxRam);

        // fprintf(stderr, "Iterations: %u\n", (uint32_t)strlen(script));
        fprintf(stderr, "maxRam: %u\n", (uint32_t)maxRam);
    }

    free(script);

    printf("Shitcounter: %d\n", shitCounter);

    return 0;
}


#include "public.h"

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

void dumpListing(struct VM *vm, const char *script)
{
    uint32_t i;

  #if VM_DEBUG
    uint32_t lastLine = 0;
  #endif

    uint32_t lineCount = 0;
    char **lines = script ? splitLines(script, &lineCount) : NULL;

    if(vm->instructions) {
        for(i = 0; i <= vm->instructionAddressMask; i++) {

            enum Opcode opcode = vm->instructions[i].opcode;
            struct Instruction *maybeParams =
                &vm->instructions[(i+1) & vm->instructionAddressMask];

            if(opcode == OP_END) {
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
            if(vm->instructions[i].opcode == OP_PUSHLITERAL_INT) {
                i++;
                printf(" %d", maybeParams->opData_int);
            } else if(vm->instructions[i].opcode == OP_PUSHLITERAL_FLOAT) {
                i++;
                printf(" %f", maybeParams->opData_float);
            } else if(vm->instructions[i].opcode == OP_PUSHLITERAL_FUNCTIONID) {
                i++;
                printf(" %u", maybeParams->opData_functionId);
            } else if(vm->instructions[i].opcode == OP_PUSHLITERAL_STRING) {
                const char *str = vmStringTableGetStringById(&vm->stringTable, maybeParams->opData_string);
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

void testVMFunc(struct VMFunctionCallbackData *data)
{
    // uint32_t i;
    printf("testVMFunc hit!\n");
    // for(i = 0; i < data->argumentCount; i++) {
    //     printf("Argument %d: %s\n", i,
    //         valueToString(data->vm, &data->arguments[i]));
    // }

    data->returnValue.intData = 565656;

    if(data->argumentCount != 1) {
        errorStateAddError(&data->vm->errorState, -1, "Bad argument count in testVMFunc.");
        return;
    }

    vmCallFunction(data->vm, &data->arguments[0], 0, NULL, &data->returnValue);

    printf("Got data back from VM: %s\n", valueToString(data->vm, &data->returnValue));
}

void getHash(struct VMFunctionCallbackData *data)
{
    if(!vmFunctionCallbackCheckArgCount(data, 1, "getHash")) return;

    vmValueSetInt(
        data->vm,
        &data->returnValue,
        valueHash(data->vm, &data->arguments[0]));
}

void testHandle1(struct VMFunctionCallbackData *data)
{
    if(!vmFunctionCallbackCheckArgCount(data, 1, "testHandle1")) return;

    vmObjectAcquireHandle(data->vm, &data->arguments[0]);
}

void testHandle2(struct VMFunctionCallbackData *data)
{
    if(!vmFunctionCallbackCheckArgCount(data, 1, "testHandle2")) return;

    vmObjectReleaseHandle(data->vm, &data->arguments[0]);
}


void vmFuncPrint(struct VMFunctionCallbackData *data)
{
    uint32_t i;

    for(i = 0; i < data->argumentCount; i++) {
        printf("\033[1m%s\033[0m", valueToString(data->vm, &data->arguments[i]));
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
    maxRam = 158000;

    while(strlen(script) && maxRam < maxMaxRam) // && maxRam < 512)
    {
        uint32_t instructionCountMax = 1024*1024*1024;
        struct VM vm;

        uint32_t lineCount = 0;
        char **lines = splitLines(script, &lineCount);
        assert(script);

        vmInit(&vm);
        // vm.limits.maxStrings = 256;
        // vm.limits.maxStringLength = 10;
        // vm.limits.maxStacksize = 5;
        // vm.limits.maxObjects = 1;
        // vm.limits.maxFieldsPerObject = 2;
        vm.limits.maxAllocatedMemory = maxRam;

        {
            struct CompilerState *cs = vmCompilerCreate(&vm);
            if(cs) {
                vmCompilerCreateCFunctionVariable(cs, "cfunc", testVMFunc, NULL);
                vmCompilerCreateCFunctionVariable(cs, "print", vmFuncPrint, &shitCounter);
                vmCompilerCreateCFunctionVariable(cs, "hash", getHash, NULL);
                vmCompilerCreateCFunctionVariable(cs, "hash2", getHash, NULL);
                vmCompilerCreateCFunctionVariable(cs, "testHandle1", testHandle1, NULL);
                vmCompilerCreateCFunctionVariable(cs, "testHandle2", testHandle2, NULL);
                vmCompilerCompileScript(cs, script);
                vmCompilerFinalize(cs);
            }

            // Dump errors.
            if(vmGetErrorCount(&vm)) {
                struct Error *err = vm.errorState.firstError;
                while(err) {
                    printf("error: %s\n", err->errorText);
                    err = err->next;
                }
            }
        }

        if(!vm.errorState.firstError) {

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

            // dumpListing(&vm, script);

            printf("----------------------------------------------------------------------\n");
            printf("  Execution\n");
            printf("----------------------------------------------------------------------\n");

            if(!vmGetErrorCount(&vm)) {

                // vmExecuteProgram(&vm);

                while(
                    vm.instructions[
                        vm.instructionPointer &
                        vm.instructionAddressMask].opcode != OP_END &&
                    vm.instructions[
                        vm.instructionPointer &
                        vm.instructionAddressMask].opcode != OP_NOP &&
                    instructionCountMax)
                {
                    vmIterate(&vm);

                    // printf("\n\n\n\n");
                    // printf("----------------------------------------------------------------------\n");
                    // printf("PC: %u\n", vm.instructionPointer);
                    // printf("Stack...\n");
                    // printf("----------------------------------------------------------------------\n");
                    // vmStackDump(&vm);
                    // printf("\n");
                    // dumpListing(&vm, script);
                    // getchar();

                    if(vm.errorState.firstError) {
                        break;
                    }

                    instructionCountMax--;
                }



                {
                    struct Value *v = vmFindGlobalVariable(&vm, "readMeFromC");
                    if(v) {
                        printf("Value found: %s\n", valueToString(&vm, v));
                    } else {
                        printf("Value NOT found.\n");
                    }
                }
            }

            // while(vm.instructions[vm.instructionPointer].opcode != OP_END) {

            //     // printf("instruction %d: %d\n", vm.instructionPointer, vm.instructions[vm.instructionPointer].opcode);
            //     // printf("  %d\n", vm.instructions[vm.instructionPointer].opcode);
            //     vmIterate(&vm);

            if(vm.errorState.firstError) {
                struct Error *err = vm.errorState.firstError;
                while(err) {
                    printf("error: %s\n", err->errorText);
                    err = err->next;
                }
            }

            //     vmStackDump(&vm);
            //     // vmGarbageCollect(&vm);

            //     printf("next at %d\n",
            //         vm.instructionPointer);
            //     printf("next instruction %d: %d = %s\n",
            //         vm.instructionPointer,
            //         vm.instructions[vm.instructionPointer].opcode,
            //         vmGetOpcodeName(vm.instructions[vm.instructionPointer].opcode));
            // }

            // // Function call test.
            // if(!vm.errorState.firstError) {
            //     struct Value retVal;
            //     memset(&retVal, 0, sizeof(retVal));
            //     vmCallFunctionByName(&cs, "callMeFromC", 0, NULL, &retVal);
            // }
        }

        printf("----------------------------------------------------------------------\n");
        printf("  Finish\n");
        printf("----------------------------------------------------------------------\n");

        printf("Final stack...\n");
        vmStackDump(&vm);
        vmGarbageCollect(&vm);
        printf("Final stack again...\n");
        vmStackDump(&vm);

        if(0) {

            // printf("----------------------------------------------------------------------\n");
            // printf("  String table crap\n");
            // printf("----------------------------------------------------------------------\n");

            // vmStringTableFindOrAddString(
            //     &vm.stringTable, "sadf");
            // vmStringTableFindOrAddString(
            //     &vm.stringTable, "sadf");
            // vmStringTableFindOrAddString(
            //     &vm.stringTable, "sadf");
            // vmStringTableFindOrAddString(
            //     &vm.stringTable, "sadf");
            // vmStringTableFindOrAddString(
            //     &vm.stringTable, "sadf");

            // vmStringTableFindOrAddString(
            //     &vm.stringTable, "bladgh");
            // vmStringTableFindOrAddString(
            //     &vm.stringTable, "foom");
            // vmStringTableFindOrAddString(
            //     &vm.stringTable, "dicks");
            // vmStringTableFindOrAddString(
            //     &vm.stringTable, "sadf");

            // vmStringTableDump(&vm.stringTable);

            // vmStringTableGetEntryById(
            //     &vm.stringTable,
            //     vmStringTableFindOrAddString(&vm.stringTable, "sadf"))->lastGCPass = 1234;

            // vmStringTableCleanOldStrings(&vm.stringTable, 1234);

            // vmStringTableDump(&vm.stringTable);

            // vmRescanProgramStrings(&vm);


            // vmIterate(&vm);
            // vmIterate(&vm);
            // vmIterate(&vm);
            // vmIterate(&vm);
            printf("Final stack dump...\n");
            vmStackDump(&vm);

        }

        vmGarbageCollect(&vm);
        printf("Final object table dump...\n");
        vmObjectTableDump(&vm);

        printf("Peak memory usage:    %u\n", vm.peakMemoryUsage);
        printf("Current memory usage: %u\n", vm.currentMemoryUsage);

        vmDestroy(&vm);

        printf("Post-cleanup memory usage: %u\n", vm.currentMemoryUsage);

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


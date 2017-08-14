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

#include "nkx.h"

// FIXME: Get rid of this.
#include "nkvm.h"

#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>

char **splitLines(const char *str, nkuint32_t *lineCount)
{
    char **lines = NULL;

    nkuint32_t len = strlen(str);
    char *workStr = strdup(str);

    nkuint32_t i = 0;
    nkuint32_t lineStart = 0;

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
    nkuint32_t i;

  #if NK_VM_DEBUG
    nkuint32_t lastLine = 0;
  #endif

    nkuint32_t lineCount = 0;
    char **lines = script ? splitLines(script, &lineCount) : NULL;

    if(vm->instructions) {
        for(i = 0; i <= vm->instructionAddressMask; i++) {

            enum NKOpcode opcode = vm->instructions[i].opcode;
            struct NKInstruction *maybeParams =
                &vm->instructions[(i+1) & vm->instructionAddressMask];

            if(opcode == NK_OP_END) {
                break;
            }

          #if NK_VM_DEBUG

            // while(lastLine < vm->instructions[i].lineNumber && lastLine < lineCount) {
            //     printf("%4u     :                                         ; %s\n", lastLine, lines[lastLine]);
            //     lastLine++;
            // }

            // // Output opcode.
            // printf("%4u %.4u: %s", vm->instructions[i].lineNumber, i, nkiVmGetOpcodeName(opcode));

            // Line-number-less version for diff.
            if(lines) {
                while(lastLine < vm->instructions[i].lineNumber && lastLine < lineCount) {
                    printf("                                         ; %s\n", lines[lastLine]);
                    lastLine++;
                }
            }
            printf("%s %.4d %s", (i == vm->instructionPointer ? ">" : " "), i, nkiVmGetOpcodeName(opcode));

          #else

            // Output opcode.
            printf("%.4u: %s", i, nkiVmGetOpcodeName(opcode));

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
    nkuint32_t len;
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
    // nkuint32_t i;
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

    nkiValueSetInt(
        data->vm,
        &data->returnValue,
        nkiValueHash(data->vm, &data->arguments[0]));
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
    nkuint32_t i;

    for(i = 0; i < data->argumentCount; i++) {
        // printf("\033[1m%s\033[0m", nkxValueToString(data->vm, &data->arguments[i]));
        printf("%s", nkxValueToString(data->vm, &data->arguments[i]));
    }

    (*(int*)data->userData)++;
}


int main(int argc, char *argv[])
{
    char *script = loadScript("test.txt");
    int shitCounter = 0;
    nkuint32_t maxRam = 19880;
    nkuint32_t maxMaxRam = (nkuint32_t)1024*(nkuint32_t)1024;
    maxRam = 60522;
    maxRam = 61818;
    maxRam = 89049;
    maxRam = 90490;
    maxRam = 115200;
    maxRam = 130000;
    maxRam = 1;
    maxRam = 158000;
    // maxRam = 15800000;
    maxMaxRam = maxRam + 1;

    if(!script) {
        printf("Script failed to even load.\n");
    }

    while(strlen(script) && maxRam < maxMaxRam) // && maxRam < 512)
    {
        nkuint32_t lineCount = 0;
        char **lines = NULL;

        nkuint32_t instructionCountMax = (nkuint32_t)1024*(nkuint32_t)1024*(nkuint32_t)1024;
        struct NKVM *vm = nkxVmCreate();
        if(!vm) continue;
        if(nkiVmGetErrorCount(vm)) {
            nkxVmDelete(vm);
            continue;
        }

        lineCount = 0;
        lines = splitLines(script, &lineCount);
        assert(script);

        // nkiVmInit(&vm);
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
                // nkiCompilerCompileScript(cs, script);
                nkxCompilerCompileScriptFile(cs, "test.txt");
                nkxCompilerFinalize(cs);
            }

            // Dump errors.
            if(nkiVmGetErrorCount(vm)) {
                struct NKError *err = vm->errorState.firstError;
                while(err) {
                    printf("error: %s\n", err->errorText);
                    err = err->next;
                }
            }
        }

        if(!nkxVmHasErrors(vm)) {

            // printf("----------------------------------------------------------------------\n");
            // printf("  Original script\n");
            // printf("----------------------------------------------------------------------\n");

            // {
            //     nkuint32_t i;
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

            if(!nkiVmGetErrorCount(vm)) {

                // nkiVmExecuteProgram(vm);

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
                    // nkiVmStackDump(vm);
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
            //     nkiVmIterate(vm);

            if(nkxVmHasErrors(vm)) {
                struct NKError *err = vm->errorState.firstError;
                printf("Errors detected...\n");
                while(err) {
                    printf("  error: %s\n", err->errorText);
                    err = err->next;
                }
            }

            //     nkiVmStackDump(vm);
            //     // nkxVmGarbageCollect(vm);

            //     printf("next at %d\n",
            //         vm->instructionPointer);
            //     printf("next instruction %d: %d = %s\n",
            //         vm->instructionPointer,
            //         vm->instructions[vm->instructionPointer].opcode,
            //         nkiVmGetOpcodeName(vm->instructions[vm->instructionPointer].opcode));
            // }

            // // Function call test.
            // if(!vm->errorState.firstError) {
            //     struct NKValue retVal;
            //     memset(&retVal, 0, sizeof(retVal));
            //     nkiVmCallFunctionByName(&cs, "callMeFromC", 0, NULL, &retVal);
            // }

        } else {

            printf("*** AN ERROR WAS DETECTED ***\n");

        }

        assert(!vm->catastrophicFailureJmpBuf);

        printf("----------------------------------------------------------------------\n");
        printf("  Finish\n");
        printf("----------------------------------------------------------------------\n");

        printf("Final stack...\n");
        nkiVmStackDump(vm);
        nkxVmGarbageCollect(vm);
        printf("Final stack again...\n");
        nkiVmStackDump(vm);

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

            // nkiVmRescanProgramStrings(vm);


            // nkiVmIterate(vm);
            // nkiVmIterate(vm);
            // nkiVmIterate(vm);
            // nkiVmIterate(vm);
            printf("Final stack dump...\n");
            nkiVmStackDump(vm);

        }

        nkxVmGarbageCollect(vm);
        printf("Final object table dump...\n");
        nkiVmObjectTableDump(vm);

        printf("Peak memory usage:    " NK_PRINTF_UINT32 "\n", vm->peakMemoryUsage);
        printf("Current memory usage: " NK_PRINTF_UINT32 "\n", vm->currentMemoryUsage);

        // nkiVmDestroy(&vm);
        nkxVmDelete(vm);

        // printf("Post-cleanup memory usage: %u\n", vm->currentMemoryUsage);

        free(lines[0]);
        free(lines);



        // script[strlen(script) - 1] = 0;
        maxRam++;
        printf("maxRam: " NK_PRINTF_UINT32 "\n", maxRam);

        // fprintf(stderr, "Iterations: %u\n", (nkuint32_t)strlen(script));
        fprintf(stderr, "maxRam: " NK_PRINTF_UINT32 "\n", (nkuint32_t)maxRam);
    }

    free(script);

    printf("Shitcounter: %d\n", shitCounter);



    return 0;
}


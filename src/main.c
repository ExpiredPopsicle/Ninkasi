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

// FIXME: Get rid of this.
#include "nkdbg.h"

#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

// ----------------------------------------------------------------------

struct WriterTestBuffer
{
    char *data;
    nkuint32_t readPtr;
    nkuint32_t size;
};

nkbool writerTest(void *data, nkuint32_t size, void *userdata, nkbool writeMode)
{
    if(userdata) {

        struct WriterTestBuffer *testBuf = (struct WriterTestBuffer *)userdata;

        if(writeMode) {

            nkuint32_t oldSize = testBuf->size;
            testBuf->size += size;
            testBuf->data = realloc(testBuf->data, testBuf->size);
            memcpy(testBuf->data + oldSize, data, size);

        } else {

            if(testBuf->readPtr + size <= testBuf->size) {
                memcpy(data, testBuf->data + testBuf->readPtr, size);
                testBuf->readPtr += size;
            } else {
                printf("END OF BUFFER!\n");
                return nkfalse;
            }
        }
    }

    // {
    //     const char charMap[17] = { "0123456789abcdef" };
    //     nkuint32_t i = 0;
    //     for(i = 0; i < size; i++) {
    //         const char *c = (const char *)data + i;
    //         printf("%c%c", charMap[(*c & 0xf0) >> 4], charMap[*c & 0xf]);
    //     }
    //     printf("\n");
    // }

    return nktrue;
}

// ----------------------------------------------------------------------




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
                printf(" %u", maybeParams->opData_functionId.id);
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

char *loadScript(const char *filename, nkuint32_t *scriptSize)
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

    if(scriptSize) {
        *scriptSize = len;
    }

    return buf;
}

char *loadScriptFromStdin(nkuint32_t *scriptSize)
{
    size_t bufSize = 256;
    size_t scriptLen = 0;
    char *buf = malloc(bufSize);
    while(fread(&buf[scriptLen], 1, 1, stdin) > 0) {
        scriptLen++;
        buf[scriptLen] = 0;
        if(scriptLen + 1 >= bufSize) {
            bufSize <<= 1;
            buf = realloc(buf, bufSize);
        }
    }
    if(scriptSize) {
        *scriptSize = scriptLen;
    }
    return buf;
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
}

// This should probably be tracked separately for each VM.
static NKVMExternalFunctionID doGCCallbackThing_id;

void doGCCallbackThing(struct NKVMFunctionCallbackData *data)
{
    // TODO: Add standard argument count handler.
    if(data->argumentCount != 1) {
        nkiAddError(
            data->vm,
            -1, "Bad argument count in doGCCallbackThing.");
        return;
    }

    assert(data->argumentCount == 1);
    printf("GC callback called.\n");
    {
        NKVMExternalDataTypeID id = nkxVmObjectGetExternalType(data->vm, &data->arguments[0]);
        printf("GCing external of type %s\n", nkxVmGetExternalTypeName(data->vm, id));
    }

    {
        char *externalData = nkxVmObjectGetExternalData(data->vm, &data->arguments[0]);
        printf("GCing external data: %s\n", externalData);
        free(externalData);
    }
}

void doSerializationCallbackThing(struct NKVMFunctionCallbackData *data)
{
    // TODO: Make this standard, or find a way to make sure
    // serialization callbacks aren't called by a normal program.
    if(!data->vm->serializationState.writer) {
        return;
    }
    printf("\nSerialization callback start!\n");
    {
        char tmp[5] = "TEST";
        data->vm->serializationState.writer(tmp, 4, data->vm->serializationState.userdata, data->vm->serializationState.writeMode);
        data->vm->serializationState.writer(tmp, 4, data->vm->serializationState.userdata, data->vm->serializationState.writeMode);
        data->vm->serializationState.writer(tmp, 4, data->vm->serializationState.userdata, data->vm->serializationState.writeMode);
    }
    printf("\nSerialization callback complete!\n");
}

void setGCCallbackThing(struct NKVMFunctionCallbackData *data)
{
    if(data->argumentCount != 1) {
        nkiAddError(
            data->vm,
            -1, "Bad argument count in setGCCallbackThing.");
        return;
    }

    assert(data->argumentCount == 1);
    printf("Setting GC callback.\n");

    doGCCallbackThing_id =
        nkxVmRegisterExternalFunction(data->vm, "doGCCallbackThing", doGCCallbackThing);

    nkxVmObjectSetGarbageCollectionCallback(
        data->vm, &data->arguments[0], doGCCallbackThing_id);

    nkxVmObjectSetExternalData(data->vm, &data->arguments[0], strdup("butts"));


    {
        NKVMExternalFunctionID serializationCallbackId = nkxVmRegisterExternalFunction(
            data->vm, "doSerializationCallbackThing", doSerializationCallbackThing);
        nkxVmObjectSetSerializationCallback(
            data->vm, &data->arguments[0], serializationCallbackId);
    }

    {
        NKVMExternalDataTypeID id = nkxVmRegisterExternalType(data->vm, "footype");
        printf("Registered external type %s\n", nkxVmGetExternalTypeName(data->vm, id));
        nkxVmObjectSetExternalType(data->vm, &data->arguments[0], id);
    }
}

struct Settings
{
    nkbool compileOnly;
    const char *filename;
    struct NKVMLimits limits;
};

void printHelp(nkbool isError)
{
    FILE *stream = isError ? stderr : stdout;
    fprintf(stream, "<TODO: help text goes here>\n");
}

nkbool parseCmdLine(int argc, char *argv[], struct Settings *settings)
{
    int i;
    nkbool noMoreSwitches = nkfalse;

    // Set up some nice defaults.
    memset(settings, 0, sizeof(*settings));
    memset(&settings->limits, 0xff, sizeof(settings->limits));

    for(i = 1; i < argc; i++) {

        if(strcmp("-c", argv[i]) == 0) {

            settings->compileOnly = nktrue;

        } else if(strcmp("-m", argv[i]) == 0) {

            i++;
            if(i < argc) {
                settings->limits.maxAllocatedMemory = atoi(argv[i]);
            } else {
                fprintf(stderr, "Missing parameter for -m.\n");
                return nkfalse;
            }

        } else if(strcmp("--help", argv[i]) == 0) {

            printHelp(nkfalse);
            exit(0);

        } else if(strcmp("--", argv[i]) == 0) {

            noMoreSwitches = nktrue;

        } else if(noMoreSwitches || (argv[i][0] != '-' || strcmp(argv[i], "-") == 0)) {

            if(settings->filename) {
                fprintf(stderr, "Too many filenames specified.\n");
                return nkfalse;
            }

            settings->filename = argv[i];

        } else {

            fprintf(stderr, "Unrecognized argument: %s\n", argv[i]);
            fprintf(stderr, "Use '%s --help' for more information.\n", argv[0]);
            return nkfalse;

        }
    }

    return nktrue;
}

void initInternalFunctions(struct NKVM *vm, struct NKCompilerState *cs)
{
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

int main(int argc, char *argv[])
{
    struct Settings settings;
    char *script = NULL;
    nkuint32_t scriptSize = 0;

    if(!parseCmdLine(argc, argv, &settings)) {
        return 1;
    }

    // Load script file.
    if(settings.filename) {
        if(strcmp(settings.filename, "-") == 0) {
            script = loadScriptFromStdin(&scriptSize);
        } else {
            script = loadScript(settings.filename, &scriptSize);
        }
    } else {
        fprintf(stderr, "No input file specified.\n");
        return 1;
    }
    if(!script) {
        fprintf(stderr, "Script failed to even load.\n");
        return 1;
    }

    {
        nkuint32_t lineCount = 0;
        char **lines = NULL;

        nkuint32_t instructionCountMax = (nkuint32_t)1024*(nkuint32_t)1024; // *(nkuint32_t)1024;
        struct NKVM *vm = nkxVmCreate();
        if(!vm) {
            assert(0);
            return 1;
        }
        if(nkiGetErrorCount(vm)) {
            nkxVmDelete(vm);
            assert(0);
            return 1;
        }

        lineCount = 0;
        lines = splitLines(script, &lineCount);
        assert(script);

        vm->limits = settings.limits;

        initInternalFunctions(vm, NULL);

        if(script && script[0] != 0) {

            struct NKCompilerState *cs = nkxCompilerCreate(vm);

            if(cs) {

                initInternalFunctions(vm, cs);
                nkxCompilerCompileScript(cs, script);
                nkxCompilerFinalize(cs);

            } else {

                fprintf(stderr, "Can't create compiler state.\n");

                return 1;
            }

            // Dump errors.
            if(nkiGetErrorCount(vm)) {
                struct NKError *err = vm->errorState.firstError;
                while(err) {
                    printf("error: %s\n", err->errorText);
                    err = err->next;
                }
            }

        } else {

            struct WriterTestBuffer buf;
            buf.data = script;
            buf.readPtr = 0;
            buf.size = scriptSize;

            if(!nkxVmSerialize(vm, writerTest, &buf, nkfalse)) {
                printf("Deserialization fail.\n");
                return 1;
            }
        }

        if(settings.compileOnly) {

            FILE *out = NULL;
            struct WriterTestBuffer buf;
            char *outputFilename = NULL;

            if(settings.filename) {

                char *newFilename = NULL;
                const char *oldFilename =
                    settings.filename ? settings.filename :
                    "stdin.nks";

                // Attempt to determine the extension by starting from
                // the end of the string and searching backwards for
                // '.'.
                nkint32_t i;
                for(i = strlen(oldFilename) - 1; i >= 0; i--) {
                    if(oldFilename[i] == '.') {
                        break;
                    }
                }

                // Make a new name with the correct extension at the
                // end.
                if(i >= 0) {
                    newFilename = malloc(i + 4 + 1);
                    memcpy(newFilename, oldFilename, i);
                    newFilename[i] = 0;
                    strcat(newFilename, ".nkb");
                    outputFilename = newFilename;
                }
            }

            // If our input is another nkb file, then I guess we
            // should do file.nkb.nkb. I don't know why you'd try to
            // recompile a compiled binary outside of testing, though.
            if(!strcmp(outputFilename, settings.filename)) {
                outputFilename = realloc(outputFilename, strlen(outputFilename) + 1 + 4);
                strcat(outputFilename, ".nkb");
            }

            if(!outputFilename) {
                fprintf(stderr, "Cannot determine output file for %s.\n", settings.filename);
                return 1;
            }

            buf.data = NULL;
            buf.readPtr = 0;
            buf.size = 0;

            nkxVmSerialize(vm, writerTest, &buf, nktrue);

            out = fopen(outputFilename, "wb+");
            if(!out) {
                fprintf(stderr,
                    "Cannot open %s for writing.\n",
                    outputFilename);
                return 1;
            }
            assert(out);
            fwrite(buf.data, buf.size, 1, out);
            fclose(out);

            free(outputFilename);
            free(buf.data);

        } else {

            if(!nkxVmHasErrors(vm)) {

                if(!nkiGetErrorCount(vm)) {

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
                        // printf("InstructionCountMax: %u\n", instructionCountMax);

                        // TODO: Give this value an accessor.
                        vm->instructionsLeftBeforeTimeout = 1024;

                        nkxVmIterate(vm, 1);

                        // nkxDbgDumpState(vm, stdout);

                        // printf("\n\n\n\n");
                        // printf("----------------------------------------------------------------------\n");
                        // printf("PC: %u\n", vm->instructionPointer);
                        // printf("Stack...\n");
                        // printf("----------------------------------------------------------------------\n");
                        // nkiVmStackDump(vm);
                        // printf("----------------------------------------------------------------------\n");
                        // printf("Static...\n");
                        // printf("----------------------------------------------------------------------\n");
                        // nkiVmStaticDump(vm);
                        // printf("\n");
                        // dumpListing(vm, script);
                        // getchar();

                        if(nkxGetErrorCount(vm)) {
                            printf("Instruction pointer of failure: %u\n", vm->instructionPointer);
                            break;
                        }

                        // {
                        //     struct WriterTestBuffer buf;
                        //     memset(&buf, 0, sizeof(buf));
                        //     nkxVmSerialize(vm, writerTest, &buf, nktrue);

                        //     {
                        //         struct NKVM *newVm = nkxVmCreate();

                        //         nkxVmRegisterExternalFunction(newVm, "cfunc", testVMFunc);
                        //         nkxVmRegisterExternalFunction(newVm, "catastrophe", testVMCatastrophe);
                        //         nkxVmRegisterExternalFunction(newVm, "print", vmFuncPrint);
                        //         nkxVmRegisterExternalFunction(newVm, "hash", getHash);
                        //         nkxVmRegisterExternalFunction(newVm, "hash2", getHash);
                        //         nkxVmRegisterExternalFunction(newVm, "testHandle1", testHandle1);
                        //         nkxVmRegisterExternalFunction(newVm, "testHandle2", testHandle2);
                        //         nkxVmRegisterExternalFunction(newVm, "setGCCallbackThing", setGCCallbackThing);
                        //         nkxVmRegisterExternalFunction(newVm, "doGCCallbackThing", doGCCallbackThing);
                        //         nkxVmRegisterExternalFunction(newVm, "doSerializationCallbackThing", doSerializationCallbackThing);

                        //         nkxVmRegisterExternalType(newVm, "footype");

                        //         printf("Deserializing...\n");
                        //         {
                        //             nkbool b = nkxVmSerialize(newVm, writerTest, &buf, nkfalse);
                        //             assert(b);
                        //         }

                        //         // {
                        //         //     FILE *out2 = fopen("stest2.txt", "w+");
                        //         //     nkxDbgDumpState(newVm, out2);
                        //         //     fclose(out2);
                        //         // }

                        //         nkxVmDelete(vm);
                        //         vm = newVm;
                        //     }

                        //     free(buf.data);
                        // }

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

                // FIXME: Standardize error output.
                if(nkxVmHasErrors(vm)) {
                    struct NKError *err = vm->errorState.firstError;
                    fprintf(stderr, "Errors detected...\n");
                    while(err) {
                        fprintf(stderr, "  error: %s\n", err->errorText);
                        err = err->next;
                    }
                }

                //     nkiVmStackDump(vm);
                //     // nkxVmGarbageCollect(vm);

                // // Function call test.
                // if(!vm->errorState.firstError) {
                //     struct NKValue retVal;
                //     memset(&retVal, 0, sizeof(retVal));
                //     nkiVmCallFunctionByName(&cs, "callMeFromC", 0, NULL, &retVal);
                // }

            } else {

                // FIXME: Standardize error output.
                fprintf(stdout, "*** AN ERROR WAS DETECTED ***\n");
                fprintf(stderr, "*** AN ERROR WAS DETECTED ***\n");
                {
                    struct NKError *err = vm->errorState.firstError;
                    // while(err) {
                    //     fprintf(stderr, "%s\n", err->errorText);
                    //     err = err->next;
                    // }

                    if(vm->errorState.allocationFailure) {
                        fprintf(stderr, "Allocation failure!\n");
                    }
                }

            }

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

        printf("Final serialized state...\n");
        if(vm->errorState.allocationFailure) {
            // TODO: Remember allocation failure condition for
            // serialization stuff.
            printf("Error was allocation error. Skipping serialization.\n");
        } else {
            struct WriterTestBuffer buf;
            memset(&buf, 0, sizeof(buf));
            printf("Serializing...\n");

            {
                nkbool c = nkxVmSerialize(vm, writerTest, &buf, nktrue);
                if(!c) {
                    printf("Error occurred during serialization.\n");
                    return 1;
                }
            }

            {
                // FILE *out1 = fopen("stest1.txt", "w+");
                // nkxDbgDumpState(vm, stdout);
                // fclose(out1);
            }

            {
                struct NKVM *newVm = nkxVmCreate();

                nkxVmRegisterExternalFunction(newVm, "cfunc", testVMFunc);
                nkxVmRegisterExternalFunction(newVm, "catastrophe", testVMCatastrophe);
                nkxVmRegisterExternalFunction(newVm, "print", vmFuncPrint);
                nkxVmRegisterExternalFunction(newVm, "hash", getHash);
                nkxVmRegisterExternalFunction(newVm, "hash2", getHash);
                nkxVmRegisterExternalFunction(newVm, "testHandle1", testHandle1);
                nkxVmRegisterExternalFunction(newVm, "testHandle2", testHandle2);
                nkxVmRegisterExternalFunction(newVm, "setGCCallbackThing", setGCCallbackThing);
                nkxVmRegisterExternalFunction(newVm, "doGCCallbackThing", doGCCallbackThing);
                nkxVmRegisterExternalFunction(newVm, "doSerializationCallbackThing", doSerializationCallbackThing);

                nkxVmRegisterExternalType(newVm, "footype");

                printf("Deserializing...\n");
                {
                    nkbool b = nkxVmSerialize(newVm, writerTest, &buf, nkfalse);
                    if(!b) {
                        printf("Deserialization of previously serialized VM state failed.\n");
                        // assert(b);
                    }
                }

                {
                    // FILE *out2 = fopen("stest2.txt", "w+");
                    // nkxDbgDumpState(newVm, stdout);
                    // fclose(out2);
                }

                nkxVmDelete(newVm);
            }

            free(buf.data);
        }


        nkxVmGarbageCollect(vm);
        printf("Final object table dump...\n");
        nkiVmObjectTableDump(vm);

        // nkiVmStringTableDump(&vm->stringTable);
        // nkxDbgDumpState(vm, stdout);

        printf("Peak memory usage:    " NK_PRINTF_UINT32 "\n", vm->peakMemoryUsage);
        printf("Current memory usage: " NK_PRINTF_UINT32 "\n", vm->currentMemoryUsage);

        // nkiVmDestroy(&vm);
        nkxVmDelete(vm);

        // printf("Post-cleanup memory usage: %u\n", vm->currentMemoryUsage);

        free(lines[0]);
        free(lines);
    }

    free(script);

    return 0;
}


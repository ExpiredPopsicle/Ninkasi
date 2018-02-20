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

// This file is mostly a messy test harness for Ninkasi. Try not to
// judge too harshly!

#include "nkx.h"

// FIXME: Get rid of this.
#include "nkvm.h"

#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#include "subtest.h"

// ----------------------------------------------------------------------

struct WriterTestBuffer
{
    char *data;
    nkuint32_t readPtr;
    nkuint32_t size;
};

nkuint32_t adler32(void *data, nkuint32_t size)
{
    nkuint32_t a = 1, b = 0;
    nkuint32_t index;

    for(index = 0; index < size; index++) {
        a = (a + ((unsigned char*)data)[index]) % 65521;
        b = (b + a) % 65521;
    }

    return (b << 16) | a;
}

void dumpBufChecksum(struct WriterTestBuffer *buf)
{
    printf("Saved state checksum: " NK_PRINTF_UINT32 "\n", adler32(buf->data, buf->size));
}

nkbool writerTest(void *data, nkuint32_t size, void *userdata, nkbool writeMode)
{
    if(userdata) {

        struct WriterTestBuffer *testBuf = (struct WriterTestBuffer *)userdata;

        if(writeMode) {

            nkuint32_t oldSize = testBuf->size;
            testBuf->size += size;
            testBuf->data = (char*)realloc(testBuf->data, testBuf->size);
            if(!testBuf->data) {
                fprintf(stderr, "Realloc failure to size: " NK_PRINTF_UINT32 "\n", testBuf->size);
                assert(testBuf->data);
            }
            memcpy(testBuf->data + oldSize, data, size);

        } else {

            if(size <= testBuf->size - testBuf->readPtr) {
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

char **nkiDbgSplitLines(const char *str, nkuint32_t *lineCount)
{
    char **lines = NULL;

    // Make a copy of the string. We'll insert null-terminators into
    // this, then record where each line starts.
    nkuint32_t len = strlen(str);
    char *workStr = strdup(str);

    nkuint32_t i = 0;
    nkuint32_t lineStart = 0;

    *lineCount = 0;

    for(i = 0; i < len + 1; i++) {

        if(workStr[i] == '\n' || workStr[i] == 0) {
            workStr[i] = 0;

            (*lineCount)++;
            lines = (char**)realloc(lines, sizeof(char*) * (*lineCount));
            lines[(*lineCount)-1] = &workStr[lineStart];

            lineStart = i + 1;
        }
    }

    return lines;
}

void dumpListing(struct NKVM *vm, const char *script);

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

    buf = (char*)malloc(len + 1);
    fread(buf, len, 1, in);
    buf[len] = 0;

    fclose(in);

    if(scriptSize) {
        *scriptSize = len;
    }

    printf("Loaded script with checksum: " NK_PRINTF_UINT32 "\n", adler32(buf, len));

    return buf;
}

char *loadScriptFromStdin(nkuint32_t *scriptSize)
{
    size_t bufSize = 256;
    size_t scriptLen = 0;
    char *buf = (char*)malloc(bufSize);
    while(fread(&buf[scriptLen], 1, 1, stdin) > 0) {
        scriptLen++;
        buf[scriptLen] = 0;
        if(scriptLen + 1 >= bufSize) {
            bufSize <<= 1;
            buf = (char*)realloc(buf, bufSize);
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
        nkxAddError(
            data->vm,
            "Bad argument count in testVMFunc.");
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

// // This should probably be tracked separately for each VM.
// static NKVMExternalFunctionID doGCCallbackThing_id;

void doGCCallbackThing(struct NKVMFunctionCallbackData *data)
{
    // TODO: Add standard argument count handler.
    if(data->argumentCount != 1) {
        nkiAddError(
            data->vm,
            -1, "Bad argument count in doGCCallbackThing.");
        return;
    }

    // {
    //     struct NKVMExternalDataTypeID id = nkxVmObjectGetExternalType(data->vm, &data->arguments[0]);
    //     if(id.id != NK_INVALID_VALUE) {
    //         printf("This thing isn't a basic object!\n");
    //         return;
    //     }
    // }

    assert(data->argumentCount == 1);
    printf("GC callback called.\n");
    {
        NKVMExternalDataTypeID id = nkxVmObjectGetExternalType(data->vm, &data->arguments[0]);
        // if(strcmp(nkxVmGetExternalTypeName(vm, id), "footype")) {
        //     printf("This thing isn't a basic object!\n");
        //     return;
        // }
        printf("GCing external of type %s\n", nkxVmGetExternalTypeName(data->vm, id));
    }

    {
        char *externalData = (char*)nkxVmObjectGetExternalData(data->vm, &data->arguments[0]);
        printf("GCing external data: %s\n", externalData);
        // free(externalData);
    }
}

void doSerializationCallbackThing(struct NKVMFunctionCallbackData *data)
{
    // FIXME: Remove.
    printf("Seralize callback hit!\n");

    // TODO: Make this standard, or find a way to make sure
    // serialization callbacks aren't called by a normal program. This
    // is important for sandboxing!
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
    // if(data->argumentCount != 1) {
    //     nkiAddError(
    //         data->vm,
    //         -1, "Bad argument count in setGCCallbackThing.");
    //     return;
    // }

    // assert(data->argumentCount == 1);
    // printf("Setting GC callback.\n");

    // doGCCallbackThing_id =
    //     nkxVmRegisterExternalFunction(data->vm, "doGCCallbackThing", doGCCallbackThing);

    // {
    //     struct NKVMExternalDataTypeID id = nkxVmObjectGetExternalType(data->vm, &data->arguments[0]);
    //     if(id.id != NK_INVALID_VALUE) {
    //         printf("This thing isn't a basic object!\n");
    //         return;
    //     }
    // }

    // nkxVmObjectSetGarbageCollectionCallback(
    //     data->vm, &data->arguments[0], doGCCallbackThing_id);

    // nkxVmObjectSetExternalData(data->vm, &data->arguments[0], "butts");

    // {
    //     NKVMExternalFunctionID serializationCallbackId = nkxVmRegisterExternalFunction(
    //         data->vm, "doSerializationCallbackThing", doSerializationCallbackThing);
    //     nkxVmObjectSetSerializationCallback(
    //         data->vm, &data->arguments[0], serializationCallbackId);
    // }

    // {
    //     NKVMExternalDataTypeID id = nkxVmRegisterExternalType(data->vm, "footype");
    //     printf("Registered external type %s\n", nkxVmGetExternalTypeName(data->vm, id));
    //     nkxVmObjectSetExternalType(data->vm, &data->arguments[0], id);
    // }
}

// ----------------------------------------------------------------------

void testSubsystemCleanup(struct NKVMFunctionCallbackData *data)
{
}

void testSubsystemSerialize(struct NKVMFunctionCallbackData *data)
{
    char tmpBuf[256];
    char *testData = (char*)nkxGetExternalSubsystemData(data->vm, "testSubsystem");
    memset(tmpBuf, 0, sizeof(tmpBuf));
    strcpy(tmpBuf, testData);
    // if(data->vm->serializationState.writeMode) {

    data->vm->serializationState.writer(
        tmpBuf, strlen(testData),
        data->vm->serializationState.userdata,
        data->vm->serializationState.writeMode);
    // }

    printf("subsystem: Serialize(%s): %s\n", data->vm->serializationState.writeMode ? "write" : "read", tmpBuf);
}


// ----------------------------------------------------------------------

struct Settings
{
    nkbool compileOnly;
    const char *filename;
    nkuint32_t maxMemory;
};

void printHelp(nkbool isError)
{
    FILE *stream = isError ? stderr : stdout;
    fprintf(stream, "<TODO: help text goes here>\n");
}

nkbool checkErrors(struct NKVM *vm)
{
    if(!vm) {
        fprintf(stderr, "Error: VM has gone missing.\n");
        return nktrue;
    }

    if(nkxVmHasErrors(vm)) {

        nkuint32_t errorBufLen = nkxGetErrorLength(vm);
        char *buf = (char*)malloc(errorBufLen);
        nkxGetErrorText(vm, buf);

        fprintf(stderr, "Errors detected:\n");
        fprintf(stderr, "%s\n", buf);

        free(buf);

        return nktrue;
    }
    return nkfalse;
}

nkbool parseCmdLine(int argc, char *argv[], struct Settings *settings)
{
    int i;
    nkbool noMoreSwitches = nkfalse;

    // Set up some nice defaults.
    memset(settings, 0, sizeof(*settings));

    // 16mb default max memory usage.
    settings->maxMemory = 16L * (1L << 20L);

    for(i = 1; i < argc; i++) {

        if(strcmp("-c", argv[i]) == 0) {

            settings->compileOnly = nktrue;

        } else if(strcmp("-m", argv[i]) == 0) {

            i++;
            if(i < argc) {
                settings->maxMemory = atol(argv[i]);
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
    subsystemTest_initLibrary(vm, cs);

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

    // FIXME: FIX THE DEMO. DON'T REGISTER THIS LATE.
    nkxVmRegisterExternalFunction(vm, "doGCCallbackThing", doGCCallbackThing);
    nkxVmRegisterExternalFunction(vm, "doSerializationCallbackThing", doSerializationCallbackThing);

    nkxVmRegisterExternalType(vm, "footype", NULL, NULL);

    // nkxSetExternalSubsystemCleanupCallback(vm, "testSubsystem", testSubsystemCleanup);
    // nkxSetExternalSubsystemSerializationCallback(vm, "testSubsystem", testSubsystemSerialize);
    // nkxSetExternalSubsystemData(vm, "testSubsystem", "ASDFASDFASDFASDFASDFASDF");

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

struct NKVM *testSerializer(struct NKVM *vm, struct Settings *settings)
{
    struct WriterTestBuffer buf;
    memset(&buf, 0, sizeof(buf));

    printf("----------------------------------------------------------------------\n");

    printf("Testing serializer...\n");

    {
        nkbool c = nkxVmSerialize(vm, writerTest, &buf, nktrue);
        dumpBufChecksum(&buf);

        if(!c) {
            printf("Error occurred during serialization. 2\n");
            nkxVmDelete(vm);
            printf("----------------------------------------------------------------------\n");
            return NULL;
        }
    }

    {
        struct NKVM *newVm = nkxVmCreate();
        nkxSetMaxAllocatedMemory(newVm, settings->maxMemory);
        initInternalFunctions(newVm, NULL);

        printf("Deserializing...\n");
        {
            nkbool b = nkxVmSerialize(newVm, writerTest, &buf, nkfalse);
            if(!b) {
                printf("Deserialization of previously serialized VM state failed.\n");
                printf("Deleting new VM...\n");
                nkxVmDelete(newVm);
                newVm = NULL;
            }
        }

        printf("Deleting old VM...\n");

        nkxVmDelete(vm);

        vm = newVm;
    }

    free(buf.data);

    printf("----------------------------------------------------------------------\n");

    return vm;
}



#define ERROR_CODE 0

// FIXME: Remove this!
extern nkuint32_t nkiMemFailRate;

void dumpGlobals(struct NKVM *vm)
{
    nkuint32_t i;

    printf("Global Variables:\n");

    for(i = 0; i < vm->globalVariableCount; i++) {

        struct NKGlobalVariableRecord *var = &vm->globalVariables[i];

        // FIXME: Not using proper format specifier.
        printf("  %4u %s = %s\n",
            i,
            var->name,
            nkxValueToString(vm, &vm->staticSpace[var->staticPosition & vm->staticAddressMask]));
    }
}

int main(int argc, char *argv[])
{
    struct Settings settings;
    char *script = NULL;
    nkuint32_t scriptSize = 0;

    if(!parseCmdLine(argc, argv, &settings)) {
        return ERROR_CODE;
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
        return ERROR_CODE;
    }

    if(!script) {
        fprintf(stderr, "Script failed to even load.\n");
        return ERROR_CODE;
    }

    {
        struct NKVM *vm = nkxVmCreate();
        if(checkErrors(vm)) {
            free(script);
            nkxVmDelete(vm);
            return ERROR_CODE;
        }

        nkxSetMaxAllocatedMemory(vm, settings.maxMemory);

        // NKVM binary blobs start with \0.
        if(script && script[0] != 0) {

            // First, scan for some directives...
            {
                nkuint32_t lineCount = 0;
                char **lines = nkiDbgSplitLines(script, &lineCount);
                nkuint32_t i;
                for(i = 0; i < lineCount; i++) {
                    const char *memFailPct = "// #failrate: ";
                    if(strlen(lines[i]) >= strlen(memFailPct)) {
                        if(memcmp(lines[i], memFailPct, strlen(memFailPct)) == 0) {
                            nkiMemFailRate = atol(lines[i] + strlen(memFailPct));
                        }
                    }
                }
                free(lines[0]);
                free(lines);
            }

            // Load and compile a script.

            {
                struct NKCompilerState *cs = nkxCompilerCreate(vm);

                if(cs) {

                    initInternalFunctions(vm, cs);
                    nkxCompilerCompileScript(cs, script);
                    nkxCompilerFinalize(cs);

                } else {

                    fprintf(stderr, "Can't create compiler state. Out of memory?\n");
                    free(script);
                    nkxVmDelete(vm);
                    return ERROR_CODE;
                }

                if(checkErrors(vm)) {
                    free(script);
                    nkxVmDelete(vm);
                    return ERROR_CODE;
                }

                dumpListing(vm, script);
            }

        } else {

            // Load a binary.

            struct WriterTestBuffer buf;
            buf.data = script;
            buf.readPtr = 0;
            buf.size = scriptSize;

            initInternalFunctions(vm, NULL);

            printf("ADSF0\n");

            if(!nkxVmSerialize(vm, writerTest, &buf, nkfalse)) {
                free(script);
                nkxVmDelete(vm);
                return ERROR_CODE;
            }
        }

        printf("Script loaded. Compiling...\n");

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
                    newFilename = (char*)malloc(i + 4 + 1);
                    memcpy(newFilename, oldFilename, i);
                    newFilename[i] = 0;
                    strcat(newFilename, ".nkb");
                    outputFilename = newFilename;
                }

            }

            // Okay well, even if all of that failed we still need
            // SOMETHING to call this output file.
            if(!outputFilename) {
                outputFilename = strdup("unknown.nkb");
            }

            // If our input is another nkb file, then I guess we
            // should do file.nkb.nkb. Maybe we can diff the files and
            // make sure they come out the same again.
            if(!strcmp(outputFilename, settings.filename)) {
                outputFilename = (char*)realloc(outputFilename, strlen(outputFilename) + 1 + 4);
                strcat(outputFilename, ".nkb");
            }

            if(!outputFilename) {
                fprintf(stderr, "Cannot determine output file for %s.\n", settings.filename);
                free(script);
                nkxVmDelete(vm);
                return ERROR_CODE;
            }

            buf.data = NULL;
            buf.readPtr = 0;
            buf.size = 0;

            nkxVmSerialize(vm, writerTest, &buf, nktrue);
            dumpBufChecksum(&buf);

            out = fopen(outputFilename, "wb+");
            if(!out) {
                fprintf(stderr,
                    "Cannot open %s for writing.\n",
                    outputFilename);
                return ERROR_CODE;
            }
            assert(out);
            fwrite(buf.data, buf.size, 1, out);
            fclose(out);

            free(outputFilename);
            free(buf.data);

        } else {

            if(!nkxVmHasErrors(vm)) {

                if(!nkiGetErrorCount(vm)) {

                    nkuint32_t counter = 0;

                    // nkiVmExecuteProgram(vm);

                    // TODO: Give this value an accessor.
                    nkxSetRemainingInstructionLimit(vm, (1024 * 1024 * 1024) & 0xffff);
                    // nkxSetRemainingInstructionLimit(vm, NK_INVALID_VALUE);

                    while(
                        vm->instructions[
                            vm->instructionPointer &
                            vm->instructionAddressMask].opcode != NK_OP_END)
                    {
                        // // FIXME: Remove this.
                        // dumpGlobals(vm);

                        nkxVmIterate(vm, 1);
                        // nkxVmGarbageCollect(vm);

                        if(counter % 1024 == 0) {
                            nkxVmShrink(vm);
                        }



                        // Test the serializer at weird intervals.
                        if(counter % 1100 == 0) {
                            nkxVmGarbageCollect(vm);
                            nkxVmShrink(vm);
                            // assert(!nkxGetErrorCount(vm));

                            vm = testSerializer(vm, &settings);

                            if(!vm || checkErrors(vm)) {
                                // free(script);
                                // printf("Cleaning up VM...\n");
                                // nkxVmDelete(vm);
                                // printf("VM cleanup done.\n");
                                // return ERROR_CODE;
                                printf("testSerializer failed\n");
                                break;
                            }
                        }

                        counter++;

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
                    }

                    if(vm) {
                        struct NKValue *v = nkxVmFindGlobalVariable(vm, NULL, "readMeFromC");
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

                if(checkErrors(vm)) {
                    free(script);
                    nkxVmDelete(vm);
                    return ERROR_CODE;
                }

                // // Function call test.
                // if(!vm->errorState.firstError) {
                //     struct NKValue retVal;
                //     memset(&retVal, 0, sizeof(retVal));
                //     nkiVmCallFunctionByName(&cs, "callMeFromC", 0, NULL, &retVal);
                // }

            } else {

                if(checkErrors(vm)) {
                    free(script);
                    nkxVmDelete(vm);
                    return ERROR_CODE;
                }

            }

        }

        assert(!vm->catastrophicFailureJmpBuf);

        printf("----------------------------------------------------------------------\n");
        printf("  Finish\n");
        printf("----------------------------------------------------------------------\n");


        // printf("Final stack...\n");
        // nkiVmStackDump(vm);

        // printf("Final dumpstate before GC...\n");
        // nkxDbgDumpState(vm, stdout);

        printf("Final GC...\n");
        nkxVmGarbageCollect(vm);

        printf("Final shrink...\n");
        nkxVmShrink(vm);

        // printf("Final dumpstate...\n");
        // nkxDbgDumpState(vm, stdout);

        // printf("Final stack again...\n");
        // nkiVmStackDump(vm);

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
                dumpBufChecksum(&buf);

                if(!c) {
                    printf("Error occurred during serialization. 1\n");
                    free(script);

                    nkxVmDelete(vm);
                    return ERROR_CODE;
                }
            }

            {
                // FILE *out1 = fopen("stest1.txt", "w+");
                // nkxDbgDumpState(vm, stdout);
                // fclose(out1);
            }

            {
                struct NKVM *newVm = nkxVmCreate();

                initInternalFunctions(newVm, NULL);


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
        printf("----------------------------------------------------------------------\n");
        printf("Peak memory usage:    " NK_PRINTF_UINT32 "\n", vm->peakMemoryUsage);
        printf("Current memory usage: " NK_PRINTF_UINT32 "\n", vm->currentMemoryUsage);

        nkxVmDelete(vm);
    }

    free(script);

    return 0;
}


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

// This file is mostly a messy test harness for Ninkasi. Try not to
// judge too harshly!
//
// No seriously this code is shit. I'll come up with actual example
// code as the project moves along.

#include "../nkx.h"
#include "subtest.h"
#include "stuff.h"
#include "settings.h"

#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

// ----------------------------------------------------------------------
// Serialization support functions

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

nkuint32_t getVmStateChecksum(struct NKVM *vm)
{
    struct WriterTestBuffer buf;
    nkbool c;
    nkuint32_t checksum = 0;

    memset(&buf, 0, sizeof(buf));
    c = nkxVmSerialize(vm, writerTest, &buf, nktrue);

    if(!c) {
        return 0;
    }

    checksum = adler32(buf.data, buf.size);

    free(buf.data);

    return checksum;
}

struct NKVM *testSerializer(struct NKVM *vm, struct Settings *settings)
{
    nkuint32_t oldInstructionLimit = nkxGetRemainingInstructionLimit(vm);
    struct WriterTestBuffer buf;
    memset(&buf, 0, sizeof(buf));

    printf("----------------------------------------------------------------------\n");

    printf("Testing serializer...\n");

    {
        nkbool c = nkxVmSerialize(vm, writerTest, &buf, nktrue);
        dumpBufChecksum(&buf);

        if(!c) {
            printf("Error occurred during serialization. 2\n");
            free(buf.data);
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

            // Check to make sure that we get the same checksum if we
            // serialize again, to make sure serializing and
            // deserializing are totally reversible.
            if(b) {
                nkuint32_t checksum = getVmStateChecksum(newVm);
                printf("Deserialize checksum: " NK_PRINTF_UINT32 "\n", checksum);
            }

        }

        printf("Deleting old VM...\n");

        nkxVmDelete(vm);

        vm = newVm;
    }

    free(buf.data);

    printf("----------------------------------------------------------------------\n");

    if(vm) {
        nkxSetRemainingInstructionLimit(vm, oldInstructionLimit);
    }

    return vm;
}

// ----------------------------------------------------------------------
// Script loaders

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

// ----------------------------------------------------------------------
// VM stuff

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

// ----------------------------------------------------------------------
// Entry point

int main(int argc, char *argv[])
{
    struct Settings settings;
    char *script = NULL;
    nkuint32_t scriptSize = 0;
    struct NKVM *vm = NULL;

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

    vm = nkxVmCreate();
    if(checkErrors(vm)) {
        free(script);
        nkxVmDelete(vm);
        return settings.exitErrorCode;
    }

    nkxSetMaxAllocatedMemory(vm, settings.maxMemory);

    // NKVM binary blobs start with \0.
    if(script && script[0] != 0) {

        struct NKCompilerState *cs;

        scanFileDirectives(script, &settings);

        // Compile the script.
        cs = nkxCompilerCreate(vm);
        if(cs) {
            initInternalFunctions(vm, cs);
            nkxCompilerCompileScript(cs, script, settings.filename);
            nkxCompilerFinalize(cs);
        }
        if(checkErrors(vm)) {
            free(script);
            nkxVmDelete(vm);
            return settings.exitErrorCode;
        }

    } else {

        // Load a binary.

        struct WriterTestBuffer buf;
        buf.data = script;
        buf.readPtr = 0;
        buf.size = scriptSize;

        initInternalFunctions(vm, NULL);

        if(!nkxVmSerialize(vm, writerTest, &buf, nkfalse)) {
            free(script);
            nkxVmDelete(vm);
            return settings.exitErrorCode;
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
            return settings.exitErrorCode;
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
            return 1;
        }
        fwrite(buf.data, buf.size, 1, out);
        fclose(out);

        free(outputFilename);
        free(buf.data);

    } else {

        printf("----------------------------------------------------------------------\n");
        printf("  Execution begin\n");
        printf("----------------------------------------------------------------------\n");

        if(!nkxVmHasErrors(vm)) {

            nkuint32_t serializerCounter = settings.serializerTestFrequency;
            nkuint32_t shrinkCounter = settings.shrinkFrequency;

            // TODO: Give this value a command line parameter.
            nkxSetRemainingInstructionLimit(vm, (1024L * 1024L));
            // nkxSetRemainingInstructionLimit(vm, NK_INVALID_VALUE);

            while(!nkxVmProgramHasEnded(vm)) {

                // Figure out how many iterations we can do
                // before the next shrink/serialize test
                // happens.
                nkuint32_t iterationCount = NK_INVALID_VALUE;
                if(serializerCounter < iterationCount) {
                    iterationCount = serializerCounter;
                }
                if(shrinkCounter < iterationCount) {
                    iterationCount = shrinkCounter;
                }

                // Iterate. Normally we'd iterate for multiple
                // instructions, but in this case we're going
                // to do one at a time so we can fire off the
                // shrink and serializer tests at certain
                // intervals.
                nkxVmIterate(vm, iterationCount);

                // Adjust counters according to how many
                // operations we probably ran.
                if(shrinkCounter != NK_INVALID_VALUE) {
                    shrinkCounter -= iterationCount;
                }
                if(serializerCounter != NK_INVALID_VALUE) {
                    serializerCounter -= iterationCount;
                }

                // Test the VM memory shrink functionality at
                // intervals.
                if(shrinkCounter == 0) {
                    nkxVmShrink(vm);
                    shrinkCounter = settings.shrinkFrequency;
                } else {
                    shrinkCounter--;
                }

                // Test the serializer at intervals.
                if(serializerCounter == 0) {

                    nkxVmGarbageCollect(vm);

                    nkxVmShrink(vm);

                    // FIXME: This was screwing up our diffs because
                    // of the pointer value. Find a better solution.

                    // printf("vm before testserializer: %p\n", vm);

                    vm = testSerializer(vm, &settings);
                    if(!vm || checkErrors(vm)) {
                        printf("testSerializer failed\n");
                        break;
                    }
                    serializerCounter = settings.serializerTestFrequency;

                } else {
                    serializerCounter--;
                }

                // Bail out on errors.
                if(nkxGetErrorCount(vm)) {
                    printf("An error occurred.\n");
                    break;
                }
            }

            // Example of searching for a global variable in the
            // VM.
            if(vm) {
                struct NKValue *v = nkxVmFindGlobalVariable(vm, NULL, "readMeFromC");
                printf("Searched for readMeFromC variable...\n  ");
                if(v) {
                    printf("Value found: %s\n", nkxValueToString(vm, v));
                } else {
                    printf("Value NOT found.\n");
                }
            }
        }

        printf("----------------------------------------------------------------------\n");
        printf("  Execution end\n");
        printf("----------------------------------------------------------------------\n");

        if(checkErrors(vm)) {
            free(script);
            nkxVmDelete(vm);
            return settings.exitErrorCode;
        }
    }

    // printf("Final dumpstate before GC...\n");
    // nkxDbgDumpState(vm, stdout);

    printf("Final garbage collection pass...\n");
    nkxVmGarbageCollect(vm);

    printf("Final shrink pass...\n");
    nkxVmShrink(vm);

    // printf("Final dumpstate...\n");
    // nkxDbgDumpState(vm, stdout);

    // printf("Final stack again...\n");
    // nkiVmStackDump(vm);

    printf("Final serialized state...\n");
    if(nkxVmHasAllocationFailure(vm)) {

        printf("Error was allocation error. Skipping serialization.\n");

    } else {

        nkbool serializerSuccess;
        struct NKVM *newVm;

        struct WriterTestBuffer buf;
        memset(&buf, 0, sizeof(buf));
        printf("Serializing...\n");

        serializerSuccess = nkxVmSerialize(vm, writerTest, &buf, nktrue);
        dumpBufChecksum(&buf);

        if(!serializerSuccess) {
            printf("Error occurred during serialization. 1\n");
            free(buf.data);
            free(script);
            nkxVmDelete(vm);
            return settings.exitErrorCode;
        }

        printf("Performing final deserialization test...\n");
        newVm = nkxVmCreate();
        initInternalFunctions(newVm, NULL);
        serializerSuccess = nkxVmSerialize(newVm, writerTest, &buf, nkfalse);
        if(!serializerSuccess) {
            printf("Deserialization of previously serialized VM state failed!\n");
        } else {
            printf("Success!\n");
        }

        printf("Cleaning up after deserialization test...\n");
        nkxVmDelete(newVm);
        free(buf.data);
    }

    printf("Peak memory usage:    " NK_PRINTF_UINT32 "\n", nkxVmGetPeakMemoryUsage(vm));
    printf("Current memory usage: " NK_PRINTF_UINT32 "\n", nkxVmGetCurrentMemoryUsage(vm));

    printf("Cleaning up main VM...\n");
    nkxVmDelete(vm);
    printf("Done!\n");

    free(script);

    return 0;
}


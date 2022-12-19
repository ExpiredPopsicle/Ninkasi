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
#include "logging.h"

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
    writeLog(2, "Saved state checksum: " NK_PRINTF_UINT32 "\n", adler32(buf->data, buf->size));
}

void dumpBufHex(int verbosity, void *data, nkuint32_t size)
{
    const char charMap[17] = { "0123456789abcdef" };
    nkuint32_t i = 0;
    for(i = 0; i < size; i++) {
        const char *c = (const char *)data + i;
        writeLog(verbosity, "%c%c", charMap[(*c & 0xf0) >> 4], charMap[*c & 0xf]);
    }
    writeLog(verbosity, "\n");
}

nkbool writerTest(void *data, nkuint32_t size, void *userdata, nkbool writeMode)
{
    if(userdata) {

        struct WriterTestBuffer *testBuf = (struct WriterTestBuffer *)userdata;

        if(writeMode) {

            // Writing path.
            nkuint32_t oldSize = testBuf->size;
            testBuf->size += size;
            testBuf->data = (char*)realloc(testBuf->data, testBuf->size);
            if(!testBuf->data) {
                writeError("Realloc failure to size: " NK_PRINTF_UINT32 "\n", testBuf->size);
                assert(testBuf->data);
            }
            memcpy(testBuf->data + oldSize, data, size);

        } else {

            // Loading path.
            if(size <= testBuf->size - testBuf->readPtr) {
                memcpy(data, testBuf->data + testBuf->readPtr, size);
                testBuf->readPtr += size;
            } else {
                writeError("Premature end of buffer!\n");
                return nkfalse;
            }
        }
    }

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

// Copy limits from the command line settings in the Settings struct
// to the VM.
void setVmLimits(
    struct NKVM *vm)
{
    nkxSetMaxAllocatedMemory(
        vm, getGlobalSettings()->maxMemory);
    nkxSetRemainingInstructionLimit(
        vm, getGlobalSettings()->instructionCountLimit);
}

struct NKVM *testSerializer(struct NKVM *vm)
{
    // This will get reset with the new VM so record it so we can set
    // it back.
    nkuint32_t oldInstructionLimit =
        nkxGetRemainingInstructionLimit(vm);

    // Output buffer.
    struct WriterTestBuffer buf;
    memset(&buf, 0, sizeof(buf));

    writeLog(2, "Testing serializer...\n");

    writeLog(2, "Garbage collecting before serializing...\n");
    nkxVmGarbageCollect(vm);
    writeLog(2, "Shrinking serializing...\n");
    nkxVmShrink(vm);

    {
        nkbool serializerSuccess =
            nkxVmSerialize(vm, writerTest, &buf, nktrue);
        dumpBufChecksum(&buf);

        if(!serializerSuccess) {
            writeError("Error occurred during serialization.\n");
            free(buf.data);
            nkxVmDelete(vm);
            return NULL;
        }
    }

    {
        struct NKVM *newVm = nkxVmCreate();
        setVmLimits(newVm);
        initInternalFunctions(newVm, NULL);

        writeLog(2, "Deserializing...\n");
        {
            nkbool b = nkxVmSerialize(newVm, writerTest, &buf, nkfalse);
            if(!b) {
                writeLog(2, "Deserialization of previously serialized VM state failed.\n");
                writeLog(2, "Deleting new VM...\n");
                nkxVmDelete(newVm);
                newVm = NULL;
            }

            // Check to make sure that we get the same checksum if we
            // serialize again, to make sure serializing and
            // deserializing are totally reversible.
            if(b) {
                nkuint32_t checksum = getVmStateChecksum(newVm);
                writeLog(2, "Deserialize checksum: " NK_PRINTF_UINT32 "\n", checksum);
            }

        }

        writeLog(2, "Deleting old VM...\n");

        nkxVmDelete(vm);

        vm = newVm;
    }

    free(buf.data);

    // Restore old instruction count limit.
    if(vm) {
        nkxSetRemainingInstructionLimit(vm, oldInstructionLimit);
    }

    return vm;
}

// ----------------------------------------------------------------------
// Script loaders

char *loadScriptFromStream(nkuint32_t *scriptSize, FILE *in)
{
    size_t bufSize = 256;
    size_t scriptLen = 0;
    char *buf = (char*)malloc(bufSize);
    while(fread(&buf[scriptLen], 1, 1, in) > 0) {
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

char *loadScript(const char *filename, nkuint32_t *scriptSize)
{
    FILE *in = fopen(filename, "rb");
    char *buf = loadScriptFromStream(scriptSize,in);
    fclose(in);
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
    char *script = NULL;
    nkuint32_t scriptSize = 0;
    struct NKVM *vm = NULL;

    if(!parseCmdLine(argc, argv)) {
        return 1;
    }

    // Load script file or standard input.
    if(getGlobalSettings()->filename) {

        if(strcmp(getGlobalSettings()->filename, "-") == 0) {
            // "-" as a filename means load from stdin.
            script = loadScriptFromStream(&scriptSize, stdin);
        } else {
            // Load from an actual file.
            script = loadScript(getGlobalSettings()->filename, &scriptSize);
        }

    } else {

        // Default to loading from stdin.
        script = loadScriptFromStream(&scriptSize, stdin);

        // Filename is still used internally, and freed from heap at
        // the end.
        getGlobalSettings()->filename = strdup("");
    }

    if(!script) {
        fprintf(stderr, "Script failed to even load.\n");
        return 1;
    }

    // Create the VM (for the first time.)
    vm = nkxVmCreate();
    if(checkErrors(vm)) {
        free(script);
        nkxVmDelete(vm);
        printf("Failed to create VM!\n");
        return getGlobalSettings()->exitErrorCode;
    }

    setVmLimits(vm);

    // NKVM binary blobs start with \0.
    if(script && script[0] != 0) {

        struct NKCompilerState *cs;

        scanFileDirectives(script);

        // Compile the script.
        cs = nkxCompilerCreate(vm);
        if(cs) {
            initInternalFunctions(vm, cs);
            nkxCompilerCompileScript(
                cs, script, getGlobalSettings()->filename);
            nkxCompilerFinalize(cs);
        }
        if(checkErrors(vm)) {
            free(script);
            nkxVmDelete(vm);
            return getGlobalSettings()->exitErrorCode;
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
            return getGlobalSettings()->exitErrorCode;
        }
    }

    printf("Script loaded. Compiling...\n");

    if(getGlobalSettings()->compileOnly) {

        FILE *out = NULL;
        struct WriterTestBuffer buf;
        char *outputFilename = NULL;

        if(getGlobalSettings()->filename) {

            char *newFilename = NULL;
            const char *oldFilename =
                getGlobalSettings()->filename ? getGlobalSettings()->filename :
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
        if(!strcmp(outputFilename, getGlobalSettings()->filename)) {
            outputFilename = (char*)realloc(outputFilename, strlen(outputFilename) + 1 + 4);
            strcat(outputFilename, ".nkb");
        }

        if(!outputFilename) {
            fprintf(
                stderr,
                "Cannot determine output file for %s.\n",
                getGlobalSettings()->filename);
            free(script);
            nkxVmDelete(vm);
            return getGlobalSettings()->exitErrorCode;
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

        assert(script);
        nkxDbgDumpState(vm, script, stdout);

        printf("----------------------------------------------------------------------\n");
        printf("  Execution begin\n");
        printf("----------------------------------------------------------------------\n");

        if(!nkxVmHasErrors(vm)) {

            nkuint32_t serializerCounter =
                getGlobalSettings()->serializerTestFrequency;
            nkuint32_t shrinkCounter =
                getGlobalSettings()->shrinkFrequency;

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
                    shrinkCounter = getGlobalSettings()->shrinkFrequency;
                } else {
                    shrinkCounter--;
                }

                // Test the serializer at intervals.
                if(serializerCounter == 0) {
                    vm = testSerializer(vm);
                    if(!vm || checkErrors(vm)) {
                        printf("testSerializer failed\n");
                        break;
                    }
                    serializerCounter =
                        getGlobalSettings()->serializerTestFrequency;
                } else {
                    serializerCounter--;
                }

                // Bail out on errors.
                if(nkxGetErrorCount(vm)) {
                    printf("An error occurred.\n");
                    break;
                }
            }
        }

        printf("----------------------------------------------------------------------\n");
        printf("  Execution end\n");
        printf("----------------------------------------------------------------------\n");

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

        if(checkErrors(vm)) {
            free(script);
            nkxVmDelete(vm);
            return getGlobalSettings()->exitErrorCode;
        }
    }

    // printf("Final dumpstate before GC...\n");
    // nkxDbgDumpState(vm, stdout);

    printf("Final garbage collection pass...\n");
    nkxVmGarbageCollect(vm);

    printf("Final shrink pass...\n");
    nkxVmShrink(vm);

    printf("Final dumpstate...\n");
    nkxDbgDumpState(vm, script, stdout);

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
            return getGlobalSettings()->exitErrorCode;
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


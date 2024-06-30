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

// Ninkasi command-line interpreter.
//
// This file needs to serve as example code in addition to being the
// actual command-line interpreter that gets installed when running
// "make install". So we're going to use normal C types instead of
// Ninkasi's internal typedefs, and also avoid using any internal
// Ninkasi functions or types.

#include "../nkx.h"

#include <stdio.h>
#include <string.h>
#include <malloc.h>

// This gets filled with information in parseCmdLine.
struct Settings
{
    char *scriptFilename;
    nkbool replMode;
};

// Dump the --help text to stdout.
void showHelp(const char *argv0)
{
    printf("Usage: %s [script filename]\n\n", argv0);

    printf(
        "Ninkasi Command Line Interpreter 0.01\n"
        "\n"
        "This program will execute Ninkasi scripts with a minimal loaded\n"
        "system library on the command line.\n"
        "\n"
        "Options:\n"
        "\n"
        "  --help        You're sitting in it.\n"
        "\n");
}

// We only have two different command line options right now, so we're
// not resorting to getopt or anything fancy just yet.
int parseCmdLine(int argc, char *argv[], struct Settings *settings)
{
    int i;

    for(i = 1; i < argc; i++) {

        if(argv[i][0] == '-') {

            if(!strcmp(argv[i], "--help")) {

                showHelp(argv[0]);
                return 1;

            } else {
                fprintf(stderr, "error: Unknown command line parameter: %s\n", argv[i]);
                return 0;
            }

        } else {

            if(!settings->scriptFilename) {

                settings->scriptFilename = argv[i];

            } else {

                fprintf(stderr, "error: Multiple scripts specified.\n");
                return 0;
            }

        }
    }

    return 1;
}

// This loads a file into a heap-allocated buffer. Returns NULL on
// error.
char *loadScriptFromStream(nkuint32_t *scriptSize, FILE *in, nkbool stopOnNewline)
{
    size_t bufSize = 256;
    nkuint32_t scriptLen = 0;
    char *buf = (char*)malloc(bufSize);

    // Use a local variable instead of the argument if the argument
    // was NULL.
    if(!scriptSize) {
        scriptSize = &scriptLen;
    }

    *scriptSize = 0;

    while(fread(&buf[*scriptSize], 1, 1, in) > 0) {

        (*scriptSize)++;

        // Add null terminator immediately.
        buf[*scriptSize] = 0;

        // Reallocate input buffer if necessary.
        if(*scriptSize + 1 >= bufSize) {

            // Double buffer size.
            bufSize <<= 1;

            // Allocate.
            char *newBuf = (char*)realloc(buf, bufSize);

            // Handle allocation failure.
            if(!newBuf) {
                free(buf);
                buf = NULL;
                *scriptSize = 0;
                return NULL;
            }

            buf = newBuf;
        }

        // Sometimes we just want to read up until the next newline.
        if(buf[*scriptSize - 1] == '\n' && stopOnNewline) {
            break;
        }
    }

    return buf;
}

char *loadScript(const char *filename, nkuint32_t *scriptSize)
{
    FILE *in = fopen(filename, "rb");
    if(in) {
        char *buf = loadScriptFromStream(scriptSize, in, nkfalse);
        fclose(in);
        return buf;
    }
    return NULL;
}

void printErrors(struct NKVM *vm)
{
    if(!vm) {
        fprintf(stderr, "error: VM has gone missing.\n");
        return;
    }

    if(nkxVmHasErrors(vm)) {
        nkuint32_t errorBufLen = nkxGetErrorLength(vm);
        char *buf = (char*)malloc(errorBufLen);
        fprintf(stderr, "error:\n");
        nkxGetErrorText(vm, buf);
        fprintf(stderr, "%s\n", buf);
        free(buf);
    }
}

// This just checks to see if the VM has any errors, and prints them
// to stderror if it does.
int checkNoErrors(struct NKVM *vm)
{
    if(!vm) {
        return 0;
    }

    return !nkxVmHasErrors(vm);
}

// "print" function callback.
void printFunc(struct NKVMFunctionCallbackData *data)
{
    nkuint32_t i;
    for(i = 0; i < data->argumentCount; i++) {
        printf("%s", nkxValueToString(data->vm, &data->arguments[i]));
    }
}

void stringGetASCII(struct NKVMFunctionCallbackData *data)
{
    struct NKValue *string = &data->arguments[0];
    struct NKValue *index =  &data->arguments[1];

    const char *cstring = nkxValueToString(data->vm, string);

    char outString[2] = { 0, 0 };
    nkint32_t indexAsInt = nkxValueToInt(data->vm, index);

    if(indexAsInt < 0 || indexAsInt >= strlen(cstring)) {
        nkxAddError(data->vm, "String ASCII index out of range!");
        return;
    }

    outString[0] = cstring[indexAsInt];

    nkxValueSetString(
        data->vm,
        &data->returnValue,
        outString);
}

void stringGetLengthASCII(struct NKVMFunctionCallbackData *data)
{
    struct NKValue *string = &data->arguments[0];
    const char *cstring = nkxValueToString(data->vm, string);

    nkxValueSetInt(
        data->vm,
        &data->returnValue,
        strlen(cstring));
}

void loadFile(struct NKVMFunctionCallbackData *data)
{
    const char *filename = nkxValueToString(
        data->vm, &data->arguments[0]);

    FILE *inFile = fopen(filename, "rb");
    size_t fileSize;

    if(!inFile) {
        nkxAddError(data->vm, "Cannot open file.");
        return;
    }

    fseek(inFile, 0, SEEK_END);
    fileSize = ftell(inFile);
    fseek(inFile, 0, SEEK_SET);

    char *buf = malloc(fileSize + 1);

    fread(buf, fileSize, 1, inFile);
    buf[fileSize] = 0;

    fclose(inFile);

    nkxValueSetString(
        data->vm,
        &data->returnValue,
        buf);

    free(buf);
}


// This just sets up IO functions in the VM.
void setupStdio(struct NKVM *vm, struct NKCompilerState *compiler)
{
    // The most basic function we will need is "print". Otherwise it's
    // not possible to get anything out. (Well, it is. You just have
    // to have the hosting application explicitly pull data out of the
    // finished VM.)
    nkxVmSetupExternalFunction(
        vm, compiler, "print",
        printFunc, nktrue,
        NK_INVALID_VALUE);

    nkxVmSetupExternalFunction(
        vm, compiler, "stringGetASCII",
        stringGetASCII, nktrue,
        2,
        NK_VALUETYPE_STRING,
        NK_VALUETYPE_INT);

    nkxVmSetupExternalFunction(
        vm, compiler, "stringGetLengthASCII",
        stringGetLengthASCII, nktrue,
        1,
        NK_VALUETYPE_STRING);

    nkxVmSetupExternalFunction(
        vm, compiler, "loadFile",
        loadFile, nktrue,
        1,
        NK_VALUETYPE_STRING);

    // FIXME: This program might actually be useful if we had some way
    // of doing input instead of just output.
}

int main(int argc, char *argv[])
{
    struct NKVM *vm = NULL;
    struct NKCompilerState *compiler = NULL;
    char *scriptText = NULL;
    struct Settings settings;
    nkuint32_t scriptSize = 0;

    memset(&settings, 0, sizeof(settings));

    if(!parseCmdLine(argc, argv, &settings)) {
        // Command line parse error occurred.
        return 1;
    }

    if(!settings.scriptFilename) {

        // No filename? Put us into REPL-mode.
        settings.replMode = nktrue;

    } else {

        // File name exists. Load it.
        scriptText = loadScript(settings.scriptFilename, &scriptSize);
        if(!scriptText) {
            // Script failed to load.
            fprintf(stderr, "error: Script failed to load.\n");
            return 1;
        }

    }

    // Create the VM and compiler.
    vm = nkxVmCreate();
    compiler = nkxCompilerCreate(vm);

    // Set up some standard functions (print, etc).
    setupStdio(vm, compiler);

    // Compile the script. (You would do this multiple times before
    // finalizing for multi-file scripts.)
    if(settings.replMode) {

        while(1) {

            nkuint32_t oldWritePointer = nkxCompilerGetInstructionWriteIndex(compiler);

            // Read text.
            printf("\n>>> ");
            scriptText = loadScriptFromStream(
                &scriptSize, stdin, nktrue);

            // Handle EOF.
            if(!scriptText || !scriptSize) {
                printf("\n");
                break;
            }

            // Attempt to compile new code.
            nkxCompilerCompileScript(compiler, scriptText, "<stdin>");
            free(scriptText);

            // Finalize and run.
            if(checkNoErrors(vm)) {
                nkxCompilerPartiallyFinalize(compiler);
                nkxVmIterate(vm, NK_UINT_MAX);
            }

            // Dump errors, if necessary.
            if(!checkNoErrors(vm)) {
                printErrors(vm);
                nkxCompilerClearReplErrorState(
                    compiler, oldWritePointer);

                // FIXME: Clear out new
                //   NKCompilerStateContextVariables from root
                //   NKCompilerStateContext!

                // FIXME: Clear out any static memory we used for
                //   those new variables. (This may not be necessary:
                //   I think we need to actually execute code to fill
                //   those.)
            }

            // Abort on allocation failures.
            if(nkxVmHasAllocationFailure(vm)) {
                fprintf(stderr, "error: Allocation failure detected. Aborting.\n");
                break;
            }
        }

    } else {

        // Just run the script.
        nkxCompilerCompileScript(compiler, scriptText, settings.scriptFilename);

    }

    // Done compiling. Finalize everything. This also frees the
    // compiler state.
    nkxCompilerFinalize(compiler);

    // Check for compile errors.
    if(!checkNoErrors(vm)) {
        printErrors(vm);
        nkxVmDelete(vm);
        free(scriptText);
        return 1;
    }

    // Run the program.
    nkxVmIterate(vm, NK_UINT_MAX);

    // Check for runtime errors.
    if(!checkNoErrors(vm)) {
        printErrors(vm);
        nkxVmDelete(vm);
        free(scriptText);
        return 1;
    }

    // Clean up and return success.
    nkxVmDelete(vm);
    free(scriptText);
    return 0;
}

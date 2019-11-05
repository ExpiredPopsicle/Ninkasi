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

#include "settings.h"

#include <string.h>
#include <stdlib.h>

void printHelp(const char *argv0, nkbool isError)
{
    FILE *stream = isError ? stderr : stdout;
    const char *helpText =
        "Usage: %s [options] <filename>\n"
        "Test system for the Ninkasi scripting system.\n"
        "\n"
        "Options:\n"
        "  -c          Compile the file to a .nkb only. Do not execute.\n"
        "  -f <count>  Set the rate of malloc calls before a forced failure.\n"
#if !NK_MALLOC_FAILURE_TEST_MODE
        "              (This feature is disabled in this build!)\n"
#endif // NK_MALLOC_FAILURE_TEST_MODE
        "  -m <bytes>  Set the maximum memory usage in bytes.\n"
        "  -se <count> Set the number of iterations to do before serializing and\n"
        "              deserializing into a new VM.\n"
        "  -ss <count> Set the number of iterations before attempting to shrink the\n"
        "              VM to reduce memory usage.\n"
        "  -ee <num>   Set the return code to use on non-fatal errors. Defauts to 0.\n"
        "              (We expect scripts to fail in fuzzing, but not the VM to\n"
        "              break.)\n"
        "  --help      You just stepped in it.\n"
        "  --          Use this to indicate that the filename may contain a dash so\n"
        "              it does not get confused for an option. No more options may\n"
        "              be specified after this.\n"
        ;
    fprintf(stream, helpText, argv0);
}

#if NK_MALLOC_FAILURE_TEST_MODE
extern nkuint32_t nkiMemFailRate;
#endif // NK_MALLOC_FAILURE_TEST_MODE

nkbool parseCmdLine(int argc, char *argv[], struct Settings *settings)
{
    int i;
    nkbool noMoreSwitches = nkfalse;

    // Set up some nice defaults.
    memset(settings, 0, sizeof(*settings));

    // 16mb default max memory usage.
    settings->maxMemory = 16L * (1L << 20L);

    // Arbitrary serializer test frequency to match what we've been
    // doing.
    settings->serializerTestFrequency = 1100;
    settings->shrinkFrequency = 1024;

    for(i = 1; i < argc; i++) {

        if(strcmp("-c", argv[i]) == 0) {

            settings->compileOnly = nktrue;

        } else if(strcmp("-f", argv[i]) == 0) {

            i++;
            if(i < argc) {
              #if NK_MALLOC_FAILURE_TEST_MODE
                nkiMemFailRate = atol(argv[i]);
              #else
                fprintf(stderr, "malloc failure test mode support is not compiled in! Ignoring -f!\n");
              #endif // NK_MALLOC_FAILURE_TEST_MODE
            } else {
                fprintf(stderr, "Missing parameter for -f.\n");
                return nkfalse;
            }

        } else if(strcmp("-m", argv[i]) == 0) {

            i++;
            if(i < argc) {
                settings->maxMemory = atol(argv[i]);
            } else {
                fprintf(stderr, "Missing parameter for -m.\n");
                return nkfalse;
            }

        } else if(strcmp("-se", argv[i]) == 0) {

            i++;
            if(i < argc) {
                settings->serializerTestFrequency = atol(argv[i]);
            } else {
                fprintf(stderr, "Missing parameter for -se.\n");
                return nkfalse;
            }

        } else if(strcmp("-ss", argv[i]) == 0) {

            i++;
            if(i < argc) {
                settings->shrinkFrequency = atol(argv[i]);
            } else {
                fprintf(stderr, "Missing parameter for -ss.\n");
                return nkfalse;
            }

        } else if(strcmp("-ee", argv[i]) == 0) {

            i++;
            if(i < argc) {
                settings->exitErrorCode = atoi(argv[i]);
            } else {
                fprintf(stderr, "Missing parameter for -ee.\n");
                return nkfalse;
            }

        } else if(strcmp("--help", argv[i]) == 0) {

            printHelp(argv[0], nkfalse);
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

char **splitLines(const char *str, nkuint32_t *lineCount)
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

void scanFileDirectives(const char *script, struct Settings *settings)
{
  #if NK_MALLOC_FAILURE_TEST_MODE
    // First, scan for some directives. We want the random
    // allocation failure rate to be something that AFL can
    // tamper with, so it's stored in the file itself instead
    // of as a command line parameter.
    nkuint32_t lineCount = 0;
    char **lines = splitLines(script, &lineCount);
    nkuint32_t i;
    for(i = 0; i < lineCount; i++) {
        const char *memFailPct = "// #failrate: ";
        if(strlen(lines[i]) >= strlen(memFailPct)) {
            if(memcmp(lines[i], memFailPct, strlen(memFailPct)) == 0) {
                nkiMemFailRate = atol(lines[i] + strlen(memFailPct));
                printf("Setting mem fail rate: " NK_PRINTF_UINT32 "\n", nkiMemFailRate);
            }
        }
    }
    free(lines[0]);
    free(lines);
  #endif // NK_MALLOC_FAILURE_TEST_MODE
}

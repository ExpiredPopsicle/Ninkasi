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

#include "nkcommon.h"

#define DEBUG_SPAM 0

// static nkint32_t nkiDbgIndentLevel = 0;

int nkiDbgWriteLine(const char *fmt, ...)
{
  #if DEBUG_SPAM
    va_list args;
    int ret;
    va_start(args, fmt);
    {
        nkint32_t i;
        for(i = 0; i < nkiDbgIndentLevel; i++) {
            printf("  ");
        }
        printf("\033[2m");
        ret = vprintf(fmt, args);
        printf("\033[0m");
        printf("\n");
    }
    va_end(args);
    return ret;
  #else
    return 0;
  #endif
}

void nkiDbgPush_real(const char *func)
{
    // nkiDbgIndentLevel++;
}

void nkiDbgPop_real(const char *func)
{
    // assert(nkiDbgIndentLevel > 0);
    // nkiDbgIndentLevel--;
}

// ----------------------------------------------------------------------
// State dump stuff.

void nkiDbgDumpRaw(FILE *stream, void *data, nkuint32_t len)
{
    nkuint32_t i;
    const char *lookup = "0123456789abcdef";
    const char *ptr = (const char *)data;
    for(i = 0; i < len; i++) {
        fputc(lookup[(ptr[i] & 0xf0) >> 4], stream);
        fputc(lookup[(ptr[i] & 0xf)], stream);
    }
}

void nkiDbgDumpState(const struct NKVM *vm, FILE *stream)
{
    nkuint32_t i;

    fprintf(stream, "Errors:\n");
    {
        struct NKError *err = vm->errorState.firstError;
        while(err) {
            fprintf(stream, "  %s\n", err->errorText);
            err = err->next;
        }
    }

    fprintf(stream, "Stack:\n");
    fprintf(stream, "  Capacity:  %u\n", vm->stack.capacity);
    fprintf(stream, "  Size:      %u\n", vm->stack.size);
    fprintf(stream, "  IndexMask: %u\n", vm->stack.indexMask);
    fprintf(stream, "  Elements:\n");
    for(i = 0; i < vm->stack.size; i++) {
        fprintf(stream, "    %4x: ", i);
        nkiDbgDumpRaw(stream, &vm->stack.values[i], sizeof(vm->stack.values[i]));
        fprintf(stream, "\n");
    }

    fprintf(stream, "Statics:\n");
    fprintf(stream, "  staticAddressMask: %u\n", vm->staticAddressMask);
    fprintf(stream, "  Elements:\n");
    for(i = 0; i <= vm->staticAddressMask; i++) {
        fprintf(stream, "    %4x: ", i);
        nkiDbgDumpRaw(stream, &vm->staticSpace[i], sizeof(vm->staticSpace[i]));
        fprintf(stream, "\n");
    }

    fprintf(stream, "Instructions:\n");
    fprintf(stream, "  instructionAddressMask: %u\n", vm->instructionAddressMask);
}


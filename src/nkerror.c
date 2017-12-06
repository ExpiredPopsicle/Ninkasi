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

void nkiErrorStateSetAllocationFailFlag(
    struct NKVM *vm)
{
    vm->errorState.allocationFailure = nktrue;
}

void nkiErrorStateInit(struct NKVM *vm)
{
    vm->errorState.firstError = NULL;
    vm->errorState.lastError = NULL;
    vm->errorState.allocationFailure = nkfalse;
}

void nkiErrorStateDestroy(struct NKVM *vm)
{
    struct NKError *e = vm->errorState.firstError;
    while(e) {
        struct NKError *next = e->next;
        nkiFree(vm, e->errorText);
        nkiFree(vm, e);
        e = next;
    }
    vm->errorState.firstError = NULL;
}

nkbool nkiVmHasErrors(struct NKVM *vm)
{
    return vm->errorState.firstError || vm->errorState.allocationFailure;
}

void nkiAddError(
    struct NKVM *vm,
    nkint32_t lineNumber,
    const char *str)
{
    struct NKError *newError = nkiMalloc(
        vm,
        sizeof(struct NKError));

    newError->next = NULL;
    newError->errorText = NULL;

    // Add error to the error list. We do this first in case the
    // nkiMalloc to create the string fails, so we still have access
    // to the error when it comes time to clean up the VM.
    if(vm->errorState.lastError) {
        vm->errorState.lastError->next = newError;
    }
    vm->errorState.lastError = newError;
    if(!vm->errorState.firstError) {
        vm->errorState.firstError = newError;
    }

    // Now add the string.
    newError->errorText =
        nkiMalloc(vm, strlen(str) + 2 + NK_PRINTF_INTCHARSNEED + 1);

    newError->errorText[0] = 0;
    sprintf(newError->errorText, NK_PRINTF_INT32 ": %s", lineNumber, str);

}

nkuint32_t nkiGetErrorCount(struct NKVM *vm)
{
    nkuint32_t count = 0;
    struct NKError *error = vm->errorState.firstError;

    while(error) {
        count++;
        error = error->next;
    }

    if(vm->errorState.allocationFailure) {
        count++;
    }

    return count;
}

const char *nkiAllocationFailureStr = "Allocation failure.";

nkuint32_t nkiGetErrorLength(struct NKVM *vm)
{
    // Start off with just enough length for a null terminator.
    nkuint32_t length = 1;

    if(vm->errorState.allocationFailure) {

        // Add length of string AND extra space for a \n.
        length += strlen(nkiAllocationFailureStr) + 1;

    } else {

        struct NKError *error = vm->errorState.firstError;

        while(error) {

            // Add length of string AND extra space for a \n.
            length += strlen(error->errorText) + 1;
            error = error->next;

        }

    }

    return length;
}

void nkiGetErrorText(struct NKVM *vm, char *buffer)
{
    buffer[0] = 0;

    if(vm->errorState.allocationFailure) {

        strcat(buffer, nkiAllocationFailureStr);
        strcat(buffer, "\n");

    } else {

        struct NKError *error = vm->errorState.firstError;

        while(error) {

            strcat(buffer, error->errorText);
            strcat(buffer, "\n");
            error = error->next;

        }

    }

}


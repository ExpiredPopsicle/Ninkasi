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
    const char *str)
{
    nkiAddErrorEx(vm, NK_INVALID_VALUE, NK_INVALID_VALUE, str);
}

void nkiAddErrorEx(
    struct NKVM *vm,
    nkuint32_t lineNumber,
    nkuint32_t fileIndex,
    const char *str)
{
    struct NKError *newError = NULL;
    const char *filename = NULL;
    nkuint32_t spaceNeeded = 0;

  #if NK_VM_DEBUG
    if(lineNumber == NK_INVALID_VALUE) {
        struct NKInstruction *inst = &vm->instructions[
            vm->instructionPointer & vm->instructionAddressMask];
        lineNumber = inst->lineNumber;
    }
    if(fileIndex == NK_INVALID_VALUE) {
        struct NKInstruction *inst = &vm->instructions[
            vm->instructionPointer & vm->instructionAddressMask];
        fileIndex = inst->fileIndex;
    }
  #endif

    newError = (struct NKError *)nkiMalloc(
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

    // Determine filename.
    if(fileIndex < vm->sourceFileCount) {
        filename = vm->sourceFileList[fileIndex];
    }

    if(!filename) {
        filename = "<unknown>";
    }

    // Check for overflow on the error itself before we call something like sprintf!
    {
        nkuint32_t fixedSpaceNeeded = NK_PRINTF_UINTCHARSNEED + 16 + 1;
        nkuint32_t errorSpaceNeeded = nkiStrlen(str);
        nkuint32_t fnameSpaceNeeded = nkiStrlen(filename);
        nkbool overflowed = nkfalse;

        spaceNeeded = fixedSpaceNeeded;
        NK_CHECK_OVERFLOW_UINT_ADD(spaceNeeded, errorSpaceNeeded, spaceNeeded, overflowed);
        NK_CHECK_OVERFLOW_UINT_ADD(spaceNeeded, fnameSpaceNeeded, spaceNeeded, overflowed);

        if(overflowed) {
            filename = "<unknown>";
            str = "Unknown error. Address space exhaustion in error reporting. Cannot report error.";
            spaceNeeded = fixedSpaceNeeded + nkiStrlen(filename) + nkiStrlen(str);
        }
    }

    // Now add the string.
    newError->errorText =
        (char *)nkiMalloc(vm, spaceNeeded);

    newError->errorText[0] = 0;
    sprintf(
        newError->errorText,
        "%s:" NK_PRINTF_UINT32 ": %s",
        filename,
        (lineNumber != NK_INVALID_VALUE) ? lineNumber : 0,
        str);
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
        length += nkiStrlen(nkiAllocationFailureStr) + 1;

    } else {

        struct NKError *error = vm->errorState.firstError;

        while(error) {

            // Add length of string AND extra space for a \n.
            length += nkiStrlen(error->errorText) + 1;
            error = error->next;

        }

    }

    return length;
}

void nkiGetErrorText(struct NKVM *vm, char *buffer)
{
    buffer[0] = 0;

    if(vm->errorState.allocationFailure) {

        nkiStrcat(buffer, nkiAllocationFailureStr);
        nkiStrcat(buffer, "\n");

    } else {

        struct NKError *error = vm->errorState.firstError;

        while(error) {

            nkiStrcat(buffer, error->errorText);
            nkiStrcat(buffer, "\n");
            error = error->next;

        }

    }

}


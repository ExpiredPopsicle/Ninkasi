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

#ifndef NINKASI_ERROR_H
#define NINKASI_ERROR_H

#include "nktypes.h"

struct NKVM;

struct NKError
{
    char *errorText;
    struct NKError *next;
};

struct NKErrorState
{
    struct NKError *firstError;
    struct NKError *lastError;

    // Allocation failures are handled separately, as their own flag,
    // because if we're in a situation where an allocation has failed,
    // we might not be in a situation where we can allocate memory for
    // any more error messages at all.
    nkbool allocationFailure;
};

void nkiErrorStateInit(struct NKVM *vm);
void nkiErrorStateDestroy(struct NKVM *vm);

void nkiAddError(
    struct NKVM *vm,
    nkint32_t lineNumber,
    const char *str);

void nkiErrorStateSetAllocationFailFlag(
    struct NKVM *vm);

/// Get whether or not an error has occurred (faster than
/// nkiGetErrorCount).
nkbool nkiVmHasErrors(struct NKVM *vm);

/// Get the number of errors that have occurred. Compile errors and
/// runtime errors are both stored here.
nkuint32_t nkiGetErrorCount(struct NKVM *vm);

nkuint32_t nkiGetErrorLength(struct NKVM *vm);
void nkiGetErrorText(struct NKVM *vm, char *buffer);

#endif

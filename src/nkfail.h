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

#ifndef NINKASI_FAILURE_H
#define NINKASI_FAILURE_H

/// Stick this at the top of public API functions.
#define NK_FAILURE_RECOVERY_DECL()                  \
    jmp_buf catastrophicFailureJmpBuf;              \
    jmp_buf *catastrophicFailureJmpBuf_lastFrame

#define NK_SET_FAILURE_RECOVERY_BASE()          \
    do {                                        \
        catastrophicFailureJmpBuf_lastFrame =   \
            vm->catastrophicFailureJmpBuf;      \
        vm->catastrophicFailureJmpBuf =         \
            &catastrophicFailureJmpBuf;         \
    } while(0)

#define NK_CLEAR_FAILURE_RECOVERY()                 \
    do {                                            \
        vm->catastrophicFailureJmpBuf =             \
            catastrophicFailureJmpBuf_lastFrame;    \
    } while(0)

/// Stick this before you do anything important in a function that
/// returns something. x is the value that will be returned on error.
#define NK_SET_FAILURE_RECOVERY(x)              \
    do {                                        \
        NK_SET_FAILURE_RECOVERY_BASE();         \
        if(vm->errorState.allocationFailure ||  \
            setjmp(catastrophicFailureJmpBuf))  \
        {                                       \
            NK_CLEAR_FAILURE_RECOVERY();        \
            return (x);                         \
        }                                       \
    } while(0)

/// Stick this before you do anything important in a function that
/// returns nothing.
#define NK_SET_FAILURE_RECOVERY_VOID()          \
    do {                                        \
        NK_SET_FAILURE_RECOVERY_BASE();         \
        if(vm->errorState.allocationFailure ||  \
            setjmp(catastrophicFailureJmpBuf))  \
        {                                       \
            NK_CLEAR_FAILURE_RECOVERY();        \
            return;                             \
        }                                       \
    } while(0)

/// Catastrophic failure handler trigger. Used by the allocator to
/// handle out-of-memory situations.
#define NK_CATASTROPHE()                            \
    do {                                            \
        nkiErrorStateSetAllocationFailFlag(vm);     \
        longjmp(*vm->catastrophicFailureJmpBuf, 1); \
    } while(0)

#define NK_CHECK_CATASTROPHE() \
    (vm ? vm->errorState.allocationFailure : nktrue)

#endif

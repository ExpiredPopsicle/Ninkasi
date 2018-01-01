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

// TODO: Phase out this entire system.

#ifndef NINKASI_VMDBG_H
#define NINKASI_VMDBG_H

#include <stdio.h>

struct NKVMTable;

int nkiDbgWriteLine(const char *fmt, ...);

void nkiDbgDumpState(struct NKVM *vm, FILE *stream);

// FIXME: Move this into nkx.

/// Dump the entire state of the VM. For comparison when testing
/// serialized data.
void nkxDbgDumpState(struct NKVM *vm, FILE *stream);

/// Verify that the holes in the string table match the entries that
/// are actually NULL, but no more and no less.
void nkiCheckStringTableHoles(struct NKVM *vm);

/// Dump the contents of the string table to stdout for debugging.
void nkiVmStringTableDump(struct NKVM *vm);

/// Dump the contents of the object table to stdout for debugging.
void nkiVmObjectTableDump(struct NKVM *vm);

/// Dump the contents of the static area to stdout for debugging.
void nkiVmStaticDump(struct NKVM *vm);

/// Dump the contents of the stack to stdout for debugging.
void nkiVmStackDump(struct NKVM *vm);

/// Dump a value to stdout for debugging purposes.
nkbool nkiValueDump(struct NKVM *vm, struct NKValue *value);

/// Check that every object in the object table has an index value
/// that matches its actual index.
void nkiVmObjectTableSanityCheck(struct NKVM *vm);

#endif // NINKASI_VMDBG_H


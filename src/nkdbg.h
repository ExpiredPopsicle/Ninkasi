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

// TODO: Phase out this entire system.

#ifndef NINKASI_VMDBG_H
#define NINKASI_VMDBG_H

#include <stdio.h>

struct NKVMTable;

/// Dump the entire (known) state of the VM. For comparison when
/// testing serialized data.
void nkiDbgDumpState(struct NKVM *vm, FILE *stream);

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
nkbool nkiValueDump(struct NKVM *vm, struct NKValue *value, FILE *stream);

/// Check that every object in the object table has an index value
/// that matches its actual index.
void nkiVmObjectTableSanityCheck(struct NKVM *vm);

/// Check that every object in the object table is in the linked list
/// if it's has outstanding external handles, or not in the linked
/// list if it does not.
void nkiExternalHandleSanityCheck(struct NKVM *vm);

/// Dump out a disassembly of the program to stdout. script can be
/// NULL, but it can be used to show assembly and code side-by-side
/// (NK_VM_DEBUG must be set to 1 for this to work).
void nkiDbgDumpListing(struct NKVM *vm, const char *script, FILE *stream);

#endif // NINKASI_VMDBG_H


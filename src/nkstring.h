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

#ifndef NINKASI_VMSTRING_H
#define NINKASI_VMSTRING_H

#include "nktypes.h"

struct NKVMString
{
    struct NKVMString *nextInHashBucket;
    nkuint32_t stringTableIndex;
    nkuint32_t lastGCPass;
    nkbool dontGC;
    nkuint32_t hash;

    // Must be last. We're going to allocated VMStrings with enough
    // extra space that we can treat this array as an
    // arbitrarily-sized one, with the data extending off the end of
    // the structure.
    char str[1];
};

struct NKVMStringTableHole
{
    struct NKVMStringTableHole *next;
    nkuint32_t index;
};

#define nkiVmStringHashTableSize 256

struct NKVMStringTable
{
    struct NKVMString *stringsByHash[nkiVmStringHashTableSize];

    struct NKVMStringTableHole *tableHoles;

    struct NKVMString **stringTable;
    nkuint32_t stringTableCapacity;
};

void nkiVmStringTableInit(struct NKVM *vm);
void nkiVmStringTableDestroy(struct NKVM *vm);

struct NKVMString *nkiVmStringTableGetEntryById(
    struct NKVMStringTable *table,
    nkuint32_t index);

const char *nkiVmStringTableGetStringById(
    struct NKVMStringTable *table,
    nkuint32_t index);

nkuint32_t nkiVmStringTableFindOrAddString(
    struct NKVM *vm,
    const char *str);

void nkiVmStringTableDump(struct NKVMStringTable *table);

void nkiVmStringTableCleanOldStrings(
    struct NKVM *vm,
    nkuint32_t lastGCPass);

#endif // NINKASI_VMSTRING_H


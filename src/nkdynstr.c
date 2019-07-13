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

struct NKDynString *nkiDynStrCreate(struct NKVM *vm, const char *str)
{
    struct NKDynString *ret = (struct NKDynString *)nkiMalloc(
        vm, sizeof(struct NKDynString) + 1);
    ret->vm = vm;
    ret->data = nkiStrdup(vm, str ? str : "<null>");
    return ret;
}

void nkiDynStrDelete(struct NKDynString *dynStr)
{
    nkiFree(dynStr->vm, dynStr->data);
    nkiFree(dynStr->vm, dynStr);
}

void nkiDynStrAppend(struct NKDynString *dynStr, const char *str)
{
    nkuint32_t len1;
    nkuint32_t len2;

    if(!str) {
        str = "<null>";
    }

    len1 = nkiStrlen(dynStr->data);
    len2 = nkiStrlen(str);

    // Truncate the lengths down so we don't go over 2^32.
    if(len2 >= ~(nkuint32_t)0 - len1) {
        len2 = ~(nkuint32_t)0 - len1 - 1;
        nkiAddError(
            dynStr->vm,
            "Tried to append a string past length limit. String truncated.");
    }

    dynStr->data = (char *)nkiRealloc(
        dynStr->vm,
        dynStr->data,
        len1 + len2 + 1);

    nkiStrcat(dynStr->data, str);
}

void nkiDynStrAppendInt32(struct NKDynString *dynStr, nkint32_t value)
{
    // +1 for terminator, +1 for '-'.
    char tmp[NK_PRINTF_INTCHARSNEED + 1];
    sprintf(tmp,
        NK_PRINTF_INT32,
        value);
    nkiDynStrAppend(dynStr, tmp);
}

void nkiDynStrAppendUint32(struct NKDynString *dynStr, nkuint32_t value)
{
    // +1 for terminator, +1 for '-'.
    char tmp[NK_PRINTF_UINTCHARSNEED + 1];
    sprintf(tmp,
        NK_PRINTF_UINT32,
        value);
    nkiDynStrAppend(dynStr, tmp);
}

void nkiDynStrAppendFloat(struct NKDynString *dynStr, float value)
{
    // +1 for terminator, +1 for '-', +1 for '.'.
    // +a lot because float.
    char tmp[NK_PRINTF_FLOATCHARSNEEDED + 1];
    sprintf(tmp, "%f", value);
    nkiDynStrAppend(dynStr, tmp);
}


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

#ifndef NINKASI_BASETYPE_H
#define NINKASI_BASETYPE_H

// We should add more 16-bit and DOS configurations as we try out
// different compilers.

#ifdef __DOS__
#   if defined(M_I86)
        // 16-bit DOS under the Watcom compiler.
#       define NK_16BIT
#   elif defined(M_I386)
        // 32-bit DOS.
#       define NK_32BIT
#   endif
#else
    // Other - assume 32-bit.
#   define NK_32BIT
#endif

#if defined(NK_32BIT)
    typedef unsigned int nkuint32_t;
    typedef int nkint32_t;
    typedef unsigned char nkbool;
#   define NK_PRINTF_INT32 "%d"
#   define NK_PRINTF_UINT32 "%u"
#else
    typedef unsigned long int nkuint32_t;
    typedef long int nkint32_t;
    typedef unsigned char nkbool;
#   define NK_PRINTF_INT32 "%ld"
#   define NK_PRINTF_UINT32 "%lu"
#endif

#define nkfalse ((nkbool)0)
#define nktrue ((nkbool)1)

#define NK_INVALID_VALUE (~(nkuint32_t)0)
#define NK_UINT_MAX (~(nkuint32_t)0)

#endif // NINKASI_BASETYPE_H


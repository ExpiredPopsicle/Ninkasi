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

#ifndef NINKASI_BASETYPE_H
#define NINKASI_BASETYPE_H

#include <limits.h>

// ----------------------------------------------------------------------
// Determine data model so we can figure out which type is really a
// 32-bit int and 32-bit unsigned int.

#if !defined(NKPP_32BIT) && !defined(NKPP_16BIT)

#  if INT_MAX == 0x7fffffff && UINT_MAX == 0xffffffff

     // ILP32 data model. Determined from INT_MAX and UINT_MAX
#    define NKPP_32BIT

#  elif defined(__ILP32__)

     // ILP32 data model. The compiler actually tells us.
#    define NKPP_32BIT

#  elif defined(__LP32__)

#    define NKPP_16BIT

#  elif defined(__LP64__)

     // LP64 data model. "int" and "unsigned int" are still 32-bit, and
     // that's all we care about here.
#    define NKPP_32BIT

#  elif defined(__LLP64__)

     // LLP64 data model. It's like LP64, but "long" is 32-bit and "long
     // long" is 64-bit.
#    define NKPP_32BIT

#  elif defined(__ILP64__)

     // We can figure this out later.
#    error "Unsupported data model: ILP4"

#  elif defined(__DOS__)

     // We're not using a modern compiler or a modern OS here. Try to
     // figure out the data model by some other means.

#    if defined(M_I86)

       // 16-bit DOS under the Watcom compiler.
#      define NKPP_16BIT

#    elif defined(M_I386)

       // 32-bit DOS.
#      define NKPP_32BIT

#    endif

#  else

     // Our platorm detection has failed. Hopefully it's something
     // where "int" and "unsigned int" are 32-bits.
     //
     // Add more cases here if we some day have more ways to
     // differentiate.
#    define NKPP_32BIT

#  endif

#endif // !defined(NKPP_32BIT) && !defined(NKPP_16BIT)

// ----------------------------------------------------------------------
// Actual type definitions

// Ints.
#ifdef NKPP_32BIT
#  ifndef nkuint32_t
#    define nkuint32_t unsigned int
#  endif
#  ifndef nkint32_t
#    define nkint32_t signed int
#  endif
#  ifndef NK_PRINTF_INT32
#    define NK_PRINTF_INT32 "%d"
#  endif
#  ifndef NK_PRINTF_UINT32
#    define NK_PRINTF_UINT32 "%u"
#  endif
#elif defined(NKPP_16BIT)
#  ifndef nkuint32_t
#    define nkuint32_t unsigned long
#  endif
#  ifndef nkint32_t
#    define nkint32_t signed long
#  endif
#  ifndef NK_PRINTF_INT32
#    define NK_PRINTF_INT32 "%ld"
#  endif
#  ifndef NK_PRINTF_UINT32
#    define NK_PRINTF_UINT32 "%lu"
#  endif
#else
#  error "No data model detected!"
#endif

#ifndef NK_PRINTF_UINTCHARSNEED
#  define NK_PRINTF_UINTCHARSNEED 11
#endif

#ifndef NK_PRINTF_INTCHARSNEED
#  define NK_PRINTF_INTCHARSNEED 12
#endif

#ifndef NK_INVALID_VALUE
#  define NK_INVALID_VALUE (~(nkuint32_t)0)
#endif

#ifndef NK_UINT_MAX
#  define NK_UINT_MAX (~(nkuint32_t)0)
#endif

// Bytes.
#if CHAR_BIT != 8
#  error "Ninkasi cannot be compiled on any platform where the 'char' type is not 8 bits."
#endif

#ifndef nkuint8_t
#  define nkuint8_t unsigned char
#endif

// Booleans.
#ifndef nkbool
#  define nkbool nkuint8_t
#endif

#ifndef nktrue
#  define nktrue ((nkbool)1)
#endif

#ifndef nkfalse
#  define nkfalse ((nkbool)0)
#endif

// Floats.
#ifndef NK_PRINTF_FLOATCHARSNEEDED
#  define NK_PRINTF_FLOATCHARSNEEDED 48
#endif

// ----------------------------------------------------------------------
// Overflow checks.

#define NK_CHECK_OVERFLOW_UINT_ADD(a, b, result, overflow)  \
    do {                                                    \
        if((a) > NK_UINT_MAX - (b)) {                       \
            overflow = nktrue;                              \
        }                                                   \
        result = a + b;                                     \
    } while(0)

#define NK_CHECK_OVERFLOW_UINT_MUL(a, b, result, overflow)  \
    do {                                                    \
        if((a) >= NK_UINT_MAX / (b)) {                      \
            overflow = nktrue;                              \
        }                                                   \
        result = a * b;                                     \
    } while(0)

// ----------------------------------------------------------------------
// Ninkasi-specific forward defines.

struct NKVM;
struct NKValue;
struct NKVMFunctionCallbackData;
struct NKCompilerState;
struct NKVMGCState;

typedef void (*NKVMFunctionCallback)(struct NKVMFunctionCallbackData *data);
typedef nkbool (*NKVMSerializationWriter)(void *data, nkuint32_t size, void *userdata, nkbool writeMode);
typedef void (*NKVMSubsystemCleanupCallback)(struct NKVM *vm, void *internalData);
typedef void (*NKVMSubsystemSerializationCallback)(struct NKVM *vm, void *internalData);
typedef void (*NKVMExternalObjectCleanupCallback)(
    struct NKVM *vm, struct NKValue *value, void *internalData);
typedef void (*NKVMExternalObjectSerializationCallback)(
    struct NKVM *vm, struct NKValue *value, void *internalData);

typedef void (*NKVMExternalObjectGCMarkCallback)(
    struct NKVM *vm,
    struct NKValue *value,
    void *internalData,
    struct NKVMGCState *gcState);

#endif // NINKASI_BASETYPE_H


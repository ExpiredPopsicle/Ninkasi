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

#endif // NINKASI_BASETYPE_H


#ifndef NINKASI_BASETYPE_H
#define NINKASI_BASETYPE_H

#ifdef __DOS__

#if defined(M_I86)
// 16-bit DOS.

#define NK_16BIT

#elif defined(M_I386)

// 32-bit DOS.
#define NK_32BIT

#endif

#else

// Other - assume 32-bit.
#define NK_32BIT

#endif

#if defined(NK_32BIT)
typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned char bool;
#define NK_PRINTF_INT32 "%d"
#define NK_PRINTF_UINT32 "%u"
#else
typedef unsigned long int uint32_t;
typedef long int int32_t;
typedef unsigned char bool;
#define NK_PRINTF_INT32 "%ld"
#define NK_PRINTF_UINT32 "%lu"
#endif

#define false ((bool)0)
#define true ((bool)1)

#endif // NINKASI_BASETYPE_H


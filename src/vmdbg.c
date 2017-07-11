#include "common.h"

#define DEBUG_SPAM 1

static int32_t dbgIndentLevel = 0;
int dbgWriteLine(const char *fmt, ...)
{
  #if DEBUG_SPAM
    va_list args;
    int ret;
    va_start(args, fmt);
    {
        int32_t i;
        for(i = 0; i < dbgIndentLevel; i++) {
            printf("  ");
        }
        printf("\033[2m");
        ret = vprintf(fmt, args);
        printf("\033[0m");
        printf("\n");
    }
    va_end(args);
    return ret;
  #else
    return 0;
  #endif
}

void dbgPush_real(const char *func)
{
    dbgIndentLevel++;
}

void dbgPop_real(const char *func)
{
    assert(dbgIndentLevel > 0);
    dbgIndentLevel--;
}

// void *wrapMalloc(uint32_t size)
// {
//     void *v = malloc(size);
//     printf("Malloc: %p\n", v);
//     return v;
// }
// void wrapFree(void *v)
// {
//     printf("Free:   %p\n", v);
//     free(v);
// }
// #define malloc(x) wrapMalloc(x)
// #define free(x) wrapFree(x)


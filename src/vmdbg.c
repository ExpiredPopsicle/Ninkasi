#include "common.h"

#define DEBUG_SPAM 0

// static nkint32_t nkiDbgIndentLevel = 0;

int nkiDbgWriteLine(const char *fmt, ...)
{
  #if DEBUG_SPAM
    va_list args;
    int ret;
    va_start(args, fmt);
    {
        nkint32_t i;
        for(i = 0; i < nkiDbgIndentLevel; i++) {
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

void nkiDbgPush_real(const char *func)
{
    // nkiDbgIndentLevel++;
}

void nkiDbgPop_real(const char *func)
{
    // assert(nkiDbgIndentLevel > 0);
    // nkiDbgIndentLevel--;
}


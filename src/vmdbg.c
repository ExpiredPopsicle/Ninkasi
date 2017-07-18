#include "common.h"

#define DEBUG_SPAM 0

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


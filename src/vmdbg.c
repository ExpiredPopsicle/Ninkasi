#include "common.h"

static int32_t dbgIndentLevel = 0;
int dbgWriteLine(const char *fmt, ...)
{
    va_list args;
    int ret;
    va_start(args, fmt);
    {
        int32_t i;
        for(i = 0; i < dbgIndentLevel; i++) {
            printf("  ");
        }
        ret = vprintf(fmt, args);
        printf("\n");
    }
    va_end(args);
    return ret;
}

void dbgPush(void)
{
    dbgIndentLevel++;
}

void dbgPop(void)
{
    dbgIndentLevel--;
}

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
        printf("\033[2m");
        ret = vprintf(fmt, args);
        printf("\033[0m");
        printf("\n");
    }
    va_end(args);
    return ret;
}

void dbgPush_real(const char *func)
{
    dbgIndentLevel++;

    // {
    //     uint32_t i;
    //     for(i = 0; i < dbgIndentLevel; i++) {
    //         printf("  ");
    //     }
    // }
    // printf("dbgIndent PUSH: %d %s\n", dbgIndentLevel, func);
}

void dbgPop_real(const char *func)
{
    assert(dbgIndentLevel > 0);

    // {
    //     uint32_t i;
    //     for(i = 0; i < dbgIndentLevel; i++) {
    //         printf("  ");
    //     }
    // }
    // printf("dbgIndent POP:  %d %s\n", dbgIndentLevel, func);

    dbgIndentLevel--;
}

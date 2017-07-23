#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <malloc.h>

void stackdump(void)
{
    const int stackLength = 256;
    void *stackArray[stackLength];
    char **symbols;
    size_t size;
    unsigned int i;

    size = backtrace(stackArray, stackLength);
    symbols = backtrace_symbols(stackArray, size);

    for(i = 0; i < size && symbols; i++) {
        printf("%4u : %s\n", i, symbols[i]);
    }

    free(symbols);
}

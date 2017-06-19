#ifndef VMDBG_H
#define VMDBG_H

int dbgWriteLine(const char *fmt, ...);
void dbgPush(void);
void dbgPop(void);

#endif


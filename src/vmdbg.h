#ifndef VMDBG_H
#define VMDBG_H

int dbgWriteLine(const char *fmt, ...);
void dbgPush_real(const char *func);
void dbgPop_real(const char *func);

// #define dbgPush() do { dbgPush_real(__FUNCTION__); } while(0)
// #define dbgPop() do { dbgPop_real(__FUNCTION__); } while(0)
#define dbgPush() do { dbgPush_real(""); } while(0)
#define dbgPop() do { dbgPop_real(""); } while(0)

// #define dbgWriteLine(...) do { } while(0)

#endif


// TODO: Phase out this entire system.

#ifndef NINKASI_VMDBG_H
#define NINKASI_VMDBG_H

int nkiDbgWriteLine(const char *fmt, ...);
void nkiDbgPush_real(const char *func);
void nkiDbgPop_real(const char *func);

// #define nkiDbgPush() do { nkiDbgPush_real(__FUNCTION__); } while(0)
// #define nkiDbgPop() do { nkiDbgPop_real(__FUNCTION__); } while(0)
#define nkiDbgPush() do { nkiDbgPush_real(""); } while(0)
#define nkiDbgPop() do { nkiDbgPop_real(""); } while(0)

#endif // NINKASI_VMDBG_H


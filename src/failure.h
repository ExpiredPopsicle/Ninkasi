#ifndef NINKASI_FAILURE_H
#define NINKASI_FAILURE_H

/// Stick this at the top of public API functions.
#define NK_FAILURE_RECOVERY_DECL()              \
    jmp_buf catastrophicFailureJmpBuf_backup

#define NK_SET_FAILURE_RECOVERY_BASE()              \
    do {                                            \
        memcpy(&catastrophicFailureJmpBuf_backup,   \
            vm->catastrophicFailureJmpBuf,          \
            sizeof(vm->catastrophicFailureJmpBuf)); \
    } while(0)

#define NK_CLEAR_FAILURE_RECOVERY()                 \
    do {                                            \
        memcpy(vm->catastrophicFailureJmpBuf,       \
            &catastrophicFailureJmpBuf_backup,      \
            sizeof(vm->catastrophicFailureJmpBuf)); \
    } while(0)

/// Stick this before you do anything important in a function that
/// returns something. x is the value that will be returned on error.
#define NK_SET_FAILURE_RECOVERY(x)                  \
    do {                                            \
        NK_SET_FAILURE_RECOVERY_BASE();             \
        if(vm->errorState.allocationFailure ||      \
            setjmp(vm->catastrophicFailureJmpBuf))  \
        {                                           \
            NK_CLEAR_FAILURE_RECOVERY();            \
            return (x);                             \
        }                                           \
    } while(0)

/// Stick this before you do anything important in a function that
/// returns nothing.
#define NK_SET_FAILURE_RECOVERY_VOID()              \
    do {                                            \
        NK_SET_FAILURE_RECOVERY_BASE();             \
        if(vm->errorState.allocationFailure ||      \
            setjmp(vm->catastrophicFailureJmpBuf))  \
        {                                           \
            NK_CLEAR_FAILURE_RECOVERY();            \
            return;                                 \
        }                                           \
    } while(0)

/// Catastrophic failure handler trigger. Used by the allocator to
/// handle out-of-memory situations.
#define NK_CATASTROPHE()                            \
    do {                                            \
        nkiErrorStateSetAllocationFailFlag(vm);     \
        printf("Catastrophic failure!\n");          \
        stackdump();                                \
        longjmp(vm->catastrophicFailureJmpBuf, 1);  \
    } while(0)

#define NK_CHECK_CATASTROPHE() \
    (vm ? vm->errorState.allocationFailure : nktrue)

void stackdump(void);

#endif

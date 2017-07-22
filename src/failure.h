#ifndef NINKASI_FAILURE_H
#define NINKASI_FAILURE_H

#define NK_FAILURE_RECOVERY_DECL()              \
    jmp_buf catastrophicFailureJmpBuf_backup;

#define NK_SET_FAILURE_RECOVERY_BASE()          \
    memcpy(&catastrophicFailureJmpBuf_backup,   \
        vm->catastrophicFailureJmpBuf,          \
        sizeof(vm->catastrophicFailureJmpBuf));

#define NK_SET_FAILURE_RECOVERY(x)              \
    NK_SET_FAILURE_RECOVERY_BASE();             \
    if(setjmp(vm->catastrophicFailureJmpBuf)) { \
        return x;                               \
    }

#define NK_SET_FAILURE_RECOVERY_VOID()          \
    NK_SET_FAILURE_RECOVERY_BASE();             \
    if(setjmp(vm->catastrophicFailureJmpBuf)) { \
        return;                                 \
    }

#define NK_CLEAR_FAILURE_RECOVERY()             \
    memcpy(vm->catastrophicFailureJmpBuf,       \
        &catastrophicFailureJmpBuf_backup,      \
        sizeof(vm->catastrophicFailureJmpBuf));

#define NK_CATASTROPHY()                        \
    vm->errorState.allocationFailure = true;    \
    longjmp(vm->catastrophicFailureJmpBuf, 1);

#endif

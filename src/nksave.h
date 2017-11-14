#ifndef NINKASI_SAVE_H
#define NINKASI_SAVE_H

#include "nktypes.h"

nkbool nkiVmSerialize(
    struct NKVM *vm,
    NKVMSerializationWriter writer,
    void *userdata,
    nkbool writeMode);

void nkiVmShrink(struct NKVM *vm);

#endif // NINKASI_SAVE_H

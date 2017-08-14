#ifndef NINKASI_VALUE_H
#define NINKASI_VALUE_H

#include "nkenums.h"

struct NKVM;

/// Basic "value" type. This can represent an integer, float, pointer
/// to an object, pointer to a string, or function ID. It does no
/// reference counting, and it is safe to instantiate outside of the
/// VM, or copy the structure in its entirety from a VM-owned
/// instance.
///
/// (Note: The garbage collector will not see NKValues that are not
/// owned by the VM. Use nkxVmObjectAcquireHandle() to keep the VM
/// from garbage collecting objects that are potentially only tracked
/// outside of the VM.)
struct NKValue
{
    enum NKValueType type;
    nkuint32_t lastGCPass;

    union
    {
        nkint32_t intData;
        float floatData;
        nkuint32_t stringTableEntry;
        nkuint32_t functionId;
        nkuint32_t objectId;

        // Used for ints, floats, functionIds, and objectIds. NOT for
        // strings. String hashes are stored in the corresponding
        // entry in the string table.
        nkuint32_t basicHashValue;
    };
};

/// Dump a value to stdout for debugging purposes.
nkbool nkiValueDump(struct NKVM *vm, struct NKValue *value);

const char *valueTypeGetName(enum NKValueType type);

nkint32_t valueToInt(struct NKVM *vm, struct NKValue *value);

float valueToFloat(struct NKVM *vm, struct NKValue *value);

// Returns a string for a value, possibly converting internally.
// Values are only guaranteed to be valid until the next garbage
// collection pass.
const char *valueToString(struct NKVM *vm, struct NKValue *value);

// The return of this value is like strcmp(). -1 for less, 0 for
// equal, 1 for greater-than. Set strictType to nktrue to force a
// comparison failure when types differ. You MUST do this for things
// like binary trees to ensure things have a consistent order.
nkint32_t value_compare(
    struct NKVM *vm,
    struct NKValue *in1,
    struct NKValue *in2,
    nkbool strictType);

nkuint32_t valueHash(struct NKVM *vm, struct NKValue *value);

void vmValueSetInt(struct NKVM *vm, struct NKValue *value, nkint32_t intData);
void vmValueSetFloat(struct NKVM *vm, struct NKValue *value, float floatData);
void vmValueSetString(struct NKVM *vm, struct NKValue *value, const char *str);

#endif // NINKASI_VALUE_H


#ifndef NINKASI_VMSTACK_H
#define NINKASI_VMSTACK_H

struct NKValue;
struct NKVM;

struct NKVMStack
{
    struct NKValue *values;
    nkuint32_t size;
    nkuint32_t capacity;
    nkuint32_t indexMask;
};

/// Push an integer onto the stack.
nkbool nkiVmStackPushInt(struct NKVM *vm, nkint32_t value);

/// Push a float onto the stack.
nkbool nkiVmStackPushFloat(struct NKVM *vm, float value);

/// Push a string onto the stack.
nkbool nkiVmStackPushString(struct NKVM *vm, const char *str);

/// Pop something off the stack and return it. NOTE: Returns a pointer
/// to the object on the stack, which will be overwritten the next
/// time you push something!
struct NKValue *nkiVmStackPop(struct NKVM *vm);
void nkiVmStackPopN(struct NKVM *vm, nkuint32_t count);

/// Fetch a value from an arbitrary stack position, masked to the
/// stack's current range.
struct NKValue *nkiVmStackPeek(struct NKVM *vm, nkuint32_t index);

/// Dump the contents of the stack to stdout for debugging.
void nkiVmStackDump(struct NKVM *vm);

/// Init the stack on the VM object.
void nkiVmStackInit(struct NKVM *vm);

/// Tear down the stack on the VM object.
void nkiVmStackDestroy(struct NKVM *vm);

/// Pushes a new value and returns a pointer to it (so the caller may
/// fill it in).
struct NKValue *nkiVmStackPush_internal(struct NKVM *vm);

#endif // NINKASI_VMSTACK_H


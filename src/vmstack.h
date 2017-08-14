#ifndef NINKASI_VMSTACK_H
#define NINKASI_VMSTACK_H

struct NKValue;
struct NKVM;
struct NKVMStack;

nkbool nkiVmStackPushInt(struct NKVM *vm, nkint32_t value);
nkbool nkiVmStackPushFloat(struct NKVM *vm, float value);
nkbool nkiVmStackPushString(struct NKVM *vm, const char *str);

/// Pop something off the stack and return it. NOTE: Returns a pointer
/// to the object on the stack, which will be overwritten the next
/// time you push something!
struct NKValue *nkiVmStackPop(struct NKVM *vm);
void nkiVmStackPopN(struct NKVM *vm, nkuint32_t count);

struct NKValue *nkiVmStackPeek(struct NKVM *vm, nkuint32_t index);

void nkiVmStackDump(struct NKVM *vm);

struct NKVMStack
{
    struct NKValue *values;
    nkuint32_t size;
    nkuint32_t capacity;
    nkuint32_t indexMask;
};

void nkiVmStackInit(struct NKVM *vm);
void nkiVmStackDestroy(struct NKVM *vm);

/// Pushes a new value and returns a pointer to it (so the caller may
/// fill it in).
struct NKValue *nkiVmStackPush_internal(struct NKVM *vm);

#endif // NINKASI_VMSTACK_H


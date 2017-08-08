#ifndef NINKASI_VMSTACK_H
#define NINKASI_VMSTACK_H

struct NKValue;
struct NKVM;
struct NKVMStack;

// ----------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------

bool vmStackPushInt(struct NKVM *vm, int32_t value);
bool vmStackPushFloat(struct NKVM *vm, float value);
bool vmStackPushString(struct NKVM *vm, const char *str);

/// Pop something off the stack and return it. NOTE: Returns a pointer
/// to the object on the stack, which will be overwritten the next
/// time you push something!
struct NKValue *vmStackPop(struct NKVM *vm);
void vmStackPopN(struct NKVM *vm, uint32_t count);

struct NKValue *vmStackPeek(struct NKVM *vm, uint32_t index);

void vmStackDump(struct NKVM *vm);

// ----------------------------------------------------------------------
// Internal structures and functions
// ----------------------------------------------------------------------

struct NKVMStack
{
    struct NKValue *values;
    uint32_t size;
    uint32_t capacity;
    uint32_t indexMask;
};

void vmStackInit(struct NKVM *vm);
void vmStackDestroy(struct NKVM *vm);

/// Pushes a new value and returns a pointer to it (so the caller may
/// fill it in).
struct NKValue *vmStackPush_internal(struct NKVM *vm);

#endif // NINKASI_VMSTACK_H


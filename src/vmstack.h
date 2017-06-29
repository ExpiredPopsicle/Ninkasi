#ifndef VMSTACK_H
#define VMSTACK_H

struct Value;
struct VM;
struct VMStack;

// ----------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------

bool vmStackPushInt(struct VM *vm, int32_t value);
bool vmStackPushFloat(struct VM *vm, float value);
bool vmStackPushString(struct VM *vm, const char *str);

/// Pop something off the stack and return it. NOTE: Returns a pointer
/// to the object on the stack, which will be overwritten the next
/// time you push something!
struct Value *vmStackPop(struct VM *vm);

struct Value *vmStackPeek(struct VM *vm, uint32_t index);

void vmStackDump(struct VM *vm);

// ----------------------------------------------------------------------
// Internal structures and functions
// ----------------------------------------------------------------------

struct VMStack
{
    struct Value *values;
    uint32_t size;
    uint32_t capacity;
    uint32_t indexMask;
};

void vmStackInit(struct VMStack *stack);
void vmStackDestroy(struct VMStack *stack);

/// Pushes a new value and returns a pointer to it (so the caller may
/// fill it in).
struct Value *vmStackPush_internal(struct VM *vm);

#endif

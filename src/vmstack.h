#ifndef VMSTACK_H
#define VMSTACK_H

struct Value;
struct VM;

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
struct Value *vmStackPush_internal(struct VMStack *stack);

bool vmStackPushInt(struct VMStack *stack, int32_t value);
bool vmStackPushString(struct VM *vm, const char *str);

struct Value *vmStackPop(struct VMStack *stack);

struct Value *vmStackPeek(struct VMStack *stack, uint32_t index);

void vmStackDump(struct VM *vm);

#endif

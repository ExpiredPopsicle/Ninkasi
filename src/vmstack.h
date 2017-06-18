#ifndef VMSTACK_H
#define VMSTACK_H

struct Value;

struct VMStack
{
    struct Value *values;
    uint32_t size;
    uint32_t capacity;
    uint32_t indexMask;
};

void vmstack_init(struct VMStack *stack);
void vmstack_destroy(struct VMStack *stack);
//struct Value *vmstack_push_internal(struct VMStack *stack);
bool vmstack_pushInt(struct VMStack *stack, int32_t value);
struct Value *vmstack_pop(struct VMStack *stack);

struct Value *vmstack_peek(struct VMStack *stack, uint32_t index);

void vmstack_dump(struct VMStack *stack);

#endif

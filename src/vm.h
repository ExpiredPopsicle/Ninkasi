#ifndef VM_H
#define VM_H

struct ErrorState;
struct VMStack;

struct VM
{
    struct ErrorState errorState;
    struct VMStack stack;
};

void vmInit(struct VM *vm);
void vmDestroy(struct VM *vm);

#endif

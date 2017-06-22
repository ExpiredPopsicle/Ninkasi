#ifndef VM_H
#define VM_H

struct ErrorState;
struct VMStack;
struct Instruction;

struct VM
{
    struct ErrorState errorState;
    struct VMStack stack;

    struct Instruction *instructions;
    uint32_t instructionAddressMask;
    uint32_t instructionPointer;
};

void vmInit(struct VM *vm);
void vmDestroy(struct VM *vm);

void vmIterate(struct VM *vm);

#endif

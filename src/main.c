#include "common.h"

// ----------------------------------------------------------------------

int main(int argc, char *argv[])
{
    struct VMStack stack;
    vmstack_init(&stack);

    // struct Value *t1 = vmstack_push(&stack);
    // struct Value *t2 = vmstack_push(&stack);

    // t1->type = VALUETYPE_INT;
    // t2->type = VALUETYPE_INT;
    // t1->intData = 123;
    // t2->intData = 456;

    vmstack_pushInt(&stack, 123);
    vmstack_pushInt(&stack, 456);

    opcode_add(&stack);

    vmstack_pushInt(&stack, 123);
    vmstack_pushInt(&stack, 456);

    vmstack_pushInt(&stack, 123);
    vmstack_pushInt(&stack, 456);

    vmstack_dump(&stack);

    vmstack_destroy(&stack);


    return 0;
}


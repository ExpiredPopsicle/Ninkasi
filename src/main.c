#include "common.h"

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





    printf("Tokenize test...\n");
    struct TokenList tokenList;
    tokenList.first = NULL;
    tokenList.last = NULL;
    {
        bool r = tokenize("(((123 + 456)[1 + 2][3])) * 789 - -100 / ------300", &tokenList);
        // bool r = tokenize("123 + 456 * 789 - -100 / ------300", &tokenList);
        // bool r = tokenize("(123 + 456 * 789) / 0", &tokenList);
        assert(r);
    }
    {
        struct Token *tokenPtr = tokenList.first;
        struct ExpressionAstNode *node = parseExpression(&tokenPtr);
        optimizeConstants(&node);
        dumpExpressionAstNode(node);
        printf("\n");
    }
    destroyTokenList(&tokenList);


    return 0;
}


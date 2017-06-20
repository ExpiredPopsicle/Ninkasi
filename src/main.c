#include "common.h"

int main(int argc, char *argv[])
{
    struct VM vm;

    struct VMStack stack;
    vmStackInit(&stack);

    // struct Value *t1 = vmStackpush(&stack);
    // struct Value *t2 = vmStackpush(&stack);

    // t1->type = VALUETYPE_INT;
    // t2->type = VALUETYPE_INT;
    // t1->intData = 123;
    // t2->intData = 456;

    vmStackPushInt(&stack, 123);
    vmStackPushInt(&stack, 456);

    opcode_add(&stack);

    vmStackPushInt(&stack, 123);
    vmStackPushInt(&stack, 456);

    vmStackPushInt(&stack, 123);
    vmStackPushInt(&stack, 456);

    vmStackDump(&stack);

    vmStackDestroy(&stack);

    {
        struct Instruction instructions[3];

        instructions[0].opcode = OP_PUSHLITERAL;
        instructions[0].pushLiteralData.value.type = VALUETYPE_INT;
        instructions[0].pushLiteralData.value.intData = 32;

        instructions[1].opcode = OP_PUSHLITERAL;
        instructions[1].pushLiteralData.value.type = VALUETYPE_INT;
        instructions[1].pushLiteralData.value.intData = 16;

        instructions[2].opcode = OP_ADD;
    }


    vmInit(&vm);


    printf("Tokenize test...\n");
    {
        struct TokenList tokenList;
        tokenList.first = NULL;
        tokenList.last = NULL;
        {
            bool r = tokenize("(((123\n +\n 456)[1 +\n 2\n]\n[\n3\n])) * 789 - -100 / ------300", &tokenList);
            // bool r = tokenize("123 + 456 * 789 - -100 / ------300", &tokenList);
            // bool r = tokenize("(123 + 456 * 789) / 0", &tokenList);
            // assert(r);

            if(r) {
                struct Token *tokenPtr = tokenList.first;
                struct ExpressionAstNode *node = parseExpression(&vm, &tokenPtr);
                if(node) {
                    optimizeConstants(&node);
                    dumpExpressionAstNode(node);
                    printf("\n");
                    deleteExpressionNode(node);
                }

                // Dump errors.
                if(vm.errorState.firstError) {
                    struct Error *err = vm.errorState.firstError;
                    while(err) {
                        printf("error: %s\n", err->errorText);
                        err = err->next;
                    }
                }
            }
            destroyTokenList(&tokenList);
        }
    }

    vmDestroy(&vm);

    return 0;
}


#include "common.h"

int main(int argc, char *argv[])
{
    struct VM vm;

    // struct VMStack stack;
    // vmStackInit(&stack);

    // struct Value *t1 = vmStackpush(&stack);
    // struct Value *t2 = vmStackpush(&stack);

    // t1->type = VALUETYPE_INT;
    // t2->type = VALUETYPE_INT;
    // t1->intData = 123;
    // t2->intData = 456;




    // vmStackPushInt(&stack, 123);
    // vmStackPushInt(&stack, 456);

    // opcode_add(&stack);

    // vmStackPushInt(&stack, 123);
    // vmStackPushInt(&stack, 456);

    // vmStackPushInt(&stack, 123);
    // vmStackPushInt(&stack, 456);

    // vmStackDump(&stack);

    // vmStackDestroy(&stack);



    vmInit(&vm);

    // {
    //     // vm.instructions =
    //     //     malloc(sizeof(struct Instruction) * 4);
    //     // vm.instructionAddressMask = 0x3;

    //     vm.instructions[0].opcode = OP_PUSHLITERAL;
    //     vm.instructions[0].pushLiteralData.value.type = VALUETYPE_INT;
    //     vm.instructions[0].pushLiteralData.value.intData = 32;

    //     vm.instructions[1].opcode = OP_PUSHLITERAL;
    //     vm.instructions[1].pushLiteralData.value.type = VALUETYPE_INT;
    //     vm.instructions[1].pushLiteralData.value.intData = 16;

    //     vm.instructions[2].opcode = OP_ADD;

    //     vm.instructions[3].opcode = OP_NOP;
    // }

    // vmIterate(&vm);
    // vmIterate(&vm);
    // vmIterate(&vm);
    // vmStackDump(&vm.stack);



    printf("Tokenize test...\n");
    {
        struct TokenList tokenList;
        tokenList.first = NULL;
        tokenList.last = NULL;
        {
            // bool r = tokenize("(((123\n +\n 456)[1 +\n 2\n]\n[\n3\n])) * 789 - -100 / ------300", &tokenList);
            // bool r = tokenize("123 + 456 * 789 - -100 / ------300", &tokenList);
            // bool r = tokenize("123 + 456", &tokenList);
            // bool r = tokenize("123 + 456 * 789", &tokenList);
            // bool r = tokenize("(123 + 456 * 789) / 0", &tokenList);
            // assert(r);

            // bool r = tokenize("123 + 456 * 789 - -100 / 300", &tokenList);

            bool r = tokenize(&vm, "\"foo\"  + \"\\\"bar\\\"\"", &tokenList);

            {
                struct Token *t = tokenList.first;
                while(t) {
                    printf("token(%d): %s\n", t->type, t->str);
                    t = t->next;
                }
            }

            if(r) {
                struct CompilerState cs;
                struct Token *tokenPtr = tokenList.first;

                // struct ExpressionAstNode *node = parseExpression(&vm, &tokenPtr);
                // if(node) {
                //     optimizeConstants(&node);
                //     dumpExpressionAstNode(node);
                //     printf("\n");
                //     deleteExpressionNode(node);
                // }

                tokenPtr = tokenList.first;

                cs.instructionWriteIndex = 0;
                cs.vm = &vm;
                compileExpression(&cs, &tokenPtr);

                // Dump errors.
                if(vm.errorState.firstError) {
                    struct Error *err = vm.errorState.firstError;
                    while(err) {
                        printf("error: %s\n", err->errorText);
                        err = err->next;
                    }
                }

                addInstructionSimple(&cs, OP_NOP);

            }
            destroyTokenList(&tokenList);
        }
    }



    while(vm.instructions[vm.instructionPointer].opcode != OP_NOP) {
        printf("instruction %d: %d\n", vm.instructionPointer, vm.instructions[vm.instructionPointer].opcode);
        // printf("  %d\n", vm.instructions[vm.instructionPointer].opcode);
        vmStackDump(&vm.stack);
        vmIterate(&vm);
    }
    // vmIterate(&vm);
    // vmIterate(&vm);
    // vmIterate(&vm);
    // vmIterate(&vm);
    printf("Final stack dump...\n");
    vmStackDump(&vm.stack);

    vmDestroy(&vm);


    printf("%d\n", 123 + 456 * 789 - -100 / 300);

    return 0;
}


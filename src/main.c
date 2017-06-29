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
            // bool r = tokenize(&vm, "(((123\n +\n 456)[1 +\n 2\n]\n[\n3\n])) * 789 - -100 / ------300", &tokenList);
            // bool r = tokenize("123 + 456 * 789 - -100 / ------300", &tokenList);
            // bool r = tokenize("123 + 456", &tokenList);
            // bool r = tokenize("123 + 456 * 789", &tokenList);
            // bool r = tokenize("(123 + 456 * 789) / 0", &tokenList);
            // assert(r);

            bool r = tokenize(&vm,
                "foo = 123 + 456 * (789 + foo * bar) - -100 / 3;\n"
                "bar = foo + 1 + 2 + 3;",
                // "((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((("
                // "999)))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))));",
                &tokenList);

            // bool r = tokenize(&vm,
            //     "foo = 123;\n",
            //     &tokenList);

            // bool r = tokenize(&vm, "\"foo\" + 1 + \"\\\"bar\\\"\"", &tokenList);
            // bool r = tokenize(&vm, "2.0 * 2.2 -100 ", &tokenList);
            // bool r = tokenize(&vm, "1  + 2", &tokenList);

            // bool r = tokenize(&vm, "\"foo\" \"bar\" \"blah\"", &tokenList);

            // bool r = tokenize(&vm, "this - is + a / test * of + identifiers123", &tokenList);

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
                cs.context = NULL;

                pushContext(&cs);

                addVariable(&cs, "cheese");
                addVariable(&cs, "butts");

                pushContext(&cs);

                addVariable(&cs, "foo");
                addVariable(&cs, "bar");

                while(tokenPtr) {
                    printf("%s\n", tokenPtr->str);
                    if(!compileStatement(&cs, &tokenPtr)) {
                        break;
                    }
                }

                popContext(&cs);
                popContext(&cs);

                // {
                //     uint32_t i;
                //     for(i = 0; i < 5; i++) {
                //         printf("op: %d\n", vm.instructions[i].opcode);
                //     }
                // }

                // Dump errors.
                if(vm.errorState.firstError) {
                    struct Error *err = vm.errorState.firstError;
                    while(err) {
                        printf("error: %s\n", err->errorText);
                        err = err->next;
                    }
                }

                addInstructionSimple(&cs, OP_NOP);

            } else {
                printf("tokenizer failed\n");
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

    if(!vm.errorState.firstError) {

        printf("----------------------------------------------------------------------\n");
        printf("  Execution\n");
        printf("----------------------------------------------------------------------\n");

        while(vm.instructions[vm.instructionPointer].opcode != OP_NOP) {

            // printf("instruction %d: %d\n", vm.instructionPointer, vm.instructions[vm.instructionPointer].opcode);
            // printf("  %d\n", vm.instructions[vm.instructionPointer].opcode);
            vmIterate(&vm);

            if(vm.errorState.firstError) {
                struct Error *err = vm.errorState.firstError;
                while(err) {
                    printf("error: %s\n", err->errorText);
                    err = err->next;
                }
            }

            vmStackDump(&vm);
            vmGarbageCollect(&vm);
        }
    }

    printf("----------------------------------------------------------------------\n");
    printf("  Finish\n");
    printf("----------------------------------------------------------------------\n");

    printf("Final stack...\n");
    vmStackDump(&vm);
    vmGarbageCollect(&vm);
    printf("Final stack again...\n");
    vmStackDump(&vm);

    if(0) {

        printf("----------------------------------------------------------------------\n");
        printf("  String table crap\n");
        printf("----------------------------------------------------------------------\n");

        vmStringTableFindOrAddString(
            &vm.stringTable, "sadf");
        vmStringTableFindOrAddString(
            &vm.stringTable, "sadf");
        vmStringTableFindOrAddString(
            &vm.stringTable, "sadf");
        vmStringTableFindOrAddString(
            &vm.stringTable, "sadf");
        vmStringTableFindOrAddString(
            &vm.stringTable, "sadf");

        vmStringTableFindOrAddString(
            &vm.stringTable, "bladgh");
        vmStringTableFindOrAddString(
            &vm.stringTable, "foom");
        vmStringTableFindOrAddString(
            &vm.stringTable, "dicks");
        vmStringTableFindOrAddString(
            &vm.stringTable, "sadf");

        vmStringTableDump(&vm.stringTable);

        vmStringTableGetEntryById(
            &vm.stringTable,
            vmStringTableFindOrAddString(&vm.stringTable, "sadf"))->lastGCPass = 1234;

        vmStringTableCleanOldStrings(&vm.stringTable, 1234);

        vmStringTableDump(&vm.stringTable);

        vmRescanProgramStrings(&vm);


        // vmIterate(&vm);
        // vmIterate(&vm);
        // vmIterate(&vm);
        // vmIterate(&vm);
        printf("Final stack dump...\n");
        vmStackDump(&vm);
    }

    vmDestroy(&vm);

    return 0;
}


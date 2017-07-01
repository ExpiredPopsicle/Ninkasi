#include "common.h"

char *loadScript(const char *filename)
{
    FILE *in = fopen(filename, "rb");
    uint32_t len;
    char *buf;

    if(!in) {
        return NULL;
    }

    fseek(in, 0, SEEK_END);
    len = ftell(in);
    fseek(in, 0, SEEK_SET);

    buf = malloc(len + 1);
    fread(buf, len, 1, in);
    buf[len] = 0;

    return buf;
}


int main(int argc, char *argv[])
{
    struct VM vm;
    char *script = loadScript("test.txt");
    assert(script);

    vmInit(&vm);

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
                // "{ var butts = 3; foo = 123 + 456 * (789 + foo * bar) - -100 / 3;\n"
                // "bar = foo + 1 + 2 + 3; }",
                // "{"
                // "{ var butts = 3; var dicks = 4; butts = dicks; dicks = 3; }"
                // // "((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((("
                // // "999)))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))));",
                // "{ \"foo\" + \"bar\"; }"
                // "}",
                script,
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

                // Global context.
                pushContext(&cs);

                // addVariable(&cs, "cheese");
                // addVariable(&cs, "butts");

                // pushContext(&cs);

                // addVariable(&cs, "foo");
                // addVariable(&cs, "bar");

                // compileStatement(&cs, &tokenPtr);
                compileBlock(&cs, &tokenPtr, true);

                // while(tokenPtr) {
                //     printf("%s\n", tokenPtr->str);
                //     if(!compileStatement(&cs, &tokenPtr)) {
                //         break;
                //     }
                // }

                // popContext(&cs);
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

                // addInstructionSimple(&cs, OP_NOP);
                // addInstructionSimple(&cs, OP_DUMP);

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
        printf("  Dump\n");
        printf("----------------------------------------------------------------------\n");

        {
            uint32_t i;
            for(i = 0; i <= vm.instructionAddressMask; i++) {

                enum Opcode opcode = vm.instructions[i].opcode;
                struct Instruction *maybeParams =
                    &vm.instructions[(i+1) & vm.instructionAddressMask];

                // Output opcode.
                printf("%4d: %s", i, vmGetOpcodeName(opcode));

                // Output parameters.
                if(vm.instructions[i].opcode == OP_PUSHLITERAL_INT) {
                    i++;
                    printf(" %d", maybeParams->opData_int);
                } else if(vm.instructions[i].opcode == OP_PUSHLITERAL_FLOAT) {
                    i++;
                    printf(" %f", maybeParams->opData_float);
                } else if(vm.instructions[i].opcode == OP_PUSHLITERAL_STRING) {
                    const char *str = vmStringTableGetStringById(&vm.stringTable, maybeParams->opData_string);
                    i++;
                    printf(" %d:%s", maybeParams->opData_string, str ? str : "<bad string>");
                }

                printf("\n");
            }
        }

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

            printf("next instruction %d: %d = %s\n",
                vm.instructionPointer,
                vm.instructions[vm.instructionPointer].opcode,
                vmGetOpcodeName(vm.instructions[vm.instructionPointer].opcode));
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

    free(script);

    return 0;
}


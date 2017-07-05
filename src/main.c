#include "common.h"

char **splitLines(const char *str, uint32_t *lineCount)
{
    char **lines = NULL;

    uint32_t len = strlen(str);
    char *workStr = strdup(str);

    uint32_t i = 0;
    uint32_t lineStart = 0;

    *lineCount = 0;

    for(i = 0; i < len + 1; i++) {

        if(workStr[i] == '\n' || workStr[i] == 0) {
            workStr[i] = 0;

            (*lineCount)++;
            lines = realloc(lines, sizeof(char*) * (*lineCount));
            lines[(*lineCount)-1] = &workStr[lineStart];

            lineStart = i + 1;
        }
    }

    return lines;
}

void dumpListing(struct VM *vm, const char *script)
{
    uint32_t i;

  #if VM_DEBUG
    uint32_t lastLine = 0;
  #endif

    uint32_t lineCount = 0;
    char **lines = script ? splitLines(script, &lineCount) : NULL;

    for(i = 0; i <= vm->instructionAddressMask; i++) {

        enum Opcode opcode = vm->instructions[i].opcode;
        struct Instruction *maybeParams =
            &vm->instructions[(i+1) & vm->instructionAddressMask];

      #if VM_DEBUG

        // while(lastLine < vm->instructions[i].lineNumber && lastLine < lineCount) {
        //     printf("%4u     :                                         ; %s\n", lastLine, lines[lastLine]);
        //     lastLine++;
        // }

        // // Output opcode.
        // printf("%4u %.4u: %s", vm->instructions[i].lineNumber, i, vmGetOpcodeName(opcode));

        // Line-number-less version for diff.
        if(lines) {
            while(lastLine < vm->instructions[i].lineNumber && lastLine < lineCount) {
                printf("                                         ; %s\n", lines[lastLine]);
                lastLine++;
            }
        }
        printf("%s %.4d %s", (i == vm->instructionPointer ? ">" : " "), i, vmGetOpcodeName(opcode));

      #else

        // Output opcode.
        printf("%.4u: %s", i, vmGetOpcodeName(opcode));

      #endif

        // Output parameters.
        if(vm->instructions[i].opcode == OP_PUSHLITERAL_INT) {
            i++;
            printf(" %d", maybeParams->opData_int);
        } else if(vm->instructions[i].opcode == OP_PUSHLITERAL_FLOAT) {
            i++;
            printf(" %f", maybeParams->opData_float);
        } else if(vm->instructions[i].opcode == OP_PUSHLITERAL_FUNCTIONID) {
            i++;
            printf(" %u", maybeParams->opData_functionId);
        } else if(vm->instructions[i].opcode == OP_PUSHLITERAL_STRING) {
            const char *str = vmStringTableGetStringById(&vm->stringTable, maybeParams->opData_string);
            i++;
            // TODO: Escape string before showing here.
            printf(" %d:\"%s\"", maybeParams->opData_string, str ? str : "<bad string>");
        }

        printf("\n");
    }

    if(lines) {
        free(lines[0]);
        free(lines);
    }
}

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

    fclose(in);

    return buf;
}

void testVMFunc(struct VMFunctionCallbackData *data)
{
    // uint32_t i;
    printf("testVMFunc hit!\n");
    // for(i = 0; i < data->argumentCount; i++) {
    //     printf("Argument %d: %s\n", i,
    //         valueToString(data->vm, &data->arguments[i]));
    // }

    data->returnValue.intData = 565656;

    if(data->argumentCount != 1) {
        errorStateAddError(&data->vm->errorState, -1, "Bad argument count in testVMFunc.");
        return;
    }

    vmCallFunction(data->vm, &data->arguments[0], 0, NULL, &data->returnValue);

    printf("Got data back from VM: %s\n", valueToString(data->vm, &data->returnValue));
}


void vmFuncPrint(struct VMFunctionCallbackData *data)
{
    uint32_t i;

    for(i = 0; i < data->argumentCount; i++) {
        printf("\033[1m%s\033[0m", valueToString(data->vm, &data->arguments[i]));
    }
}


int main(int argc, char *argv[])
{
    struct VM vm;

    char *script = loadScript("test.txt");
    uint32_t lineCount = 0;
    char **lines = splitLines(script, &lineCount);
    assert(script);

    vmInit(&vm);

    {
        struct CompilerState *cs = vmCompilerCreate(&vm);
        vmCompilerCreateCFunctionVariable(cs, "cfunc", testVMFunc);
        vmCompilerCreateCFunctionVariable(cs, "print", vmFuncPrint);
        vmCompilerCompileScriptFile(cs, "test.txt");
        vmCompilerFinalize(cs);

        // Dump errors.
        if(vmGetErrorCount(&vm)) {
            struct Error *err = vm.errorState.firstError;
            while(err) {
                printf("error: %s\n", err->errorText);
                err = err->next;
            }
        }
    }

    if(!vm.errorState.firstError) {

        printf("----------------------------------------------------------------------\n");
        printf("  Original script\n");
        printf("----------------------------------------------------------------------\n");

        {
            uint32_t i;
            for(i = 0; i < lineCount; i++) {
                printf("%4u : %s\n", i, lines[i]);
            }
        }

        printf("----------------------------------------------------------------------\n");
        printf("  Dump\n");
        printf("----------------------------------------------------------------------\n");

        dumpListing(&vm, script);

        printf("----------------------------------------------------------------------\n");
        printf("  Execution\n");
        printf("----------------------------------------------------------------------\n");

        if(!vmGetErrorCount(&vm)) {

            // vmExecuteProgram(&vm);

            while(vm.instructions[
                    vm.instructionPointer &
                    vm.instructionAddressMask].opcode != OP_END)
            {
                vmIterate(&vm);

                printf("\n\n\n\n");
                printf("----------------------------------------------------------------------\n");
                printf("PC: %u\n", vm.instructionPointer);
                printf("Stack...\n");
                printf("----------------------------------------------------------------------\n");
                vmStackDump(&vm);
                printf("\n");

                dumpListing(&vm, script);

                if(vm.errorState.firstError) {
                    break;
                } else {
                    getchar();
                }
            }



            {
                struct Value *v = vmFindGlobalVariable(&vm, "readMeFromC");
                if(v) {
                    printf("Value found: %s\n", valueToString(&vm, v));
                } else {
                    printf("Value NOT found.\n");
                }
            }
        }

        // while(vm.instructions[vm.instructionPointer].opcode != OP_END) {

        //     // printf("instruction %d: %d\n", vm.instructionPointer, vm.instructions[vm.instructionPointer].opcode);
        //     // printf("  %d\n", vm.instructions[vm.instructionPointer].opcode);
        //     vmIterate(&vm);

        if(vm.errorState.firstError) {
            struct Error *err = vm.errorState.firstError;
            while(err) {
                printf("error: %s\n", err->errorText);
                err = err->next;
            }
        }

        //     vmStackDump(&vm);
        //     // vmGarbageCollect(&vm);

        //     printf("next at %d\n",
        //         vm.instructionPointer);
        //     printf("next instruction %d: %d = %s\n",
        //         vm.instructionPointer,
        //         vm.instructions[vm.instructionPointer].opcode,
        //         vmGetOpcodeName(vm.instructions[vm.instructionPointer].opcode));
        // }

        // // Function call test.
        // if(!vm.errorState.firstError) {
        //     struct Value retVal;
        //     memset(&retVal, 0, sizeof(retVal));
        //     vmCallFunctionByName(&cs, "callMeFromC", 0, NULL, &retVal);
        // }
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
    free(lines[0]);
    free(lines);

    return 0;
}


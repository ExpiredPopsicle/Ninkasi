#include "common.h"

// ----------------------------------------------------------------------
// Static opcode table setup.

typedef void (*VMOpcodeCall)(struct VM *vm, struct Instruction *instruction);
static VMOpcodeCall opcodeTable[OPCODE_PADDEDCOUNT];
static void vmInitOpcodeTable(void)
{
    assert(OPCODE_PADDEDCOUNT >= OPCODE_REALCOUNT);

    opcodeTable[OP_ADD]         = opcode_add;
    opcodeTable[OP_SUBTRACT]    = opcode_subtract;
    opcodeTable[OP_MULTIPLY]    = opcode_multiply;
    opcodeTable[OP_DIVIDE]      = opcode_divide;
    opcodeTable[OP_NEGATE]      = opcode_negate;
    opcodeTable[OP_PUSHLITERAL] = opcode_pushLiteral;
    opcodeTable[OP_NOP]         = opcode_nop;

    // Fill in the rest of the opcode table with no-ops. We just want
    // to pad up to a power of two so we can easily mask instructions
    // instead of branching to make sure they're valid.
    {
        uint32_t i;
        for(i = OPCODE_REALCOUNT; i < OPCODE_PADDEDCOUNT; i++) {
            opcodeTable[i] = opcode_nop;
        }
    }
}

// ----------------------------------------------------------------------
// Init/shutdown

void vmInit(struct VM *vm)
{
    vmInitOpcodeTable();

    errorStateInit(&vm->errorState);
    vmStackInit(&vm->stack);
    vm->instructionPointer = 0;

    vm->instructions =
        malloc(sizeof(struct Instruction) * 4);
    vm->instructionAddressMask = 0x3;
    memset(vm->instructions, 0, sizeof(struct Instruction) * 4);

    vmStringTableInit(&vm->stringTable);

    vm->lastGCPass = 0;
}

void vmDestroy(struct VM *vm)
{
    vmStringTableDestroy(&vm->stringTable);

    vmStackDestroy(&vm->stack);
    errorStateDestroy(&vm->errorState);
    free(vm->instructions);
}

// ----------------------------------------------------------------------
// Iteration

void vmIterate(struct VM *vm)
{
    struct Instruction *inst = &vm->instructions[
        vm->instructionPointer & vm->instructionAddressMask];
    opcodeTable[inst->opcode & (OPCODE_PADDEDCOUNT - 1)](vm, inst);
    vm->instructionPointer++;
}

// ----------------------------------------------------------------------
// Garbage collection

struct VMValueGCEntry
{
    struct Value *value;
    struct VMValueGCEntry *next;
};

void vmGarbageCollect_markValue(
    struct VM *vm, struct Value *v,
    uint32_t currentGCPass)
{
    struct VMValueGCEntry *openList;

    if(v->lastGCPass == currentGCPass) {
        return;
    }

    openList = malloc(sizeof(struct VMValueGCEntry));
    openList->value = v;
    openList->next = NULL;

    while(openList) {

        struct VMValueGCEntry *currentEntry = openList;
        openList = openList->next;

        if(currentEntry->value->lastGCPass != currentGCPass) {
            struct Value *value = currentEntry->value;
            value->lastGCPass = currentGCPass;

            // Add all references from this value to openList.

            switch(value->type) {

                // Mark strings in the string table.
                case VALUETYPE_STRING: {
                    struct VMString *str = vmStringTableGetEntryById(
                        &vm->stringTable, value->stringTableEntry);
                    if(str) {
                        str->lastGCPass = currentGCPass;
                    } else {
                        errorStateAddError(
                            &vm->errorState, -1,
                            "GC error: Bad string table index.");
                    }
                } break;

                default:
                    break;
            }
        }

        free(currentEntry);
    }
}

void vmGarbageCollect(struct VM *vm)
{
    uint32_t currentGCPass = ++vm->lastGCPass;

    // TODO: Remove this.
    printf("vmGarbageCollect\n");

    // Iterate through the current stack.
    {
        uint32_t i;
        struct Value *values = vm->stack.values;
        for(i = 0; i < vm->stack.size; i++) {
            vmGarbageCollect_markValue(
                vm, &values[i], currentGCPass);
        }
    }

    // TODO: Iterate through current variables.

    // TODO: Delete unmarked stuff from the heap.

    // Delete unmarked strings.
    vmStringTableCleanOldStrings(
        &vm->stringTable,
        currentGCPass);
}

void vmRescanProgramStrings(struct VM *vm)
{
    // This is needed for REPL support. Without it, string literals
    // would clutter stuff up permanently.

    // Unmark dontGC on everything.
    uint32_t i;
    for(i = 0; i < vm->stringTable.stringTableCapacity; i++) {
        struct VMString *str = vm->stringTable.stringTable[i];
        if(str) {
            str->dontGC = false;
        }
    }

    // Mark everything that's referenced from the program.
    for(i = 0; i <= vm->instructionAddressMask; i++) {
        if(vm->instructions[i].opcode == OP_PUSHLITERAL) {
            if(vm->instructions[i].pushLiteralData.value.type == VALUETYPE_STRING) {

                struct VMString *entry =
                    vmStringTableGetEntryById(
                        &vm->stringTable,
                        vm->instructions[i].pushLiteralData.value.stringTableEntry);

                if(entry) {
                    entry->dontGC = true;

                    printf("Marked string as in-use by program: %s\n", entry->str);
                }

            }
        }
    }
}


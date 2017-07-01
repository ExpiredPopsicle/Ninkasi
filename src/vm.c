#include "common.h"

// ----------------------------------------------------------------------
// Static opcode table setup.

typedef void (*VMOpcodeCall)(struct VM *vm, struct Instruction *instruction);
static VMOpcodeCall opcodeTable[OPCODE_PADDEDCOUNT];
static const char *opcodeNameTable[OPCODE_PADDEDCOUNT];

#define NAME_OPCODE(x) do { opcodeNameTable[x] = #x; } while(0)

static void vmInitOpcodeTable(void)
{
    assert(OPCODE_PADDEDCOUNT >= OPCODE_REALCOUNT);

    opcodeTable[OP_ADD]                = opcode_add;
    opcodeTable[OP_SUBTRACT]           = opcode_subtract;
    opcodeTable[OP_MULTIPLY]           = opcode_multiply;
    opcodeTable[OP_DIVIDE]             = opcode_divide;
    opcodeTable[OP_NEGATE]             = opcode_negate;
    opcodeTable[OP_PUSHLITERAL_INT]    = opcode_pushLiteral_int;
    opcodeTable[OP_PUSHLITERAL_FLOAT]  = opcode_pushLiteral_float;
    opcodeTable[OP_PUSHLITERAL_STRING] = opcode_pushLiteral_string;
    opcodeTable[OP_PUSHLITERAL_FUNCTIONID] = opcode_pushLiteral_functionId;
    opcodeTable[OP_NOP]                = opcode_nop;
    opcodeTable[OP_POP]                = opcode_pop;

    opcodeTable[OP_DUMP]               = opcode_dump;

    opcodeTable[OP_STACKPEEK]          = opcode_stackPeek;
    opcodeTable[OP_STACKPOKE]          = opcode_stackPoke;

    opcodeTable[OP_JUMP_RELATIVE]      = opcode_jumpRelative;

    opcodeTable[OP_CALL]               = opcode_call;
    opcodeTable[OP_RETURN]             = opcode_return;

    NAME_OPCODE(OP_ADD);
    NAME_OPCODE(OP_SUBTRACT);
    NAME_OPCODE(OP_MULTIPLY);
    NAME_OPCODE(OP_DIVIDE);
    NAME_OPCODE(OP_NEGATE);
    NAME_OPCODE(OP_PUSHLITERAL_INT);
    NAME_OPCODE(OP_PUSHLITERAL_FLOAT);
    NAME_OPCODE(OP_PUSHLITERAL_STRING);
    NAME_OPCODE(OP_PUSHLITERAL_FUNCTIONID);
    NAME_OPCODE(OP_NOP);
    NAME_OPCODE(OP_POP);

    NAME_OPCODE(OP_DUMP);

    NAME_OPCODE(OP_STACKPEEK);
    NAME_OPCODE(OP_STACKPOKE);

    NAME_OPCODE(OP_JUMP_RELATIVE);

    NAME_OPCODE(OP_CALL);
    NAME_OPCODE(OP_RETURN);

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

    vm->functionCount = 0;
    vm->functionTable = NULL;
}

void vmDestroy(struct VM *vm)
{
    vmStringTableDestroy(&vm->stringTable);

    vmStackDestroy(&vm->stack);
    errorStateDestroy(&vm->errorState);
    free(vm->instructions);
    free(vm->functionTable);
}

// ----------------------------------------------------------------------
// Iteration

void vmIterate(struct VM *vm)
{
    struct Instruction *inst = &vm->instructions[
        vm->instructionPointer & vm->instructionAddressMask];
    uint32_t opcodeId = inst->opcode & (OPCODE_PADDEDCOUNT - 1);

    printf("Executing: %s\n", vmGetOpcodeName(opcodeId));

    opcodeTable[opcodeId](vm, inst);
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

        if(vm->instructions[i].opcode == OP_PUSHLITERAL_STRING) {

            i++;
            {
                struct VMString *entry =
                    vmStringTableGetEntryById(
                        &vm->stringTable,
                        vm->instructions[i].opData_string);

                if(entry) {
                    entry->dontGC = true;

                    printf("Marked string as in-use by program: %s\n", entry->str);
                }
            }
        } else if(vm->instructions[i].opcode == OP_PUSHLITERAL_INT) {
            i++; // Skip data for this.
        } else if(vm->instructions[i].opcode == OP_PUSHLITERAL_FLOAT) {
            i++; // Skip data for this.
        }
    }
}

const char *vmGetOpcodeName(enum Opcode op)
{
    return opcodeNameTable[op & (OPCODE_PADDEDCOUNT - 1)];
}

struct VMFunction *vmCreateFunction(struct VM *vm, uint32_t *functionId)
{
    if(functionId) {
        *functionId = vm->functionCount++;
    }

    vm->functionTable = realloc(
        vm->functionTable,
        sizeof(struct VMFunction) * vm->functionCount);

    return &vm->functionTable[vm->functionCount - 1];
}

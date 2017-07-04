#include "common.h"

// ----------------------------------------------------------------------
// Static opcode table setup.

typedef void (*VMOpcodeCall)(struct VM *vm);
static VMOpcodeCall opcodeTable[OPCODE_PADDEDCOUNT];
static const char *opcodeNameTable[OPCODE_PADDEDCOUNT];

#define SETUP_OP(x, y)                          \
    do {                                        \
        opcodeNameTable[(x)] = #x;              \
        opcodeTable[(x)] = (y);                 \
    } while(0);

static void vmInitOpcodeTable(void)
{
    assert(OPCODE_PADDEDCOUNT >= OPCODE_REALCOUNT);

    SETUP_OP(OP_ADD,                    opcode_add);
    SETUP_OP(OP_SUBTRACT,               opcode_subtract);
    SETUP_OP(OP_MULTIPLY,               opcode_multiply);
    SETUP_OP(OP_DIVIDE,                 opcode_divide);
    SETUP_OP(OP_NEGATE,                 opcode_negate);
    SETUP_OP(OP_PUSHLITERAL_INT,        opcode_pushLiteral_int);
    SETUP_OP(OP_PUSHLITERAL_FLOAT,      opcode_pushLiteral_float);
    SETUP_OP(OP_PUSHLITERAL_STRING,     opcode_pushLiteral_string);
    SETUP_OP(OP_PUSHLITERAL_FUNCTIONID, opcode_pushLiteral_functionId);
    SETUP_OP(OP_NOP,                    opcode_nop);
    SETUP_OP(OP_POP,                    opcode_pop);
    SETUP_OP(OP_POPN,                   opcode_popN);

    SETUP_OP(OP_DUMP,                   opcode_dump);

    SETUP_OP(OP_STACKPEEK,              opcode_stackPeek);
    SETUP_OP(OP_STACKPOKE,              opcode_stackPoke);

    SETUP_OP(OP_JUMP_RELATIVE,          opcode_jumpRelative);

    SETUP_OP(OP_CALL,                   opcode_call);
    SETUP_OP(OP_RETURN,                 opcode_return);

    SETUP_OP(OP_END,                    opcode_end);

    SETUP_OP(OP_JUMP_IF_ZERO,           opcode_jz);

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
    vm->gcInterval = 5;
    vm->gcCountdown = vm->gcInterval;

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

    free(vm->globalVariables);
    free(vm->globalVariableNameStorage);
}

// ----------------------------------------------------------------------
// Iteration

void vmIterate(struct VM *vm)
{
    struct Instruction *inst = &vm->instructions[
        vm->instructionPointer & vm->instructionAddressMask];
    uint32_t opcodeId = inst->opcode & (OPCODE_PADDEDCOUNT - 1);

    dbgWriteLine("Executing: %s", vmGetOpcodeName(opcodeId));

    opcodeTable[opcodeId](vm);
    vm->instructionPointer++;

    // Handle periodic garbage collection.
    vm->gcCountdown--;
    if(!vm->gcCountdown) {
        vmGarbageCollect(vm);
        vm->gcCountdown = vm->gcInterval;
    }
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
    dbgWriteLine("vmGarbageCollect");

    // Iterate through the current stack.
    {
        uint32_t i;
        struct Value *values = vm->stack.values;
        for(i = 0; i < vm->stack.size; i++) {

            // FIXME: Remove this.
            assert(values == vm->stack.values);
            printf("Stacksize: %u\n", vm->stack.size);

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

                    dbgWriteLine("Marked string as in-use by program: %s", entry->str);
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

    memset(
        &vm->functionTable[vm->functionCount - 1], 0,
        sizeof(struct VMFunction));

    return &vm->functionTable[vm->functionCount - 1];
}

void vmCreateCFunction(
    struct VM *vm,
    VMFunctionCallback func,
    struct Value *output)
{
    // TODO: Lookup function first, to make sure we aren't making
    // duplicate functions.

    // TODO: Test this stupid thing.

    uint32_t functionId = 0;
    struct VMFunction *vmfunc =
        vmCreateFunction(vm, &functionId);

    vmfunc->argumentCount = ~(uint32_t)0;
    vmfunc->isCFunction = true;
    vmfunc->CFunctionCallback = func;

    output->type = VALUETYPE_FUNCTIONID;
    output->functionId = functionId;
}

void vmCallFunction(
    struct VM *vm,
    struct Value *functionValue,
    uint32_t argumentCount,
    struct Value *arguments,
    struct Value *returnValue)
{
    if(functionValue->type != VALUETYPE_FUNCTIONID) {
        errorStateAddError(
            &vm->errorState,
            -1,
            "Tried to call a non-function with vmCallFunction.");
        return;
    }

    {
        uint32_t oldInstructionPtr = vm->instructionPointer;
        uint32_t i;

        *vmStackPush_internal(vm) = *functionValue;
        for(i = 0; i < argumentCount; i++) {
            *vmStackPush_internal(vm) = arguments[i];
        }
        vmStackPushInt(vm, argumentCount);

        vm->instructionPointer = (~(uint32_t)0 - 1);

        opcode_call(vm);
        if(vm->errorState.firstError) {
            return;
        }

        vm->instructionPointer++;

        while(vm->instructionPointer != ~(uint32_t)0 &&
            !vm->errorState.firstError)
        {
            dbgWriteLine("In vmCallFunction %u", vm->instructionPointer);
            vmIterate(vm);
        }

        // Save return value.
        *returnValue = *vmStackPop(vm);

        // Restore old state.
        vm->instructionPointer = oldInstructionPtr;
    }
}

bool vmExecuteProgram(struct VM *vm)
{
    while(vm->instructions[
            vm->instructionPointer &
            vm->instructionAddressMask].opcode != OP_END)
    {

        vmIterate(vm);

        if(vm->errorState.firstError) {
            return false;
        }
    }

    return true;
}

uint32_t vmGetErrorCount(struct VM *vm)
{
    uint32_t count = 0;
    struct Error *error = vm->errorState.firstError;
    while(error) {
        count++;
        error = error->next;
    }
    return count;
}

struct VM *vmCreate(void)
{
    struct VM *vm = malloc(sizeof(struct VM));
    memset(vm, 0, sizeof(*vm));
    vmInit(vm);
    return vm;
}

void vmDelete(struct VM *vm)
{
    vmDestroy(vm);
    free(vm);
}

struct Value *vmFindGlobalVariable(
    struct VM *vm, const char *name)
{
    uint32_t i;
    for(i = 0; i < vm->globalVariableCount; i++) {
        if(!strcmp(vm->globalVariables[i].name, name)) {
            return &vm->stack.values[vm->stack.indexMask & vm->globalVariables[i].stackPosition];
        }
    }
    return NULL;
}


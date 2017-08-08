#include "common.h"

// ----------------------------------------------------------------------
// Static opcode table setup.

// TODO: Rename all of these.
typedef void (*VMOpcodeCall)(struct NKVM *vm);
static VMOpcodeCall opcodeTable[NK_OPCODE_PADDEDCOUNT];
static const char *opcodeNameTable[NK_OPCODE_PADDEDCOUNT];

// Change to the stack offset for each instruction. For example, POP
// will be -1. PUSHLITERAL_* will be +1. Be aware that some opcodes
// (like POPN and CALL) will adjust by some dynamic amount that can't
// be hardcoded here. They just need special care when generating the
// code, and have a value of zero here. Some instructions do weird
// shit to the stack, like RETURN. That's also got zero stack offset
// here.
int32_t nkiCompilerStackOffsetTable[NK_OPCODE_PADDEDCOUNT];

#define SETUP_OP(x, y, z)                       \
    do {                                        \
        opcodeNameTable[(x)] = #x;              \
        opcodeTable[(x)] = (y);                 \
        nkiCompilerStackOffsetTable[(x)] = (z); \
    } while(0);

static void vmInitOpcodeTable(void)
{
    assert(NK_OPCODE_PADDEDCOUNT >= NK_OPCODE_REALCOUNT);

    SETUP_OP(NK_OP_ADD,                    opcode_add,                    -1);
    SETUP_OP(NK_OP_SUBTRACT,               opcode_subtract,               -1);
    SETUP_OP(NK_OP_MULTIPLY,               opcode_multiply,               -1);
    SETUP_OP(NK_OP_DIVIDE,                 opcode_divide,                 -1);
    SETUP_OP(NK_OP_MODULO,                 opcode_modulo,                 -1);
    SETUP_OP(NK_OP_NEGATE,                 opcode_negate,                 0);
    SETUP_OP(NK_OP_PUSHLITERAL_INT,        opcode_pushLiteral_int,        1);
    SETUP_OP(NK_OP_PUSHLITERAL_FLOAT,      opcode_pushLiteral_float,      1);
    SETUP_OP(NK_OP_PUSHLITERAL_STRING,     opcode_pushLiteral_string,     1);
    SETUP_OP(NK_OP_PUSHLITERAL_FUNCTIONID, opcode_pushLiteral_functionId, 1);
    SETUP_OP(NK_OP_NOP,                    opcode_nop,                    0);
    SETUP_OP(NK_OP_POP,                    opcode_pop,                    -1);
    SETUP_OP(NK_OP_POPN,                   opcode_popN,                   0);

    SETUP_OP(NK_OP_DUMP,                   opcode_dump,                   -1);

    SETUP_OP(NK_OP_STACKPEEK,              opcode_stackPeek,              0);
    SETUP_OP(NK_OP_STACKPOKE,              opcode_stackPoke,              -1);

    SETUP_OP(NK_OP_JUMP_RELATIVE,          opcode_jumpRelative,           -1);

    SETUP_OP(NK_OP_CALL,                   opcode_call,                   0);
    SETUP_OP(NK_OP_RETURN,                 opcode_return,                 0);

    SETUP_OP(NK_OP_END,                    opcode_end,                    0);

    SETUP_OP(NK_OP_JUMP_IF_ZERO,           opcode_jz,                     -2);

    SETUP_OP(NK_OP_GREATERTHAN,            opcode_gt,                     -1);
    SETUP_OP(NK_OP_LESSTHAN,               opcode_lt,                     -1);
    SETUP_OP(NK_OP_GREATERTHANOREQUAL,     opcode_ge,                     -1);
    SETUP_OP(NK_OP_LESSTHANOREQUAL,        opcode_le,                     -1);
    SETUP_OP(NK_OP_EQUAL,                  opcode_eq,                     -1);
    SETUP_OP(NK_OP_NOTEQUAL,               opcode_ne,                     -1);
    SETUP_OP(NK_OP_EQUALWITHSAMETYPE,      opcode_eqsametype,             -1);
    SETUP_OP(NK_OP_NOT,                    opcode_not,                    0);

    SETUP_OP(NK_OP_AND,                    opcode_and,                    -1);
    SETUP_OP(NK_OP_OR,                     opcode_or,                     -1);

    SETUP_OP(NK_OP_OBJECTFIELDGET,         opcode_objectFieldGet,         -1);
    SETUP_OP(NK_OP_OBJECTFIELDSET,         opcode_objectFieldSet,         -2);

    SETUP_OP(NK_OP_CREATEOBJECT,           opcode_createObject,           1);

    SETUP_OP(NK_OP_PREPARESELFCALL,        opcode_prepareSelfCall,        0);
    SETUP_OP(NK_OP_OBJECTFIELDGET_NOPOP,   opcode_objectFieldGet_noPop,   0);

    SETUP_OP(NK_OP_PUSHNIL,                opcode_pushNil,                1);

    // Fill in the rest of the opcode table with no-ops. We just want
    // to pad up to a power of two so we can easily mask instructions
    // instead of branching to make sure they're valid.
    {
        uint32_t i;
        for(i = NK_OPCODE_REALCOUNT; i < NK_OPCODE_PADDEDCOUNT; i++) {
            opcodeTable[i] = opcode_nop;
            nkiCompilerStackOffsetTable[i] = 0;
        }
    }
}

// ----------------------------------------------------------------------
// Init/shutdown

void vmInit(struct NKVM *vm)
{
    // FIXME: Don't do this here.
    // memset(
    //     &vm->catastrophicFailureJmpBuf,
    //     0, sizeof(vm->catastrophicFailureJmpBuf));

    // Init memory management before almost anything else.
    vm->currentMemoryUsage = 0;
    vm->peakMemoryUsage = 0;
    vm->allocations = NULL;

    vm->limits.maxStrings = ~(uint32_t)0;
    vm->limits.maxStringLength = ~(uint32_t)0;
    vm->limits.maxStacksize = ~(uint32_t)0;
    vm->limits.maxObjects = ~(uint32_t)0;
    vm->limits.maxFieldsPerObject = ~(uint32_t)0;
    vm->limits.maxAllocatedMemory = ~(uint32_t)0;

    vmInitOpcodeTable();

    nkiErrorStateInit(vm);
    vmStackInit(vm);
    vm->instructionPointer = 0;

    vm->instructions =
        nkiMalloc(vm, sizeof(struct NKInstruction) * 4);
    vm->instructionAddressMask = 0x3;
    memset(vm->instructions, 0, sizeof(struct NKInstruction) * 4);

    nkiVmStringTableInit(vm);

    vm->lastGCPass = 0;
    vm->gcInterval = 1024;
    vm->gcCountdown = vm->gcInterval;
    vm->gcNewObjectInterval = 256;
    vm->gcNewObjectCountdown = vm->gcNewObjectInterval;

    vm->functionCount = 0;
    vm->functionTable = NULL;

    vm->globalVariableCount = 0;
    vm->globalVariables = NULL;
    vm->globalVariableNameStorage = NULL;

    vmObjectTableInit(vm);
}

void vmDestroy(struct NKVM *vm)
{
    if(vm->errorState.allocationFailure) {

        // Catastrophic failure cleanup mode. Do not trust most
        // internal pointers and use the allocation tracker directly.
        while(vm->allocations) {
            struct NKMemoryHeader *next = vm->allocations->nextAllocation;
            free(vm->allocations);
            vm->allocations = next;
        }

    } else {

        // Standard cleanup mode.

        NK_FAILURE_RECOVERY_DECL();

        NK_SET_FAILURE_RECOVERY_VOID();

        nkiVmStringTableDestroy(vm);

        vmStackDestroy(vm);
        nkiErrorStateDestroy(vm);
        nkiFree(vm, vm->instructions);
        nkiFree(vm, vm->functionTable);

        nkiFree(vm, vm->globalVariables);
        nkiFree(vm, vm->globalVariableNameStorage);

        vmObjectTableDestroy(vm);

        NK_CLEAR_FAILURE_RECOVERY();
    }
}

// ----------------------------------------------------------------------
// Iteration

void vmIterate(struct NKVM *vm)
{
    struct NKInstruction *inst = &vm->instructions[
        vm->instructionPointer & vm->instructionAddressMask];
    uint32_t opcodeId = inst->opcode & (NK_OPCODE_PADDEDCOUNT - 1);

    dbgWriteLine("Executing: %s", vmGetOpcodeName(opcodeId));

    opcodeTable[opcodeId](vm);
    vm->instructionPointer++;

    // Handle periodic garbage collection.

    vm->gcCountdown--;
    if(!vm->gcCountdown) {
        if(!vm->gcNewObjectCountdown) {
            vmGarbageCollect(vm);
            vm->gcNewObjectCountdown = vm->gcNewObjectInterval;
        }
        vm->gcCountdown = vm->gcInterval;
    }
}

// ----------------------------------------------------------------------
// Garbage collection

struct NKVMValueGCEntry
{
    struct NKValue *value;
    struct NKVMValueGCEntry *next;
};

struct NKVMGCState
{
    struct NKVM *vm;
    uint32_t currentGCPass;
    struct NKVMValueGCEntry *openList;
    struct NKVMValueGCEntry *closedList; // We'll keep this just for re-using allocations.
};

struct NKVMValueGCEntry *vmGcStateMakeEntry(struct NKVMGCState *state)
{
    struct NKVMValueGCEntry *ret = NULL;

    if(state->closedList) {
        ret = state->closedList;
        state->closedList = ret->next;
    } else {
        ret = nkiMalloc(state->vm, sizeof(struct NKVMValueGCEntry));
    }

    ret->next = state->openList;
    state->openList = ret;
    return ret;
}

void vmGarbageCollect_markObject(
    struct NKVMGCState *gcState,
    struct NKVMObject *ob)
{
    uint32_t bucket;

    if(ob->lastGCPass == gcState->currentGCPass) {
        return;
    }

    ob->lastGCPass = gcState->currentGCPass;

    for(bucket = 0; bucket < VMObjectHashBucketCount; bucket++) {

        struct NKVMObjectElement *el = ob->hashBuckets[bucket];

        while(el) {

            uint32_t k;
            for(k = 0; k < 2; k++) {
                struct NKValue *v = k ? &el->value : &el->key;

                if(v->type == NK_VALUETYPE_STRING ||
                    v->type == NK_VALUETYPE_OBJECTID)
                {
                    struct NKVMValueGCEntry *newEntry = vmGcStateMakeEntry(gcState);
                    newEntry->value = v;
                }
            }

            el = el->next;
        }
    }

}

// FIXME: Find a way to do this without tons of allocations.
void vmGarbageCollect_markValue(
    struct NKVMGCState *gcState,
    struct NKValue *v)
{
    if(v->lastGCPass == gcState->currentGCPass) {
        return;
    }

    {
        struct NKVMValueGCEntry *entry = vmGcStateMakeEntry(gcState);
        entry->value = v;
    }
}

void vmGarbageCollect_markReferenced(
    struct NKVMGCState *gcState)
{
    while(gcState->openList) {

        // Remove the value from the list.
        struct NKVMValueGCEntry *currentEntry = gcState->openList;
        gcState->openList = gcState->openList->next;

        if(currentEntry->value->lastGCPass != gcState->currentGCPass) {
            struct NKValue *value = currentEntry->value;
            value->lastGCPass = gcState->currentGCPass;

            // Add all references from this value to openList.

            switch(value->type) {

                case NK_VALUETYPE_STRING: {

                    struct NKVMString *str = nkiVmStringTableGetEntryById(
                        &gcState->vm->stringTable,
                        value->stringTableEntry);

                    if(str) {
                        str->lastGCPass = gcState->currentGCPass;
                    } else {
                        nkiAddError(
                            gcState->vm, -1,
                            "GC error: Bad string table index.");
                    }
                } break;

                case NK_VALUETYPE_OBJECTID: {

                    struct NKVMObject *ob = vmObjectTableGetEntryById(
                        &gcState->vm->objectTable,
                        value->objectId);

                    if(ob) {

                        vmGarbageCollect_markObject(gcState, ob);

                    } else {
                        nkiAddError(
                            gcState->vm, -1,
                            "GC error: Bad object table index.");
                    }
                } break;

                default:
                    break;
            }
        }

        currentEntry->next = gcState->closedList;
        gcState->closedList = currentEntry;
    }
}

void vmGarbageCollect(struct NKVM *vm)
{
    struct NKVMGCState gcState;
    memset(&gcState, 0, sizeof(gcState));
    gcState.currentGCPass = ++vm->lastGCPass;
    gcState.vm = vm;

    // TODO: Remove this.
    dbgWriteLine("vmGarbageCollect");

    // Iterate through objects with external handles.
    {
        struct NKVMObject *ob;
        for(ob = vm->objectTable.objectsWithExternalHandles; ob;
            ob = ob->nextObjectWithExternalHandles)
        {
            vmGarbageCollect_markObject(&gcState, ob);
        }
    }

    // Iterate through the current stack.
    {
        uint32_t i;
        struct NKValue *values = vm->stack.values;
        for(i = 0; i < vm->stack.size; i++) {
            vmGarbageCollect_markValue(
                &gcState, &values[i]);
        }
    }

    // TODO: Iterate through external data with external handles.

    // Now go and mark everything that the things in the open list
    // reference.
    vmGarbageCollect_markReferenced(&gcState);

    // Delete unmarked strings.
    nkiVmStringTableCleanOldStrings(
        vm, gcState.currentGCPass);

    // Delete unmarked (and not externally-referenced) objects.
    vmObjectTableCleanOldObjects(
        vm, gcState.currentGCPass);

    // TODO: Delete unmarked external data. Also run the GC callback
    // for them.

    // Clean up.
    assert(!gcState.openList);
    {
        uint32_t count = 0;
        while(gcState.closedList) {
            struct NKVMValueGCEntry *next = gcState.closedList->next;
            nkiFree(vm, gcState.closedList);
            gcState.closedList = next;
            count++;
        }
        dbgWriteLine("Closed list grew to: %u\n", count);
    }
}

void vmRescanProgramStrings(struct NKVM *vm)
{
    // This is needed for REPL support. Without it, string literals
    // would clutter stuff up permanently.

    // Unmark dontGC on everything.
    uint32_t i;
    for(i = 0; i < vm->stringTable.stringTableCapacity; i++) {
        struct NKVMString *str = vm->stringTable.stringTable[i];
        if(str) {
            str->dontGC = false;
        }
    }

    // Mark everything that's referenced from the program.
    for(i = 0; i <= vm->instructionAddressMask; i++) {

        if(vm->instructions[i].opcode == NK_OP_PUSHLITERAL_STRING) {

            i++;
            {
                struct NKVMString *entry =
                    nkiVmStringTableGetEntryById(
                        &vm->stringTable,
                        vm->instructions[i].opData_string);

                if(entry) {
                    entry->dontGC = true;

                    dbgWriteLine("Marked string as in-use by program: %s", entry->str);
                }
            }
        } else if(vm->instructions[i].opcode == NK_OP_PUSHLITERAL_INT) {
            i++; // Skip data for this.
        } else if(vm->instructions[i].opcode == NK_OP_PUSHLITERAL_FLOAT) {
            i++; // Skip data for this.
        }
    }
}

const char *vmGetOpcodeName(enum NKOpcode op)
{
    return opcodeNameTable[op & (NK_OPCODE_PADDEDCOUNT - 1)];
}

struct NKVMFunction *vmCreateFunction(struct NKVM *vm, uint32_t *functionId)
{
    if(functionId) {
        *functionId = vm->functionCount++;
    }


    vm->functionTable = nkiRealloc(
        vm,
        vm->functionTable,
        sizeof(struct NKVMFunction) * vm->functionCount);

    memset(
        &vm->functionTable[vm->functionCount - 1], 0,
        sizeof(struct NKVMFunction));

    return &vm->functionTable[vm->functionCount - 1];
}

void vmCreateCFunction(
    struct NKVM *vm,
    VMFunctionCallback func,
    struct NKValue *output)
{
    // TODO: Lookup function first, to make sure we aren't making
    // duplicate functions.

    // TODO: Test this stupid thing.

    uint32_t functionId = 0;
    struct NKVMFunction *vmfunc =
        vmCreateFunction(vm, &functionId);

    vmfunc->argumentCount = ~(uint32_t)0;
    vmfunc->isCFunction = true;
    vmfunc->CFunctionCallback = func;

    output->type = NK_VALUETYPE_FUNCTIONID;
    output->functionId = functionId;
}

void vmCallFunction(
    struct NKVM *vm,
    struct NKValue *functionValue,
    uint32_t argumentCount,
    struct NKValue *arguments,
    struct NKValue *returnValue)
{
    if(functionValue->type != NK_VALUETYPE_FUNCTIONID) {
        nkiAddError(
            vm, -1,
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

bool vmExecuteProgram(struct NKVM *vm)
{
    while(vm->instructions[
            vm->instructionPointer &
            vm->instructionAddressMask].opcode != NK_OP_END)
    {
        vmIterate(vm);

        if(vm->errorState.firstError) {
            return false;
        }
    }

    return true;
}

uint32_t vmGetErrorCount(struct NKVM *vm)
{
    uint32_t count = 0;
    struct NKError *error = vm->errorState.firstError;

    while(error) {
        count++;
        error = error->next;
    }

    if(vm->errorState.allocationFailure) {
        count++;
    }

    return count;
}

struct NKValue *vmFindGlobalVariable(
    struct NKVM *vm, const char *name)
{
    uint32_t i;
    for(i = 0; i < vm->globalVariableCount; i++) {
        if(!strcmp(vm->globalVariables[i].name, name)) {
            return &vm->stack.values[vm->stack.indexMask & vm->globalVariables[i].stackPosition];
        }
    }
    return NULL;
}


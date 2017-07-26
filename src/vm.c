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

    SETUP_OP(NK_OP_ADD,                    opcode_add);
    SETUP_OP(NK_OP_SUBTRACT,               opcode_subtract);
    SETUP_OP(NK_OP_MULTIPLY,               opcode_multiply);
    SETUP_OP(NK_OP_DIVIDE,                 opcode_divide);
    SETUP_OP(NK_OP_MODULO,                 opcode_modulo);
    SETUP_OP(NK_OP_NEGATE,                 opcode_negate);
    SETUP_OP(NK_OP_PUSHLITERAL_INT,        opcode_pushLiteral_int);
    SETUP_OP(NK_OP_PUSHLITERAL_FLOAT,      opcode_pushLiteral_float);
    SETUP_OP(NK_OP_PUSHLITERAL_STRING,     opcode_pushLiteral_string);
    SETUP_OP(NK_OP_PUSHLITERAL_FUNCTIONID, opcode_pushLiteral_functionId);
    SETUP_OP(NK_OP_NOP,                    opcode_nop);
    SETUP_OP(NK_OP_POP,                    opcode_pop);
    SETUP_OP(NK_OP_POPN,                   opcode_popN);

    SETUP_OP(NK_OP_DUMP,                   opcode_dump);

    SETUP_OP(NK_OP_STACKPEEK,              opcode_stackPeek);
    SETUP_OP(NK_OP_STACKPOKE,              opcode_stackPoke);

    SETUP_OP(NK_OP_JUMP_RELATIVE,          opcode_jumpRelative);

    SETUP_OP(NK_OP_CALL,                   opcode_call);
    SETUP_OP(NK_OP_RETURN,                 opcode_return);

    SETUP_OP(NK_OP_END,                    opcode_end);

    SETUP_OP(NK_OP_JUMP_IF_ZERO,           opcode_jz);

    SETUP_OP(NK_OP_GREATERTHAN,            opcode_gt);
    SETUP_OP(NK_OP_LESSTHAN,               opcode_lt);
    SETUP_OP(NK_OP_GREATERTHANOREQUAL,     opcode_ge);
    SETUP_OP(NK_OP_LESSTHANOREQUAL,        opcode_le);
    SETUP_OP(NK_OP_EQUAL,                  opcode_eq);
    SETUP_OP(NK_OP_NOTEQUAL,               opcode_ne);
    SETUP_OP(NK_OP_EQUALWITHSAMETYPE,      opcode_eqsametype);
    SETUP_OP(NK_OP_NOT,                    opcode_not);

    SETUP_OP(NK_OP_AND,                    opcode_and);
    SETUP_OP(NK_OP_OR,                     opcode_or);

    SETUP_OP(NK_OP_OBJECTFIELDGET,         opcode_objectFieldGet);
    SETUP_OP(NK_OP_OBJECTFIELDSET,         opcode_objectFieldSet);

    SETUP_OP(NK_OP_CREATEOBJECT,           opcode_createObject);

    SETUP_OP(NK_OP_PREPARESELFCALL,        opcode_prepareSelfCall);
    SETUP_OP(NK_OP_OBJECTFIELDGET_NOPOP,   opcode_objectFieldGet_noPop);

    SETUP_OP(NK_OP_PUSHNIL,                opcode_pushNil);

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
        nkMalloc(vm, sizeof(struct Instruction) * 4);
    vm->instructionAddressMask = 0x3;
    memset(vm->instructions, 0, sizeof(struct Instruction) * 4);

    vmStringTableInit(vm);

    vm->lastGCPass = 0;
    vm->gcInterval = 1000;
    vm->gcCountdown = vm->gcInterval;

    vm->functionCount = 0;
    vm->functionTable = NULL;

    vm->globalVariableCount = 0;
    vm->globalVariables = NULL;
    vm->globalVariableNameStorage = NULL;

    vmObjectTableInit(vm);
}

void vmDestroy(struct VM *vm)
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

        vmStringTableDestroy(vm);

        vmStackDestroy(vm);
        nkiErrorStateDestroy(vm);
        nkFree(vm, vm->instructions);
        nkFree(vm, vm->functionTable);

        nkFree(vm, vm->globalVariables);
        nkFree(vm, vm->globalVariableNameStorage);

        vmObjectTableDestroy(vm);

        NK_CLEAR_FAILURE_RECOVERY();
    }
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

struct VMGCState
{
    struct VM *vm;
    uint32_t currentGCPass;
    struct VMValueGCEntry *openList;
    struct VMValueGCEntry *closedList; // We'll keep this just for re-using allocations.
};

struct VMValueGCEntry *vmGcStateMakeEntry(struct VMGCState *state)
{
    struct VMValueGCEntry *ret = NULL;

    if(state->closedList) {
        ret = state->closedList;
        state->closedList = ret->next;
    } else {
        ret = nkMalloc(state->vm, sizeof(struct VMValueGCEntry));
    }

    ret->next = state->openList;
    state->openList = ret;
    return ret;
}

void vmGarbageCollect_markObject(
    struct VMGCState *gcState,
    struct VMObject *ob)
{
    uint32_t bucket;

    if(ob->lastGCPass == gcState->currentGCPass) {
        return;
    }

    ob->lastGCPass = gcState->currentGCPass;

    for(bucket = 0; bucket < VMObjectHashBucketCount; bucket++) {

        struct VMObjectElement *el = ob->hashBuckets[bucket];

        while(el) {

            uint32_t k;
            for(k = 0; k < 2; k++) {
                struct Value *v = k ? &el->value : &el->key;

                if(v->type == NK_VALUETYPE_STRING ||
                    v->type == NK_VALUETYPE_OBJECTID)
                {
                    struct VMValueGCEntry *newEntry = vmGcStateMakeEntry(gcState);
                    newEntry->value = v;
                }
            }

            el = el->next;
        }
    }

}

// FIXME: Find a way to do this without tons of allocations.
void vmGarbageCollect_markValue(
    struct VMGCState *gcState,
    struct Value *v)
{
    if(v->lastGCPass == gcState->currentGCPass) {
        return;
    }

    {
        struct VMValueGCEntry *entry = vmGcStateMakeEntry(gcState);
        entry->value = v;
    }
}

void vmGarbageCollect_markReferenced(
    struct VMGCState *gcState)
{
    while(gcState->openList) {

        // Remove the value from the list.
        struct VMValueGCEntry *currentEntry = gcState->openList;
        gcState->openList = gcState->openList->next;

        if(currentEntry->value->lastGCPass != gcState->currentGCPass) {
            struct Value *value = currentEntry->value;
            value->lastGCPass = gcState->currentGCPass;

            // Add all references from this value to openList.

            switch(value->type) {

                case NK_VALUETYPE_STRING: {

                    struct VMString *str = vmStringTableGetEntryById(
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

                    struct VMObject *ob = vmObjectTableGetEntryById(
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

void vmGarbageCollect(struct VM *vm)
{
    struct VMGCState gcState;
    memset(&gcState, 0, sizeof(gcState));
    gcState.currentGCPass = ++vm->lastGCPass;
    gcState.vm = vm;

    // TODO: Remove this.
    dbgWriteLine("vmGarbageCollect");

    // Iterate through objects with external handles.
    {
        struct VMObject *ob;
        for(ob = vm->objectTable.objectsWithExternalHandles; ob;
            ob = ob->nextObjectWithExternalHandles)
        {
            vmGarbageCollect_markObject(&gcState, ob);
        }
    }

    // Iterate through the current stack.
    {
        uint32_t i;
        struct Value *values = vm->stack.values;
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
    vmStringTableCleanOldStrings(
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
            struct VMValueGCEntry *next = gcState.closedList->next;
            nkFree(vm, gcState.closedList);
            gcState.closedList = next;
            count++;
        }
        dbgWriteLine("Closed list grew to: %u\n", count);
    }
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

        if(vm->instructions[i].opcode == NK_OP_PUSHLITERAL_STRING) {

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
        } else if(vm->instructions[i].opcode == NK_OP_PUSHLITERAL_INT) {
            i++; // Skip data for this.
        } else if(vm->instructions[i].opcode == NK_OP_PUSHLITERAL_FLOAT) {
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


    vm->functionTable = nkRealloc(
        vm,
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

    output->type = NK_VALUETYPE_FUNCTIONID;
    output->functionId = functionId;
}

void vmCallFunction(
    struct VM *vm,
    struct Value *functionValue,
    uint32_t argumentCount,
    struct Value *arguments,
    struct Value *returnValue)
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

bool vmExecuteProgram(struct VM *vm)
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

uint32_t vmGetErrorCount(struct VM *vm)
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


#include "nkcommon.h"

// ----------------------------------------------------------------------
// Static opcode table setup.

typedef void (*NKVMOpcodeCall)(struct NKVM *vm);
static NKVMOpcodeCall nkiOpcodeTable[NK_OPCODE_PADDEDCOUNT];
static const char *nkiOpcodeNameTable[NK_OPCODE_PADDEDCOUNT];

// Change to the stack offset for each instruction. For example, POP
// will be -1. PUSHLITERAL_* will be +1. Be aware that some opcodes
// (like POPN and CALL) will adjust by some dynamic amount that can't
// be hardcoded here. They just need special care when generating the
// code, and have a value of zero here. Some instructions do weird
// shit to the stack, like RETURN. That's also got zero stack offset
// here.
nkint32_t nkiCompilerStackOffsetTable[NK_OPCODE_PADDEDCOUNT];

#define NK_SETUP_OP(x, y, z)                       \
    do {                                        \
        nkiOpcodeNameTable[(x)] = #x;              \
        nkiOpcodeTable[(x)] = (y);                 \
        nkiCompilerStackOffsetTable[(x)] = (z); \
    } while(0);

static void nkiVmInitOpcodeTable(void)
{
    assert(NK_OPCODE_PADDEDCOUNT >= NK_OPCODE_REALCOUNT);

    NK_SETUP_OP(NK_OP_ADD,                    nkiOpcode_add,                    -1);
    NK_SETUP_OP(NK_OP_SUBTRACT,               nkiOpcode_subtract,               -1);
    NK_SETUP_OP(NK_OP_MULTIPLY,               nkiOpcode_multiply,               -1);
    NK_SETUP_OP(NK_OP_DIVIDE,                 nkiOpcode_divide,                 -1);
    NK_SETUP_OP(NK_OP_MODULO,                 nkiOpcode_modulo,                 -1);
    NK_SETUP_OP(NK_OP_NEGATE,                 nkiOpcode_negate,                 0);
    NK_SETUP_OP(NK_OP_PUSHLITERAL_INT,        nkiOpcode_pushLiteral_int,        1);
    NK_SETUP_OP(NK_OP_PUSHLITERAL_FLOAT,      nkiOpcode_pushLiteral_float,      1);
    NK_SETUP_OP(NK_OP_PUSHLITERAL_STRING,     nkiOpcode_pushLiteral_string,     1);
    NK_SETUP_OP(NK_OP_PUSHLITERAL_FUNCTIONID, nkiOpcode_pushLiteral_functionId, 1);
    NK_SETUP_OP(NK_OP_NOP,                    nkiOpcode_nop,                    0);
    NK_SETUP_OP(NK_OP_POP,                    nkiOpcode_pop,                    -1);
    NK_SETUP_OP(NK_OP_POPN,                   nkiOpcode_popN,                   0);

    NK_SETUP_OP(NK_OP_DUMP,                   nkiOpcode_dump,                   -1);

    NK_SETUP_OP(NK_OP_STACKPEEK,              nkiOpcode_stackPeek,              0);
    NK_SETUP_OP(NK_OP_STACKPOKE,              nkiOpcode_stackPoke,              -1);

    NK_SETUP_OP(NK_OP_JUMP_RELATIVE,          nkiOpcode_jumpRelative,           -1);

    NK_SETUP_OP(NK_OP_CALL,                   nkiOpcode_call,                   0);
    NK_SETUP_OP(NK_OP_RETURN,                 nkiOpcode_return,                 0);

    NK_SETUP_OP(NK_OP_END,                    nkiOpcode_end,                    0);

    NK_SETUP_OP(NK_OP_JUMP_IF_ZERO,           nkiOpcode_jz,                     -2);

    NK_SETUP_OP(NK_OP_GREATERTHAN,            nkiOpcode_gt,                     -1);
    NK_SETUP_OP(NK_OP_LESSTHAN,               nkiOpcode_lt,                     -1);
    NK_SETUP_OP(NK_OP_GREATERTHANOREQUAL,     nkiOpcode_ge,                     -1);
    NK_SETUP_OP(NK_OP_LESSTHANOREQUAL,        nkiOpcode_le,                     -1);
    NK_SETUP_OP(NK_OP_EQUAL,                  nkiOpcode_eq,                     -1);
    NK_SETUP_OP(NK_OP_NOTEQUAL,               nkiOpcode_ne,                     -1);
    NK_SETUP_OP(NK_OP_EQUALWITHSAMETYPE,      nkiOpcode_eqsametype,             -1);
    NK_SETUP_OP(NK_OP_NOT,                    nkiOpcode_not,                    0);

    NK_SETUP_OP(NK_OP_AND,                    nkiOpcode_and,                    -1);
    NK_SETUP_OP(NK_OP_OR,                     nkiOpcode_or,                     -1);

    NK_SETUP_OP(NK_OP_OBJECTFIELDGET,         nkiOpcode_objectFieldGet,         -1);
    NK_SETUP_OP(NK_OP_OBJECTFIELDSET,         nkiOpcode_objectFieldSet,         -2);

    NK_SETUP_OP(NK_OP_CREATEOBJECT,           nkiOpcode_createObject,           1);

    NK_SETUP_OP(NK_OP_PREPARESELFCALL,        nkiOpcode_prepareSelfCall,        0);
    NK_SETUP_OP(NK_OP_OBJECTFIELDGET_NOPOP,   nkiOpcode_objectFieldGet_noPop,   0);

    NK_SETUP_OP(NK_OP_PUSHNIL,                nkiOpcode_pushNil,                1);

    // Fill in the rest of the opcode table with no-ops. We just want
    // to pad up to a power of two so we can easily mask instructions
    // instead of branching to make sure they're valid.
    {
        nkuint32_t i;
        for(i = NK_OPCODE_REALCOUNT; i < NK_OPCODE_PADDEDCOUNT; i++) {
            nkiOpcodeTable[i] = nkiOpcode_nop;
            nkiCompilerStackOffsetTable[i] = 0;
        }
    }

    // Quick sanity check.
    assert(sizeof(nkuint32_t) == 4);
    assert(sizeof(nkint32_t) == 4);
    assert(sizeof(nkbool) == 1);
}

// ----------------------------------------------------------------------
// Init/shutdown

void nkiVmInit(struct NKVM *vm)
{
    // NOTE: By the time this function is called, the
    // catastrophicFailureJmpBuf field should already be set. Do not
    // overwrite it.

    // Init memory management before almost anything else.
    vm->currentMemoryUsage = 0;
    vm->peakMemoryUsage = 0;
    vm->allocations = NULL;

    vm->limits.maxStrings = ~(nkuint32_t)0;
    vm->limits.maxStringLength = ~(nkuint32_t)0;
    vm->limits.maxStacksize = ~(nkuint32_t)0;
    vm->limits.maxObjects = ~(nkuint32_t)0;
    vm->limits.maxFieldsPerObject = ~(nkuint32_t)0;
    vm->limits.maxAllocatedMemory = ~(nkuint32_t)0;

    nkiVmInitOpcodeTable();

    nkiErrorStateInit(vm);
    nkiVmStackInit(vm);
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

    nkiVmObjectTableInit(vm);
}

void nkiVmDestroy(struct NKVM *vm)
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

        nkiVmStackDestroy(vm);
        nkiErrorStateDestroy(vm);
        nkiFree(vm, vm->instructions);
        nkiFree(vm, vm->functionTable);

        nkiFree(vm, vm->globalVariables);
        nkiFree(vm, vm->globalVariableNameStorage);

        nkiVmObjectTableDestroy(vm);

        NK_CLEAR_FAILURE_RECOVERY();
    }
}

// ----------------------------------------------------------------------
// Iteration

void nkiVmIterate(struct NKVM *vm)
{
    struct NKInstruction *inst = &vm->instructions[
        vm->instructionPointer & vm->instructionAddressMask];
    nkuint32_t opcodeId = inst->opcode & (NK_OPCODE_PADDEDCOUNT - 1);

    nkiDbgWriteLine("Executing: %s", nkiVmGetOpcodeName(opcodeId));

    nkiOpcodeTable[opcodeId](vm);
    vm->instructionPointer++;

    // Handle periodic garbage collection.

    vm->gcCountdown--;
    if(!vm->gcCountdown) {
        if(!vm->gcNewObjectCountdown) {
            nkiVmGarbageCollect(vm);
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
    nkuint32_t currentGCPass;
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

void nkiVmGarbageCollect_markObject(
    struct NKVMGCState *gcState,
    struct NKVMObject *ob)
{
    nkuint32_t bucket;

    if(ob->lastGCPass == gcState->currentGCPass) {
        return;
    }

    ob->lastGCPass = gcState->currentGCPass;

    for(bucket = 0; bucket < nkiVMObjectHashBucketCount; bucket++) {

        struct NKVMObjectElement *el = ob->hashBuckets[bucket];

        while(el) {

            nkuint32_t k;
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

void nkiVmGarbageCollect_markValue(
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

void nkiVmGarbageCollect_markReferenced(
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

                    struct NKVMObject *ob = nkiVmObjectTableGetEntryById(
                        &gcState->vm->objectTable,
                        value->objectId);

                    if(ob) {

                        nkiVmGarbageCollect_markObject(gcState, ob);

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

void nkiVmGarbageCollect(struct NKVM *vm)
{
    struct NKVMGCState gcState;
    memset(&gcState, 0, sizeof(gcState));
    gcState.currentGCPass = ++vm->lastGCPass;
    gcState.vm = vm;

    // TODO: Remove this.
    nkiDbgWriteLine("nkiVmGarbageCollect");

    // Iterate through objects with external handles.
    {
        struct NKVMObject *ob;
        for(ob = vm->objectTable.objectsWithExternalHandles; ob;
            ob = ob->nextObjectWithExternalHandles)
        {
            nkiVmGarbageCollect_markObject(&gcState, ob);
        }
    }

    // Iterate through the current stack.
    {
        nkuint32_t i;
        struct NKValue *values = vm->stack.values;
        for(i = 0; i < vm->stack.size; i++) {
            nkiVmGarbageCollect_markValue(
                &gcState, &values[i]);
        }
    }

    // TODO: Iterate through external data with external handles.

    // Now go and mark everything that the things in the open list
    // reference.
    nkiVmGarbageCollect_markReferenced(&gcState);

    // Delete unmarked strings.
    nkiVmStringTableCleanOldStrings(
        vm, gcState.currentGCPass);

    // Delete unmarked (and not externally-referenced) objects.
    nkiVmObjectTableCleanOldObjects(
        vm, gcState.currentGCPass);

    // TODO: Delete unmarked external data. Also run the GC callback
    // for them.

    // Clean up.
    assert(!gcState.openList);
    {
        nkuint32_t count = 0;
        while(gcState.closedList) {
            struct NKVMValueGCEntry *next = gcState.closedList->next;
            nkiFree(vm, gcState.closedList);
            gcState.closedList = next;
            count++;
        }
        nkiDbgWriteLine("Closed list grew to: %u\n", count);
    }
}

void nkiVmRescanProgramStrings(struct NKVM *vm)
{
    // This is needed for REPL support. Without it, string literals
    // would clutter stuff up permanently.

    // Unmark dontGC on everything.
    nkuint32_t i;
    for(i = 0; i < vm->stringTable.stringTableCapacity; i++) {
        struct NKVMString *str = vm->stringTable.stringTable[i];
        if(str) {
            str->dontGC = nkfalse;
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
                    entry->dontGC = nktrue;

                    nkiDbgWriteLine("Marked string as in-use by program: %s", entry->str);
                }
            }
        } else if(vm->instructions[i].opcode == NK_OP_PUSHLITERAL_INT) {
            i++; // Skip data for this.
        } else if(vm->instructions[i].opcode == NK_OP_PUSHLITERAL_FLOAT) {
            i++; // Skip data for this.
        }
    }
}

const char *nkiVmGetOpcodeName(enum NKOpcode op)
{
    return nkiOpcodeNameTable[op & (NK_OPCODE_PADDEDCOUNT - 1)];
}

struct NKVMFunction *nkiVmCreateFunction(struct NKVM *vm, nkuint32_t *functionId)
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

void nkiVmCreateCFunction(
    struct NKVM *vm,
    VMFunctionCallback func,
    struct NKValue *output)
{
    // TODO: Lookup function first, to make sure we aren't making
    // duplicate functions.

    nkuint32_t functionId = 0;
    struct NKVMFunction *vmfunc =
        nkiVmCreateFunction(vm, &functionId);

    vmfunc->argumentCount = ~(nkuint32_t)0;
    vmfunc->isCFunction = nktrue;
    vmfunc->CFunctionCallback = func;

    output->type = NK_VALUETYPE_FUNCTIONID;
    output->functionId = functionId;
}

void nkiVmCallFunction(
    struct NKVM *vm,
    struct NKValue *functionValue,
    nkuint32_t argumentCount,
    struct NKValue *arguments,
    struct NKValue *returnValue)
{
    if(functionValue->type != NK_VALUETYPE_FUNCTIONID) {
        nkiAddError(
            vm, -1,
            "Tried to call a non-function with nkiVmCallFunction.");
        return;
    }

    {
        nkuint32_t oldInstructionPtr = vm->instructionPointer;
        nkuint32_t i;

        *nkiVmStackPush_internal(vm) = *functionValue;
        for(i = 0; i < argumentCount; i++) {
            *nkiVmStackPush_internal(vm) = arguments[i];
        }
        nkiVmStackPushInt(vm, argumentCount);

        vm->instructionPointer = (~(nkuint32_t)0 - 1);

        nkiOpcode_call(vm);
        if(vm->errorState.firstError) {
            return;
        }

        vm->instructionPointer++;

        while(vm->instructionPointer != ~(nkuint32_t)0 &&
            !vm->errorState.firstError)
        {
            nkiDbgWriteLine("In nkiVmCallFunction %u", vm->instructionPointer);
            nkiVmIterate(vm);
        }

        // Save return value.
        *returnValue = *nkiVmStackPop(vm);

        // Restore old state.
        vm->instructionPointer = oldInstructionPtr;
    }
}

nkbool nkiVmExecuteProgram(struct NKVM *vm)
{
    while(vm->instructions[
            vm->instructionPointer &
            vm->instructionAddressMask].opcode != NK_OP_END)
    {
        nkiVmIterate(vm);

        if(vm->errorState.firstError) {
            return nkfalse;
        }
    }

    return nktrue;
}

nkuint32_t nkiVmGetErrorCount(struct NKVM *vm)
{
    nkuint32_t count = 0;
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

struct NKValue *nkiVmFindGlobalVariable(
    struct NKVM *vm, const char *name)
{
    nkuint32_t i;
    for(i = 0; i < vm->globalVariableCount; i++) {
        if(!strcmp(vm->globalVariables[i].name, name)) {
            return &vm->stack.values[vm->stack.indexMask & vm->globalVariables[i].stackPosition];
        }
    }
    return NULL;
}


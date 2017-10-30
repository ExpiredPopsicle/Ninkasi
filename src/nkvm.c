// ----------------------------------------------------------------------
//
//        ▐ ▄ ▪   ▐ ▄ ▄ •▄  ▄▄▄· .▄▄ · ▪
//       •█▌▐███ •█▌▐██▌▄▌▪▐█ ▀█ ▐█ ▀. ██
//       ▐█▐▐▌▐█·▐█▐▐▌▐▀▀▄·▄█▀▀█ ▄▀▀▀█▄▐█·
//       ██▐█▌▐█▌██▐█▌▐█.█▌▐█ ▪▐▌▐█▄▪▐█▐█▌
//       ▀▀ █▪▀▀▀▀▀ █▪·▀  ▀ ▀  ▀  ▀▀▀▀ ▀▀▀
//
// ----------------------------------------------------------------------
//
//   Ninkasi 0.01
//
//   By Cliff "ExpiredPopsicle" Jolly
//     https://expiredpopsicle.com
//     https://intoxicoding.com
//     expiredpopsicle@gmail.com
//
// ----------------------------------------------------------------------
//
//   Copyright (c) 2017 Clifford Jolly
//
//   Permission is hereby granted, free of charge, to any person
//   obtaining a copy of this software and associated documentation files
//   (the "Software"), to deal in the Software without restriction,
//   including without limitation the rights to use, copy, modify, merge,
//   publish, distribute, sublicense, and/or sell copies of the Software,
//   and to permit persons to whom the Software is furnished to do so,
//   subject to the following conditions:
//
//   The above copyright notice and this permission notice shall be
//   included in all copies or substantial portions of the Software.
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
//   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
//   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//   SOFTWARE.
//
// -------------------------- END HEADER -------------------------------------

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
    static nkbool opcodeTableInitComplete = nkfalse;
    if(opcodeTableInitComplete) {
        return;
    }

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

    NK_SETUP_OP(NK_OP_STACKPEEK,              nkiOpcode_stackPeek,              0);
    NK_SETUP_OP(NK_OP_STACKPOKE,              nkiOpcode_stackPoke,              -1);

    NK_SETUP_OP(NK_OP_STATICPEEK,             nkiOpcode_staticPeek,             0);
    NK_SETUP_OP(NK_OP_STATICPOKE,             nkiOpcode_staticPoke,             -1);

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

    opcodeTableInitComplete = nktrue;
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

    vm->limits.maxStrings = NK_UINT_MAX;
    vm->limits.maxStringLength = NK_UINT_MAX;
    vm->limits.maxStacksize = NK_UINT_MAX;
    vm->limits.maxObjects = NK_UINT_MAX;
    vm->limits.maxFieldsPerObject = NK_UINT_MAX;
    vm->limits.maxAllocatedMemory = NK_UINT_MAX;

    vm->userData = NULL;
    vm->externalFunctionCount = 0;
    vm->externalFunctionTable = NULL;

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

    // Start with a single static value.
    vm->staticSpace = nkiMalloc(vm, sizeof(struct NKValue));
    memset(vm->staticSpace, 0, sizeof(struct NKValue));
    vm->staticAddressMask = 1;

    vm->externalTypeNames = NULL;
    vm->externalTypeCount = 0;
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

        // Nuke the entire static address space and stack, then force
        // a final garbage collection pass before we start making the
        // VM unusable, so that GC callbacks get hit for external data
        // and that stuff can clean up in a conventional manner.
        nkiVmStackClear(vm);
        memset(
            vm->staticSpace, 0,
            (vm->staticAddressMask + 1) * sizeof(struct NKValue));
        nkiVmGarbageCollect(vm);

        nkiVmStringTableDestroy(vm);

        nkiVmStackDestroy(vm);
        nkiErrorStateDestroy(vm);
        nkiFree(vm, vm->instructions);
        nkiFree(vm, vm->functionTable);

        nkiFree(vm, vm->globalVariables);
        nkiFree(vm, vm->globalVariableNameStorage);

        nkiVmObjectTableDestroy(vm);

        nkiFree(vm, vm->staticSpace);

        // Free external function table.
        {
            nkuint32_t n;
            for(n = 0; n < vm->externalFunctionCount; n++) {
                nkiFree(vm, vm->externalFunctionTable[n].name);
            }
        }
        nkiFree(vm, vm->externalFunctionTable);

        // Free external type data.
        {
            nkuint32_t n;
            for(n = 0; n < vm->externalTypeCount; n++) {
                nkiFree(vm, vm->externalTypeNames[n]);
            }
            nkiFree(vm, vm->externalTypeNames);
        }

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

struct NKVMValueGCEntry *nkiVmGcStateMakeEntry(struct NKVMGCState *state)
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
                    struct NKVMValueGCEntry *newEntry = nkiVmGcStateMakeEntry(gcState);
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
        struct NKVMValueGCEntry *entry = nkiVmGcStateMakeEntry(gcState);
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

    // Iterate through the current static space.
    {
        nkuint32_t i = 0;
        struct NKValue *values = vm->staticSpace;
        while(1) {

            nkiVmGarbageCollect_markValue(
                &gcState, &values[i]);

            if(i == vm->staticAddressMask) {
                break;
            }
            i++;
        }
    }

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

// ----------------------------------------------------------------------

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

struct NKVMFunction *nkiVmCreateFunction(
    struct NKVM *vm, NKVMInternalFunctionID *functionId)
{
    if(functionId) {
        functionId->id = vm->functionCount++;
    }

    vm->functionTable = nkiRealloc(
        vm,
        vm->functionTable,
        sizeof(struct NKVMFunction) * vm->functionCount);

    memset(
        &vm->functionTable[vm->functionCount - 1], 0,
        sizeof(struct NKVMFunction));
    vm->functionTable[vm->functionCount - 1].externalFunctionId.id = NK_INVALID_VALUE;

    return &vm->functionTable[vm->functionCount - 1];
}

void nkiVmCallFunction(
    struct NKVM *vm,
    struct NKValue *functionValue,
    nkuint32_t argumentCount,
    struct NKValue *arguments,
    struct NKValue *returnValue)
{
    if(functionValue->type != NK_VALUETYPE_FUNCTIONID) {
        printf("Type: %d\n", functionValue->type);
        nkiAddError(
            vm, -1,
            "Tried to call a non-function with nkiVmCallFunction.");
        return;
    }

    {
        // Save whatever the real instruction pointer was so that we
        // can restore it after we're done with this artificially
        // injected function call.
        nkuint32_t oldInstructionPtr = vm->instructionPointer;
        nkuint32_t i;

        // Push the function ID itself onto the stack.
        *nkiVmStackPush_internal(vm) = *functionValue;

        // Push arguments.
        for(i = 0; i < argumentCount; i++) {
            *nkiVmStackPush_internal(vm) = arguments[i];
        }

        // Push argument count.
        nkiVmStackPushInt(vm, argumentCount);

        // Set the instruction pointer to a special return value we
        // use for external calls into the VM. This will push a
        // NK_UINT_MAX-1 pointer onto the stack, which the VM will
        // return to after the function is called. Once it returns and
        // it moves to the next instruction, we'll check for the
        // instruction pointer to equal NK_UINT_MAX, indicating that
        // the function call is complete.
        vm->instructionPointer = (NK_UINT_MAX - 1);

        // Execute the call instruction. When this returns we will
        // either be inside the function in the VM, or we will have
        // just returned from the C function the function ID is
        // associated with. Or an error.
        nkiOpcode_call(vm);
        if(vm->errorState.firstError) {
            return;
        }

        // The call opcode sets us in a position right before the
        // start of the function, because the instruction pointer will
        // be incremented after the call opcode returns during the
        // normal instruction iteration process. We have to mimic that
        // normal instruction iteration process as long as we're using
        // the opcode directly.
        vm->instructionPointer++;

        // If the function call was a C function, we should have our
        // instruction pointer at NK_UINT_MAX right now. Otherwise
        // it'll be at the start of the function, with a return
        // pointer of NK_UINT_MAX-1. So to execute instructions util
        // we return, we'll just keep executing until we hit an error,
        // or the instruction pointer is at NK_UINT_MAX
        // (NK_UINT_MAX-1, +1 for the instruction iteration).
        while(vm->instructionPointer != NK_UINT_MAX &&
            !vm->errorState.firstError)
        {
            nkiDbgWriteLine("In nkiVmCallFunction %u", vm->instructionPointer);
            nkiVmIterate(vm);
        }

        // Save return value.
        if(returnValue) {
            *returnValue = *nkiVmStackPop(vm);
        } else {
            nkiVmStackPop(vm);
        }

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
            return &vm->staticSpace[vm->staticAddressMask & vm->globalVariables[i].staticPosition];
        }
    }
    return NULL;
}

void nkiVmStaticDump(struct NKVM *vm)
{
    nkuint32_t i = 0;
    while(1) {

        printf("%3d: ", i);
        nkiValueDump(vm, &vm->staticSpace[i]);
        printf("\n");

        if(i == vm->staticAddressMask) {
            break;
        }
        i++;
    }
}

NKVMExternalFunctionID nkiVmRegisterExternalFunction(
    struct NKVM *vm,
    const char *name,
    NKVMFunctionCallback func)
{
    // Lookup function first, to make sure we aren't making duplicate
    // functions. (We're probably at compile time right now so we can
    // spend some time searching for this.)
    NKVMExternalFunctionID externalFunctionId = { NK_INVALID_VALUE };
    for(externalFunctionId.id = 0; externalFunctionId.id < vm->externalFunctionCount; externalFunctionId.id++) {
        if(vm->externalFunctionTable[externalFunctionId.id].CFunctionCallback == func &&
            !strcmp(vm->externalFunctionTable[externalFunctionId.id].name, name))
        {
            printf("FUNCTION FOUND ALREADY: %s\n", name);
            break;
        }
    }

    if(externalFunctionId.id == vm->externalFunctionCount) {
        // Function not found. Allocate a new one.
        return nkiVmRegisterExternalFunctionNoSearch(vm, name, func);
    }

    return externalFunctionId;
}

NKVMExternalFunctionID nkiVmRegisterExternalFunctionNoSearch(
    struct NKVM *vm,
    const char *name,
    NKVMFunctionCallback func)
{
    struct NKVMExternalFunction *funcEntry;

    vm->externalFunctionCount++;
    if(!vm->externalFunctionCount) {
        vm->externalFunctionCount--;
        nkiAddError(vm, -1, "Too many external functions registered.");
        {
            NKVMExternalFunctionID ret = { NK_INVALID_VALUE };
            return ret;
        }
    }

    vm->externalFunctionTable = nkiRealloc(
        vm, vm->externalFunctionTable,
        vm->externalFunctionCount * sizeof(struct NKVMExternalFunction));

    funcEntry = &vm->externalFunctionTable[vm->externalFunctionCount - 1];
    memset(funcEntry, 0, sizeof(*funcEntry));
    funcEntry->internalFunctionId.id = NK_INVALID_VALUE;
    funcEntry->name = nkiStrdup(vm, name);
    funcEntry->CFunctionCallback = func;

    {
        NKVMExternalFunctionID ret;
        ret.id = vm->externalFunctionCount - 1;
        return ret;
    }
}

NKVMInternalFunctionID nkiVmGetOrCreateInternalFunctionForExternalFunction(
    struct NKVM *vm, NKVMExternalFunctionID externalFunctionId)
{
    NKVMInternalFunctionID functionId = { NK_INVALID_VALUE };

    if(externalFunctionId.id >= vm->externalFunctionCount) {
        nkiAddError(
            vm, -1,
            "Tried to create an internal function to represent a bad external function.");
        return functionId;
    }

    if(vm->externalFunctionTable[externalFunctionId.id].internalFunctionId.id == NK_INVALID_VALUE) {

        // Gotta make a new function. Nothing exists yet.
        struct NKVMFunction *vmfunc =
            nkiVmCreateFunction(vm, &functionId);

        memset(vmfunc, 0, sizeof(*vmfunc));
        vmfunc->argumentCount = NK_INVALID_VALUE;

        // Set up function ID mappings.
        vmfunc->externalFunctionId = externalFunctionId;
        vm->externalFunctionTable[externalFunctionId.id].internalFunctionId = functionId;

    } else {

        // Some function already exists in the VM with this external
        // function ID.
        functionId = vm->externalFunctionTable[externalFunctionId.id].internalFunctionId;
    }

    return functionId;
}

// ----------------------------------------------------------------------
// External data interface

NKVMExternalDataTypeID nkiVmRegisterExternalType(
    struct NKVM *vm, const char *name)
{
    NKVMExternalDataTypeID ret = nkiVmFindExternalType(vm, name);
    if(ret.id != NK_INVALID_VALUE) {
        return ret;
    }

    if(vm->externalTypeCount == NK_INVALID_VALUE) {
        nkiAddError(vm, -1, "Allocated too many types.");
        return ret;
    }

    ret.id = vm->externalTypeCount;

    vm->externalTypeCount++;
    vm->externalTypeNames = nkiRealloc(
        vm, vm->externalTypeNames,
        sizeof(*(vm->externalTypeNames)) * vm->externalTypeCount);

    vm->externalTypeNames[ret.id] = nkiStrdup(vm, name);

    return ret;
}

NKVMExternalDataTypeID nkiVmFindExternalType(
    struct NKVM *vm, const char *name)
{
    NKVMExternalDataTypeID ret = { NK_INVALID_VALUE };
    nkuint32_t i;
    for(i = 0; i < vm->externalTypeCount; i++) {
        if(!strcmp(vm->externalTypeNames[i], name)) {
            ret.id = i;
            return ret;
        }
    }
    return ret;
}

const char *nkiVmGetExternalTypeName(
    struct NKVM *vm, NKVMExternalDataTypeID id)
{
    if(id.id >= vm->externalTypeCount) {
        return "";
    }
    return vm->externalTypeNames[id.id];
}

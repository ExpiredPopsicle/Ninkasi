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
//   By Kiri "ExpiredPopsicle" Jolly
//     https://expiredpopsicle.com
//     https://intoxicoding.com
//     expiredpopsicle@gmail.com
//
// ----------------------------------------------------------------------
//
//   Copyright (c) 2017 Kiri Jolly
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

#define NK_SETUP_OP(x, y, z)                                \
    do {                                                    \
        nkiOpcodeNameTable[(x)] = #x + nkiStrlen("NK_OP_"); \
        nkiOpcodeTable[(x)] = (y);                          \
        nkiCompilerStackOffsetTable[(x)] = (z);             \
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

const char *nkiVmGetOpcodeName(enum NKOpcode op)
{
    return nkiOpcodeNameTable[op & (NK_OPCODE_PADDEDCOUNT - 1)];
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

#if NK_EXTRA_FANCY_LEAK_TRACKING_LINUX
    vm->allocationCount = 0;
#endif //NK_EXTRA_FANCY_LEAK_TRACKING_LINUX

    vm->limits.maxStackSize = NK_UINT_MAX;
    vm->limits.maxFieldsPerObject = NK_UINT_MAX;
    vm->limits.maxAllocatedMemory = NK_UINT_MAX;
    vm->instructionsLeftBeforeTimeout = NK_INVALID_VALUE;

    vm->userData = NULL;
    vm->externalFunctionCount = 0;
    vm->externalFunctionTable = NULL;

    nkiVmInitOpcodeTable();

    nkiErrorStateInit(vm);
    nkiVmStackInit(vm);
    vm->instructionPointer = 0;

    vm->instructions =
        (struct NKInstruction *)nkiMalloc(vm, sizeof(struct NKInstruction) * 4);
    vm->instructionAddressMask = 0x3;
    nkiMemset(vm->instructions, 0, sizeof(struct NKInstruction) * 4);

    nkiVmStringTableInit(vm);

    vm->gcInfo.lastGCPass = 0;
    vm->gcInfo.gcInterval = 1024;
    vm->gcInfo.gcCountdown = vm->gcInfo.gcInterval;
    vm->gcInfo.gcNewObjectInterval = 256;
    vm->gcInfo.gcNewObjectCountdown = vm->gcInfo.gcNewObjectInterval;

    vm->functionCount = 0;
    vm->functionTable = NULL;

    vm->globalVariableCount = 0;
    vm->globalVariables = NULL;

    nkiVmObjectTableInit(vm);

    // Start with two static values (so our static value mask starts
    // off at 1).
    vm->staticSpace = (struct NKValue *)nkiMalloc(
        vm, 2 * sizeof(struct NKValue));
    nkiMemset(vm->staticSpace, 0, 2 * sizeof(struct NKValue));
    vm->staticAddressMask = 1;

    vm->externalTypes = NULL;
    vm->externalTypeCount = 0;

    nkiMemset(vm->subsystemDataTable, 0, sizeof(vm->subsystemDataTable));

    vm->sourceFileCount = 0;
    vm->sourceFileList = NULL;

    vm->positionMarkerList = NULL;
    vm->positionMarkerCount = 0;
}

void nkiVmDestroy(struct NKVM *vm)
{
    // The external data table is still good, regardless of allocation
    // failure status. Make sure we run all of our cleanup functions.
    {
        nkuint32_t i;
        for(i = 0; i < nkiVmExternalSubsystemHashTableSize; i++) {
            while(vm->subsystemDataTable[i]) {
                struct NKVMExternalSubsystemData *data = vm->subsystemDataTable[i];
                struct NKVMExternalSubsystemData *next = data->nextInHashTable;
                if(data->cleanupCallback) {
                    data->cleanupCallback(vm, data->data);
                }
                nkiFree(vm, data->name);
                nkiFree(vm, data);
                vm->subsystemDataTable[i] = next;
            }
        }
    }

    if(vm->errorState.allocationFailure) {

        // Catastrophic failure cleanup mode. Do not trust most
        // internal pointers and use the allocation tracker directly.

        // Now just free every allocation we've made.
        while(vm->allocations) {
            struct NKMemoryHeader *next = vm->allocations->nextAllocation;
#if NK_EXTRA_FANCY_LEAK_TRACKING_LINUX
            free(vm->allocations->stackTrace);
#endif
            free(vm->allocations);
            vm->allocations = next;
        }

#if NK_EXTRA_FANCY_LEAK_TRACKING_LINUX
        nkiDumpLeakData(vm);
#endif

    } else {

        // Standard cleanup mode.

        NK_FAILURE_RECOVERY_DECL();

        NK_SET_FAILURE_RECOVERY_VOID();

        // Nuke the entire static address space and stack, then force
        // a final garbage collection pass before we start making the
        // VM unusable, so that GC callbacks get hit for external data
        // and that stuff can clean up in a conventional manner.
        nkiVmStackClear(vm, nkfalse);
        nkiMemset(
            vm->staticSpace, 0,
            (vm->staticAddressMask + 1) * sizeof(struct NKValue));

        nkiVmGarbageCollect(vm);

        nkiVmObjectTableCleanAllObjects(vm);

        nkiVmStringTableCleanAllStrings(vm);

        nkiVmStringTableDestroy(vm);

        nkiVmStackDestroy(vm);
        nkiErrorStateDestroy(vm);
        nkiFree(vm, vm->instructions);
        nkiFree(vm, vm->functionTable);

        // Free global variable records.
        {
            nkuint32_t n;
            for(n = 0; n < vm->globalVariableCount; n++) {
                nkiFree(vm, vm->globalVariables[n].name);
            }
            nkiFree(vm, vm->globalVariables);
        }

        nkiVmObjectTableDestroy(vm);

        nkiFree(vm, vm->staticSpace);

        // Free external function table.
        {
            nkuint32_t n;
            for(n = 0; n < vm->externalFunctionCount; n++) {
                nkiFree(vm, vm->externalFunctionTable[n].name);
                nkiFree(vm, vm->externalFunctionTable[n].argTypes);
                nkiFree(vm, vm->externalFunctionTable[n].argExternalTypes);
            }
        }
        nkiFree(vm, vm->externalFunctionTable);

        // Free external type data.
        {
            nkuint32_t n;
            if(vm->externalTypes) {
                for(n = 0; n < vm->externalTypeCount; n++) {
                    nkiFree(vm, vm->externalTypes[n].name);
                }
                nkiFree(vm, vm->externalTypes);
            }
        }

        // Free source file list.
        nkiVmClearSourceFileList(vm);

        // Clear marker list.
        nkiFree(vm, vm->positionMarkerList);
        vm->positionMarkerList = NULL;
        vm->positionMarkerCount = 0;

#if NK_EXTRA_FANCY_LEAK_TRACKING_LINUX
        nkiDumpLeakData(vm);
#endif

        NK_CLEAR_FAILURE_RECOVERY();
    }

    assert(!vm->allocations);
 }

// ----------------------------------------------------------------------
// Iteration

void nkiVmIterate(struct NKVM *vm)
{
    const nkuint32_t opcodeMask = (NK_OPCODE_PADDEDCOUNT - 1);
    struct NKInstruction *inst = &vm->instructions[
        vm->instructionPointer & vm->instructionAddressMask];
    nkuint32_t opcodeId = inst->opcode & opcodeMask;

    // Handle periodic garbage collection. Do this BEFORE the
    // instruction, so we can handle instruction count limits in here
    // too.
    vm->gcInfo.gcCountdown--;
    if(!vm->gcInfo.gcCountdown) {

        // Thanks AFL!
        if(vm->instructionsLeftBeforeTimeout != NK_INVALID_VALUE) {
            if(vm->instructionsLeftBeforeTimeout < vm->gcInfo.gcInterval) {
                nkiAddError(vm, "Instruction count limit reached.");
                vm->instructionsLeftBeforeTimeout = 0;
                return;
            }
            vm->instructionsLeftBeforeTimeout -= vm->gcInfo.gcInterval;
        }

        if(!vm->gcInfo.gcNewObjectCountdown) {
            nkiVmGarbageCollect(vm);
            vm->gcInfo.gcNewObjectCountdown = vm->gcInfo.gcNewObjectInterval;
        }
        vm->gcInfo.gcCountdown = vm->gcInfo.gcInterval;
    }

    // Do the instruction.
    nkiOpcodeTable[opcodeId](vm);
    vm->instructionPointer++;
}

nkbool nkiVmExecuteProgram(struct NKVM *vm)
{
    while(vm->instructions[
            vm->instructionPointer &
            vm->instructionAddressMask].opcode != NK_OP_END)
    {
        nkiVmIterate(vm);

        if(nkiVmHasErrors(vm)) {
            return nkfalse;
        }
    }

    return nktrue;
}

struct NKValue *nkiVmFindGlobalVariable(
    struct NKVM *vm, const char *name)
{
    nkuint32_t i;
    for(i = 0; i < vm->globalVariableCount; i++) {
        if(!nkiStrcmp(vm->globalVariables[i].name, name)) {
            return &vm->staticSpace[vm->staticAddressMask & vm->globalVariables[i].staticPosition];
        }
    }
    return NULL;
}

// ----------------------------------------------------------------------
// External data interface

NKVMExternalDataTypeID nkiVmRegisterExternalType(
    struct NKVM *vm, const char *name,
    NKVMExternalObjectSerializationCallback serializationCallback,
    NKVMExternalObjectCleanupCallback cleanupCallback,
    NKVMExternalObjectGCMarkCallback gcMarkCallback)
{
    NKVMExternalDataTypeID ret = nkiVmFindExternalType(vm, name);
    if(ret.id != NK_INVALID_VALUE) {
        ret.id = NK_INVALID_VALUE;
        nkiAddError(vm, "Attempted to register an external type twice.");
        return ret;
    }

    if(vm->externalTypeCount == NK_INVALID_VALUE) {
        ret.id = NK_INVALID_VALUE;
        nkiAddError(vm, "Allocated too many types.");
        return ret;
    }

    ret.id = vm->externalTypeCount;

    // Note: Increment after reallocation to maintain correct state at
    // all times, in case of alloc failure.
    vm->externalTypes = (struct NKVMExternalType *)nkiReallocArray(
        vm, vm->externalTypes,
        sizeof(*(vm->externalTypes)),
        vm->externalTypeCount + 1);
    vm->externalTypeCount++;

    nkiMemset(
        &vm->externalTypes[ret.id], 0,
        sizeof(vm->externalTypes[ret.id]));

    vm->externalTypes[ret.id].name = nkiStrdup(vm, name);
    vm->externalTypes[ret.id].serializationCallback =
        serializationCallback;
    vm->externalTypes[ret.id].cleanupCallback =
        cleanupCallback;
    vm->externalTypes[ret.id].gcMarkCallback =
        gcMarkCallback;

    return ret;
}

NKVMExternalDataTypeID nkiVmFindExternalType(
    struct NKVM *vm, const char *name)
{
    NKVMExternalDataTypeID ret = { NK_INVALID_VALUE };
    nkuint32_t i;

    if(vm->externalTypes) {
        for(i = 0; i < vm->externalTypeCount; i++) {
            if(!nkiStrcmp(vm->externalTypes[i].name, name)) {
                ret.id = i;
                return ret;
            }
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
    return vm->externalTypes[id.id].name;
}

struct NKVMExternalSubsystemData *nkiFindExternalSubsystemData(
    struct NKVM *vm,
    const char *name,
    nkbool create)
{
    nkuint32_t hash = nkiStringHash(name);
    nkuint32_t bucketIndex = hash & (nkiVmExternalSubsystemHashTableSize - 1);
    struct NKVMExternalSubsystemData *el = vm->subsystemDataTable[bucketIndex];

    while(el) {
        if(!nkiStrcmp(el->name, name)) {
            return el;
        }
        el = el->nextInHashTable;
    }

    if(create) {
        el = (struct NKVMExternalSubsystemData *)nkiMalloc(
            vm, sizeof(*el));
        nkiMemset(el, 0, sizeof(*el));
        el->nextInHashTable = vm->subsystemDataTable[bucketIndex];
        el->name = nkiStrdup(vm, name);
        vm->subsystemDataTable[bucketIndex] = el;
        return el;
    }

    return NULL;
}

void *nkiGetExternalSubsystemData(
    struct NKVM *vm,
    const char *name)
{
    struct NKVMExternalSubsystemData *externalSubsystemData =
        nkiFindExternalSubsystemData(vm, name, nkfalse);

    if(externalSubsystemData) {
        return externalSubsystemData->data;
    }

    return NULL;
}

nkuint32_t nkiVmAddSourceFile(struct NKVM *vm, const char *filename)
{
    nkuint32_t i;
    nkuint32_t newCount = vm->sourceFileCount + 1;
    char **newList = NULL;
    nkuint32_t newIndex = vm->sourceFileCount;

    // First search the list for it.
    for(i = 0; i < vm->sourceFileCount; i++) {
        if(nkiStrcmp(filename, vm->sourceFileList[i]) == 0) {
            return i;
        }
    }

    if(newCount == NK_INVALID_VALUE) {
        return NK_INVALID_VALUE;
    }

    newList = (char**)nkiReallocArray(vm, vm->sourceFileList, sizeof(char*), newCount);
    vm->sourceFileList = newList;
    vm->sourceFileCount = newCount;

    // Set to NULL first, so an allocation failure on the strdup
    // doesn't leave us with garbage here.
    vm->sourceFileList[newIndex] = NULL;

    vm->sourceFileList[newIndex] = nkiStrdup(vm, filename);

    return newIndex;
}

void nkiVmClearSourceFileList(struct NKVM *vm)
{
    nkuint32_t i;

    if(vm->sourceFileList) {
        for(i = 0; i < vm->sourceFileCount; i++) {
            nkiFree(vm, vm->sourceFileList[i]);
            vm->sourceFileList[i] = NULL;
        }
        nkiFree(vm, vm->sourceFileList);
        vm->sourceFileList = NULL;
    }
}

struct NKVMFilePositionMarker *nkiVmFindSourceMarker(
    struct NKVM *vm, nkuint32_t instructionIndex)
{
    nkuint32_t i;

    if(!vm->positionMarkerList) {
        return NULL;
    }

    if(!vm->positionMarkerCount) {
        return NULL;
    }

    for(i = 0; i + 1 < vm->positionMarkerCount; i++) {
        if(vm->positionMarkerList[i+1].instructionIndex > instructionIndex) {
            break;
        }
    }

    return &vm->positionMarkerList[i];
}

struct NKVMFilePositionMarker *nkiVmFindCurrentSourceMarker(struct NKVM *vm)
{
    return nkiVmFindSourceMarker(
        vm,
        vm->instructionPointer & vm->instructionAddressMask);
}


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
#include "nkconfig.h"

// Wrapper for all of our serialization helpers that just adds some
// error checking and handling.
#define NKI_WRAPSERIALIZE(x)                    \
    do {                                        \
        if(!(x)) {                              \
            return nkfalse;                     \
        }                                       \
    } while(0)

#define NKI_SERIALIZE_DATA(data, size)                              \
    NKI_WRAPSERIALIZE(writer((void*)(data), (size), userdata, writeMode))

#define NKI_SERIALIZE_BASIC(t, val)                 \
    do {                                            \
        t tmp = (val);                              \
        NKI_SERIALIZE_DATA(&(val), sizeof(tmp));    \
    } while(0)

nkbool nkiSerializeString_save(
    struct NKVM *vm,
    NKVMSerializationWriter writer,
    void *userdata,
    const char *str)
{
    nkbool writeMode = nktrue;

    size_t lenFull = strlen(str);
    nkuint32_t len = (nkuint32_t)lenFull;
    if(lenFull >= NK_UINT_MAX) {
        nkiAddError(vm, -1, "String too long to serialize.");
        return nkfalse;
    }

    // Write length.
    NKI_SERIALIZE_BASIC(nkuint32_t, len);

    // Write actual string.
    NKI_SERIALIZE_DATA(str, len);

    return nktrue;
}

nkbool nkiSerializeString_load(
    struct NKVM *vm,
    NKVMSerializationWriter writer,
    void *userdata,
    char **outData)
{
    nkbool writeMode = nkfalse;
    nkuint32_t len = 0;

    // Read length.
    NKI_SERIALIZE_BASIC(nkuint32_t, len);
    if(len >= NK_UINT_MAX) {
        nkiAddError(vm, -1, "String too long to deserialize.");
        return nkfalse;
    }

    *outData = nkiMalloc(vm, len + 1);

    // Read actual string.
    NKI_SERIALIZE_DATA(*outData, len);
    (*outData)[len] = 0;

    return nktrue;
}

#define NKI_SERIALIZE_STRING(str)                                       \
    if(writeMode) {                                                     \
        NKI_WRAPSERIALIZE(nkiSerializeString_save(vm, writer, userdata, (str))); \
    } else {                                                            \
        char *outData = NULL;                                           \
        NKI_WRAPSERIALIZE(nkiSerializeString_load(vm, writer, userdata, &outData)); \
        (str) = outData;                                                \
    }

nkbool nkiSerializeErrorState(
    struct NKVM *vm, NKVMSerializationWriter writer,
    void *userdata, nkbool writeMode)
{
    nkuint32_t errorCount = nkiGetErrorCount(vm);

    // Save error count.
    NKI_SERIALIZE_BASIC(nkuint32_t, errorCount);
    NKI_SERIALIZE_BASIC(nkbool, vm->errorState.allocationFailure);

    if(writeMode) {

        struct NKError *err = vm->errorState.firstError;

        while(err) {
            char *tmp = err->errorText;
            NKI_SERIALIZE_STRING(tmp);
            err = err->next;
        }

    } else {

        nkuint32_t errorNum;
        struct NKError **lastPtr = &vm->errorState.firstError;

        for(errorNum = 0; errorNum < errorCount; errorNum++) {
            struct NKError *newError = nkiMalloc(vm, sizeof(struct NKError));
            NKI_SERIALIZE_STRING(newError->errorText);
            newError->next = NULL;
            *lastPtr = newError;
            lastPtr = &newError->next;
        }

    }

    return nktrue;
}
nkbool nkiSerializeObject(
    struct NKVM *vm, struct NKVMObject *object,
    NKVMSerializationWriter writer, void *userdata,
    nkbool writeMode)
{
    NKI_SERIALIZE_BASIC(nkuint32_t, object->objectTableIndex);
    NKI_SERIALIZE_BASIC(nkuint32_t, object->size);
    NKI_SERIALIZE_BASIC(nkuint32_t, object->externalHandleCount);
    NKI_SERIALIZE_BASIC(nkuint32_t, object->lastGCPass);
    NKI_SERIALIZE_BASIC(NKVMInternalFunctionID, object->gcCallback);
    NKI_SERIALIZE_BASIC(NKVMInternalFunctionID, object->serializationCallback);
    NKI_SERIALIZE_BASIC(NKVMExternalDataTypeID, object->externalDataType);

    // Serialize all hash buckets.
    if(writeMode) {
        nkuint32_t n;
        for(n = 0; n < nkiVMObjectHashBucketCount; n++) {
            struct NKVMObjectElement *el = object->hashBuckets[n];
            while(el) {
                NKI_SERIALIZE_BASIC(struct NKValue, el->key);
                NKI_SERIALIZE_BASIC(struct NKValue, el->value);
                el = el->next;
            }
        }
    } else {
        nkuint32_t loadedSize = object->size;
        nkuint32_t n;
        object->size = 0;
        for(n = 0; n < loadedSize; n++) {
            struct NKValue key;
            struct NKValue *value;
            memset(&key, 0, sizeof(key));
            NKI_SERIALIZE_BASIC(struct NKValue, key);
            value = nkiVmObjectFindOrAddEntry(vm, object, &key, nkfalse);
            if(value) {
                NKI_SERIALIZE_BASIC(struct NKValue, *value);
            }
        }
    }

    // External serialization callback.
    if(object->serializationCallback.id != NK_INVALID_VALUE) {
        if(object->serializationCallback.id < vm->functionCount) {
            struct NKVMFunction *func = &vm->functionTable[object->serializationCallback.id];
            if(func->externalFunctionId.id != NK_INVALID_VALUE) {
                if(func->externalFunctionId.id < vm->externalFunctionCount) {
                    struct NKValue funcValue;
                    struct NKValue argValue;
                    memset(&funcValue, 0, sizeof(funcValue));
                    funcValue.type = NK_VALUETYPE_FUNCTIONID;
                    funcValue.functionId = object->serializationCallback;
                    memset(&argValue, 0, sizeof(argValue));
                    argValue.type = NK_VALUETYPE_OBJECTID;
                    argValue.objectId = object->objectTableIndex;

                    {
                        NKVMSerializationWriter oldWriter = vm->serializationState.writer;
                        void *oldUserdata = vm->serializationState.userdata;
                        nkbool oldWriteMode = vm->serializationState.writeMode;
                        vm->serializationState.writer = writer;
                        vm->serializationState.userdata = userdata;
                        vm->serializationState.writeMode = writeMode;
                        nkiVmCallFunction(vm, &funcValue, 1, &argValue, NULL);
                        vm->serializationState.userdata = oldUserdata;
                        vm->serializationState.writer = oldWriter;
                        vm->serializationState.writeMode = oldWriteMode;
                    }
                }
            }
        }
    }

    // If we're loading, we need to reconstruct the external handle
    // list.
    if(!writeMode) {
        if(object->externalHandleCount) {
            if(vm->objectsWithExternalHandles) {
                vm->objectsWithExternalHandles->previousExternalHandleListPtr =
                    &object->nextObjectWithExternalHandles;
            }
            object->previousExternalHandleListPtr = &vm->objectsWithExternalHandles;
            object->nextObjectWithExternalHandles = vm->objectsWithExternalHandles;
            vm->objectsWithExternalHandles = object;
        }
    }

    return nktrue;
}

nkbool nkiIsPow2(nkuint32_t x)
{
    if(x && (x & (x - 1))) {
        return nkfalse;
    }
    return nktrue;
}

nkbool nkiSerializeObjectTable(
    struct NKVM *vm, NKVMSerializationWriter writer,
    void *userdata, nkbool writeMode)
{
    nkuint32_t objectCount = 0;

    // FIXME: Ensure capacity is a power of two, or find a
    // better way to store it!
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->objectTable.capacity);
    if(!nkiIsPow2(vm->objectTable.capacity)) {
        nkiAddError(vm, -1, "Object table capacity is not a power of two.");
        return nkfalse;
    }

    if(writeMode) {

        // Count up objects if we're writing.
        nkuint32_t i;
        for(i = 0; i < vm->objectTable.capacity; i++) {
            if(vm->objectTable.objectTable[i]) {
                objectCount++;
            }
        }

    } else {

        // Reallocate the object table if we're reading.
        nkiFree(vm, vm->objectTable.objectTable);
        vm->objectTable.objectTable =
            nkiMallocArray(vm, sizeof(struct NKVMObject *), vm->objectTable.capacity);
        memset(
            vm->objectTable.objectTable, 0,
            sizeof(struct NKVMObject *) * vm->objectTable.capacity);

        // Free the holes.
        while(vm->objectTable.tableHoles) {
            struct NKVMTableHole *hole = vm->objectTable.tableHoles;
            vm->objectTable.tableHoles = hole->next;
            nkiFree(vm, hole);
        }
    }

    NKI_SERIALIZE_BASIC(nkuint32_t, objectCount);
    if(objectCount > vm->objectTable.capacity) {
        nkiAddError(vm, -1, "Object count exceeds object table capacity.");
        return nkfalse;
    }

    if(writeMode) {

        nkuint32_t i;
        for(i = 0; i < vm->objectTable.capacity; i++) {
            struct NKVMObject *object = vm->objectTable.objectTable[i];
            if(object) {
                assert(i == object->objectTableIndex);
                NKI_SERIALIZE_BASIC(nkuint32_t, object->objectTableIndex);
                NKI_WRAPSERIALIZE(
                    nkiSerializeObject(
                        vm, object,
                        writer, userdata, writeMode));
            }
        }

    } else {

        nkuint32_t i;
        for(i = 0; i < objectCount; i++) {
            struct NKVMObject *object = nkiMalloc(vm, sizeof(struct NKVMObject));
            memset(object, 0, sizeof(struct NKVMObject));

            NKI_SERIALIZE_BASIC(nkuint32_t, object->objectTableIndex);

            // Thanks AFL! Holy crap I'm an idiot for letting this one
            // slide by.
            if(object->objectTableIndex >= vm->objectTable.capacity) {
                nkiAddError(vm, -1, "Object index exceeds object table capacity.");
                return nkfalse;
            }

            vm->objectTable.objectTable[object->objectTableIndex] = object;
            NKI_WRAPSERIALIZE(
                nkiSerializeObject(
                    vm, object,
                    writer, userdata, writeMode));
        }
    }

    // Recreate holes for read mode.
    if(!writeMode) {
        nkuint32_t i;
        for(i = 0; i < vm->objectTable.capacity; i++) {
            if(!vm->objectTable.objectTable[i]) {
                struct NKVMTableHole *hole =
                    nkiMalloc(vm, sizeof(struct NKVMTableHole));
                hole->next = vm->objectTable.tableHoles;
                hole->index = i;
                vm->objectTable.tableHoles = hole;
            }
        }
    }

    return nktrue;
}

nkbool nkiSerializeStringTable(
    struct NKVM *vm, NKVMSerializationWriter writer,
    void *userdata, nkbool writeMode)
{
    nkuint32_t capacity = vm->stringTable.capacity;

    NKI_SERIALIZE_BASIC(nkuint32_t, capacity);

    // Destroy and recreate the string table with the specified
    // capacity, if we're in READ mode.
    if(!writeMode) {
        nkiVmStringTableDestroy(vm);
        vm->stringTable.capacity = capacity;

        // Thanks AFL! String table size can cause a 32-bit integer
        // overflow after being multiplied by the size of the string
        // pointer. (FIXME: We might not need this after the addition
        // of nkiMallocArray.)
        if(~(nkuint32_t)0 / sizeof(struct NKVMString*) <= vm->stringTable.capacity) {
            nkiAddError(vm, -1, "String table too large.");
            return nkfalse;
        }

        vm->stringTable.stringTable =
            nkiMallocArray(vm, sizeof(struct NKVMString*), vm->stringTable.capacity);

        // Note: Memset takes a size_t here, which on 64-bit bit can
        // take higher values than our nkuint32_t type passed into
        // nkiMalloc, meaning that this command can take larger values
        // than it's possible for us to allocate, even with the same
        // equation!
        memset(vm->stringTable.stringTable, 0,
            vm->stringTable.capacity * sizeof(struct NKVMString*));
    }

    // Thanks AFL!
    if(!nkiIsPow2(vm->stringTable.capacity)) {
        nkiAddError(vm, -1, "String table capacity is not a power of two.");
        return nkfalse;
    }

    {
        nkuint32_t actualCount = 0;

        // Count up the number of actual entries.
        if(writeMode) {
            nkuint32_t i;
            for(i = 0; i < vm->stringTable.capacity; i++) {
                if(vm->stringTable.stringTable[i]) {
                    actualCount++;
                }
            }
        }

        NKI_SERIALIZE_BASIC(nkuint32_t, actualCount);

        if(writeMode) {

            nkuint32_t i;
            for(i = 0; i < vm->stringTable.capacity; i++) {
                if(vm->stringTable.stringTable[i]) {
                    char *tmp = vm->stringTable.stringTable[i]->str;
                    NKI_SERIALIZE_BASIC(nkuint32_t, i);
                    NKI_SERIALIZE_STRING(tmp);
                    NKI_SERIALIZE_BASIC(nkuint32_t, vm->stringTable.stringTable[i]->lastGCPass);
                    NKI_SERIALIZE_BASIC(nkbool, vm->stringTable.stringTable[i]->dontGC);
                }
            }

        } else {

            nkuint32_t n;

            // Delete all hole objects. We'll recreate them later.
            while(vm->stringTable.tableHoles) {
                struct NKVMTableHole *hole = vm->stringTable.tableHoles;
                vm->stringTable.tableHoles = hole->next;
                nkiFree(vm, hole);
            }

            for(n = 0; n < actualCount; n++) {

                nkuint32_t index = 0;
                char *tmpStr = NULL;

                NKI_SERIALIZE_BASIC(nkuint32_t, index);
                if(index >= vm->stringTable.capacity) {
                    nkiAddError(vm, -1, "String index exceeds string table capacity.");
                    return nkfalse;
                }

                NKI_SERIALIZE_STRING(tmpStr);
                {
                    nkuint32_t stringLen = strlen(tmpStr);
                    nkuint32_t structAndPaddingSize = sizeof(*vm->stringTable.stringTable[index]) + 1;
                    nkuint32_t size = stringLen + structAndPaddingSize;

                    if(stringLen >= NK_UINT_MAX - structAndPaddingSize) {
                        nkiAddError(vm, -1, "A string is longer than the addressable space to load it into.");
                        return nkfalse;
                    }

                    // Thanks AFL! Bail out if the binary has two
                    // strings in the same slot.
                    if(vm->stringTable.stringTable[index]) {
                        nkiAddError(vm, -1, "Two strings occupy the same slot in the string table.");
                        return nkfalse;
                    }

                    // Allocate new string entry.
                    vm->stringTable.stringTable[index] =
                        nkiMalloc(vm, size);

                    // Clear it out.
                    memset(vm->stringTable.stringTable[index], 0, size);

                    // Copy the string data in.
                    memcpy(vm->stringTable.stringTable[index]->str, tmpStr, strlen(tmpStr) + 1);

                    // Free the string we loaded from the file.
                    nkiFree(vm, tmpStr);

                    NKI_SERIALIZE_BASIC(nkuint32_t, vm->stringTable.stringTable[index]->lastGCPass);
                    NKI_SERIALIZE_BASIC(nkbool, vm->stringTable.stringTable[index]->dontGC);

                    // Fix up some data.
                    vm->stringTable.stringTable[index]->stringTableIndex = index;
                    vm->stringTable.stringTable[index]->hash =
                        nkiStringHash(vm->stringTable.stringTable[index]->str);

                    // Add a hash table entry for this string.
                    {
                        const nkuint32_t hashMask = nkiVmStringHashTableSize - 1;
                        struct NKVMString *hashBucket =
                            vm->stringsByHash[
                                vm->stringTable.stringTable[index]->hash & hashMask];
                        vm->stringTable.stringTable[index]->nextInHashBucket = hashBucket;
                        vm->stringsByHash[
                            vm->stringTable.stringTable[index]->hash & hashMask] =
                            vm->stringTable.stringTable[index];
                    }
                }

            }

            // Delete and recreate hole objects.
            {
                nkuint32_t i;

                while(vm->stringTable.tableHoles) {
                    struct NKVMTableHole *hole = vm->stringTable.tableHoles;
                    vm->stringTable.tableHoles = hole->next;
                    nkiFree(vm, hole);
                }

                for(i = 0; i < vm->stringTable.capacity; i++) {
                    if(!vm->stringTable.stringTable[i]) {
                        struct NKVMTableHole *newHole = nkiMalloc(vm, sizeof(struct NKVMTableHole));
                        newHole->index = i;
                        newHole->next = vm->stringTable.tableHoles;
                        vm->stringTable.tableHoles = newHole;
                    }
                }
            }
        }
    }

    return nktrue;
}

nkbool nkiSerializeInstructions(struct NKVM *vm, NKVMSerializationWriter writer, void *userdata, nkbool writeMode)
{
    // Find the actual end of the instruction buffer.
    nkuint32_t instructionLimitSearch = vm->instructionAddressMask;
    if(writeMode) {
        while(instructionLimitSearch && vm->instructions[instructionLimitSearch].opcode == NK_OP_NOP) {
            instructionLimitSearch--;
        }
    }

    // Record how much information we're going to store in the stream.
    NKI_SERIALIZE_BASIC(nkuint32_t, instructionLimitSearch);

    // We still need the real instruction address mask. There's a
    // chance that some of that NK_OP_OP on the end was really
    // something like a literal value instead of a real opcode, and if
    // we cut off the address mask before it, that would read the
    // wrong value.
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->instructionAddressMask);

    // Thanks AFL! Instruction address mask of 2^32-1 means that the
    // actual allocation is 2^32, which overflows to zero, and we're
    // left in a situation where we think we have a giant instruction
    // buffer when we really have a NULL pointer.
    if(vm->instructionAddressMask == ~(nkuint32_t)0) {
        nkiAddError(vm, -1, "Instruction address mask is too large.");
        return nkfalse;
    }

    if(!nkiIsPow2(vm->instructionAddressMask + 1)) {
        nkiAddError(vm, -1, "Instruction address mask is not a power of two minus one.");
        return nkfalse;
    }

    if(instructionLimitSearch > vm->instructionAddressMask) {
        nkiAddError(vm, -1, "Too many instructions for given instruction address mask.");
        return nkfalse;
    }

    // Recreate the entire instruction space if we're reading.
    if(!writeMode) {
        nkiFree(vm, vm->instructions);

        // Thanks AFL!
        vm->instructions = nkiMallocArray(
            vm,
            sizeof(struct NKInstruction),
            vm->instructionAddressMask + 1);

        memset(vm->instructions, 0, sizeof(struct NKInstruction) * (vm->instructionAddressMask + 1));
    }

    // Load/save the actual instructions themselves.
    {
        nkuint32_t i;
        for(i = 0; i <= instructionLimitSearch; i++) {

            // Note: We're skipping the line number that's only
            // visible on the debug version of the structure here, for
            // binary compatibility between debug and non-debug
            // versions.

            NKI_SERIALIZE_BASIC(struct NKInstruction, vm->instructions[i]);
        }
    }

    return nktrue;
}

nkbool nkiSerializeStatics(struct NKVM *vm, NKVMSerializationWriter writer, void *userdata, nkbool writeMode)
{
    // Check that a valid mask is 2^n-1. (Or just find a better way to
    // store it.)
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->staticAddressMask);
    if(!nkiIsPow2(vm->staticAddressMask + 1)) {
        nkiAddError(vm, -1, "Static space address mask is not a power of two minus one.");
        return nkfalse;
    }

    if(vm->staticAddressMask >= NK_UINT_MAX) {
        nkiAddError(vm, -1, "Static space address mask is too large.");
        return nkfalse;
    }

    // Make new static space for read mode.
    if(!writeMode) {
        nkiFree(vm, vm->staticSpace);
        vm->staticSpace = nkiMallocArray(vm, sizeof(struct NKValue), vm->staticAddressMask + 1);
    }

    // Read/write the static space.
    NKI_SERIALIZE_DATA(
        vm->staticSpace,
        sizeof(struct NKValue) * (vm->staticAddressMask + 1));

    return nktrue;
}

nkbool nkiSerializeStack(struct NKVM *vm, NKVMSerializationWriter writer, void *userdata, nkbool writeMode)
{
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->stack.size);
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->stack.capacity);
    if(!nkiIsPow2(vm->stack.capacity)) {
        nkiAddError(vm, -1, "Stack capacity is not a power of two.");
        return nkfalse;
    }

    // Thanks AFL! Stack size = 0 means stack index mask becomes
    // 0xffffffff, even though there's no stack.
    if(vm->stack.capacity < 1) {
        nkiAddError(vm, -1, "Stack capacity is zero.");
        return nkfalse;
    }

    vm->stack.indexMask = vm->stack.capacity - 1;

    // Make new stack space for read mode.
    if(!writeMode) {
        nkiFree(vm, vm->stack.values);

        vm->stack.values = nkiMallocArray(vm,
            sizeof(struct NKValue), vm->stack.capacity);

        // Thanks AFL!
        if(!vm->stack.values) {
            return nkfalse;
        }

        memset(vm->stack.values, 0,
            sizeof(struct NKValue) * (vm->stack.capacity));
    }

    // Thanks AFL!
    if(vm->stack.capacity < vm->stack.size) {
        return nkfalse;
    }

    NKI_SERIALIZE_DATA(
        vm->stack.values,
        sizeof(struct NKValue) * (vm->stack.size));

    return nktrue;
}

nkbool nkiSerializeGcState(
    struct NKVM *vm, NKVMSerializationWriter writer,
    void *userdata, nkbool writeMode)
{
    // GC state. Exposing the garbage collector parameters to someone
    // writing a malicious binary is an issue if the memory usage
    // limits are not set, but right now I think we should go with the
    // most consistency between serialized and deserialized versions
    // of stuff.
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->gcInfo.lastGCPass);
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->gcInfo.gcInterval);

    // gcInterval needs a hard limit here because the instruction
    // count limit is only enforced on garbage collection intervals.
    if(vm->gcInfo.gcInterval > 65536) {
        vm->gcInfo.gcInterval = 65536;
    }

    NKI_SERIALIZE_BASIC(nkuint32_t, vm->gcInfo.gcCountdown);
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->gcInfo.gcNewObjectInterval);
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->gcInfo.gcNewObjectCountdown);

    return nktrue;
}

// Saves the external function table or match up existing external
// functions to loaded external functions at read time, then load/save
// internal functions that might reference them.
nkbool nkiSerializeFunctionTable(
    struct NKVM *vm, NKVMSerializationWriter writer,
    void *userdata, nkbool writeMode)
{
    NKVMExternalFunctionID *functionIdMapping = NULL;
    nkuint32_t tmpExternalFunctionCount = vm->externalFunctionCount;
    nkuint32_t i;

    // Start with external functions.

    NKI_SERIALIZE_BASIC(nkuint32_t, tmpExternalFunctionCount);

    // Allocate space for a temporary table where we'll keep track of
    // the mapping of the external function IDs in the serialized data
    // to the ones we already have already in the VM.
    if(!writeMode) {
        functionIdMapping = nkiMallocArray(
            vm, sizeof(NKVMInternalFunctionID), tmpExternalFunctionCount);
        memset(
            functionIdMapping, NK_INVALID_VALUE,
            tmpExternalFunctionCount * sizeof(NKVMInternalFunctionID));
    }

    for(i = 0; i < tmpExternalFunctionCount; i++) {

        struct NKVMExternalFunction tmpExternalFunc = {0};
        if(writeMode) {
            tmpExternalFunc = vm->externalFunctionTable[i];
        }

        NKI_SERIALIZE_BASIC(
            NKVMInternalFunctionID,
            tmpExternalFunc.internalFunctionId);

        NKI_SERIALIZE_STRING(
            tmpExternalFunc.name);

        // For loading, we need to find the existing external
        // function.
        if(!writeMode) {

            nkuint32_t n;

            for(n = 0; n < vm->externalFunctionCount; n++) {

                // Clear out the internal function value,
                // in case we don't actually find
                // something.
                vm->externalFunctionTable[n].internalFunctionId.id = NK_INVALID_VALUE;

                if(!strcmp(tmpExternalFunc.name, vm->externalFunctionTable[n].name)) {
                    functionIdMapping[i].id = n;
                    vm->externalFunctionTable[n].internalFunctionId =
                        tmpExternalFunc.internalFunctionId;
                    break;
                }
            }

            nkiFree(vm, tmpExternalFunc.name);
        }
    }

    // Internal function table time.

    NKI_SERIALIZE_BASIC(nkuint32_t, vm->functionCount);

    // Reallocate internal function table for read mode.
    if(!writeMode) {
        nkiFree(vm, vm->functionTable);
        vm->functionTable = nkiMallocArray(vm, sizeof(struct NKVMFunction), vm->functionCount);
        memset(vm->functionTable, 0, sizeof(struct NKVMFunction) * vm->functionCount);
    }

    for(i = 0; i < vm->functionCount; i++) {

        NKI_SERIALIZE_BASIC(nkuint32_t, vm->functionTable[i].argumentCount);
        NKI_SERIALIZE_BASIC(nkuint32_t, vm->functionTable[i].firstInstructionIndex);
        NKI_SERIALIZE_BASIC(NKVMExternalFunctionID, vm->functionTable[i].externalFunctionId);

        // Remap external function ID.
        if(!writeMode) {

            if(vm->functionTable[i].externalFunctionId.id != NK_INVALID_VALUE) {

                // Thanks AFL! There was a <= instead of a <
                // here.
                if(vm->functionTable[i].externalFunctionId.id < tmpExternalFunctionCount) {
                    vm->functionTable[i].externalFunctionId =
                        functionIdMapping[vm->functionTable[i].externalFunctionId.id];
                } else {
                    nkiAddError(vm, -1, "External function ID is outside external function count.");
                    return nkfalse;
                }
            }
        }

    }

    if(!writeMode) {
        nkiFree(vm, functionIdMapping);
    }

    return nktrue;
}

nkbool nkiSerializeGlobalsList(
    struct NKVM *vm, NKVMSerializationWriter writer,
    void *userdata, nkbool writeMode)
{
    nkuint32_t i;

    NKI_SERIALIZE_BASIC(nkuint32_t, vm->globalVariableCount);

    // If we're loading a file, nuke whatever might be in the VM
    // already.
    if(!writeMode) {
        nkiFree(vm, vm->globalVariables);
        nkiFree(vm, vm->globalVariableNameStorage);
        vm->globalVariables = nkiMallocArray(
            vm, sizeof(vm->globalVariables[0]),
            vm->globalVariableCount);
    }

    // Load/save each global name and index.
    for(i = 0; i < vm->globalVariableCount; i++) {
        NKI_SERIALIZE_STRING(vm->globalVariables[i].name);
        NKI_SERIALIZE_BASIC(nkuint32_t, vm->globalVariables[i].staticPosition);
    }

    // Convert individually heap-allocated names to a single chunk
    // of memory.
    if(!writeMode) {

        char *storagePtr;
        nkuint32_t storageSize = 0;

        for(i = 0; i < vm->globalVariableCount; i++) {
            storageSize += strlen(vm->globalVariables[i].name) + 1;
        }

        vm->globalVariableNameStorage = nkiMalloc(vm, storageSize);

        storagePtr = vm->globalVariableNameStorage;

        for(i = 0; i < vm->globalVariableCount; i++) {
            nkuint32_t len = strlen(vm->globalVariables[i].name) + 1;
            memcpy(storagePtr, vm->globalVariables[i].name, len);
            nkiFree(vm, (char*)vm->globalVariables[i].name);
            vm->globalVariables[i].name = storagePtr;
            storagePtr += len;
        }
    }

    return nktrue;
}

nkbool nkiSerializeExternalTypes(
    struct NKVM *vm, NKVMSerializationWriter writer,
    void *userdata, nkbool writeMode)
{
    nkuint32_t serializedTypeCount = vm->externalTypeCount;
    nkuint32_t i;
    nkuint32_t *typeMapping = NULL;

    NKI_SERIALIZE_BASIC(nkuint32_t, serializedTypeCount);

    // Allocate an array if we have to map types between the binary
    // and existing types.
    if(!writeMode) {
        typeMapping = nkiMallocArray(vm, sizeof(nkuint32_t), serializedTypeCount);
    }

    for(i = 0; i < serializedTypeCount; i++) {

        char *typeName = NULL;
        if(writeMode) {
            typeName = vm->externalTypeNames[i];
        }

        NKI_SERIALIZE_STRING(typeName);

        if(!writeMode) {

            nkuint32_t n;

            // Search for an existing type with the same name as the
            // one we're loading.
            for(n = 0; n < vm->externalTypeCount; n++) {
                if(!strcmp(vm->externalTypeNames[n], typeName)) {
                    typeMapping[i] = n;
                    break;
                }
            }

            if(n == vm->externalTypeCount) {
                nkiAddError(vm, -1, "Could not find a matching type for deserialized external data type.");
                return nkfalse;
            }

            nkiFree(vm, typeName);
        }
    }

    // If we're loading, go through the entire object table and
    // reassign every external type ID based on the mapping we figured
    // out.
    if(!writeMode) {

        nkuint32_t i;

        for(i = 0; i < vm->objectTable.capacity; i++) {
            if(vm->objectTable.objectTable[i]) {
                if(vm->objectTable.objectTable[i]->externalDataType.id != NK_INVALID_VALUE) {
                    if(vm->objectTable.objectTable[i]->externalDataType.id < serializedTypeCount) {
                        vm->objectTable.objectTable[i]->externalDataType.id =
                            typeMapping[vm->objectTable.objectTable[i]->externalDataType.id];
                    } else {
                        return nkfalse;
                    }
                }
            }
        }

        nkiFree(vm, typeMapping);
    }

    return nktrue;
}

#define NKI_VERSION 1

nkbool nkiVmSerialize(struct NKVM *vm, NKVMSerializationWriter writer, void *userdata, nkbool writeMode)
{
    // Serialize format marker.
    {
        const char *formatMarker = "\0NKVM";
        char formatMarkerTmp[5];
        memcpy(formatMarkerTmp, formatMarker, 5);
        NKI_SERIALIZE_DATA(formatMarkerTmp, 5);
        if(memcmp(formatMarkerTmp, formatMarker, 5)) {
            nkiAddError(vm, -1, "Wrong binary format.");
            return nkfalse;
        }
    }

    // Serialize version number.
    {
        nkuint32_t version = NKI_VERSION;
        NKI_SERIALIZE_BASIC(nkuint32_t, version);
        if(version != NKI_VERSION) {
            nkiAddError(vm, -1, "Wrong binary VM version.");
            return nkfalse;
        }
    }

    NKI_WRAPSERIALIZE(
        nkiSerializeInstructions(vm, writer, userdata, writeMode));

    NKI_WRAPSERIALIZE(
        nkiSerializeErrorState(vm, writer, userdata, writeMode));

    NKI_WRAPSERIALIZE(
        nkiSerializeStatics(vm, writer, userdata, writeMode));

    NKI_WRAPSERIALIZE(
        nkiSerializeStack(vm, writer, userdata, writeMode));

    // Instruction pointer.
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->instructionPointer);

    NKI_WRAPSERIALIZE(
        nkiSerializeStringTable(vm, writer, userdata, writeMode));

    NKI_WRAPSERIALIZE(
        nkiSerializeGcState(vm, writer, userdata, writeMode));

    NKI_WRAPSERIALIZE(
        nkiSerializeFunctionTable(vm, writer, userdata, writeMode));

    // Serialize object table and objects. This MUST happen after the
    // functions, because deserialization routines are set up there.
    NKI_WRAPSERIALIZE(
        nkiSerializeObjectTable(vm, writer, userdata, writeMode));

    NKI_WRAPSERIALIZE(
        nkiSerializeGlobalsList(vm, writer, userdata, writeMode));

    // Skip limits (defined before serialization in application, also
    // would nullify safety precautions if we just take the serialized
    // data's word for it).

    // Skip memory usage and all allocation tracking (generated by nki
    // malloc interface).

    // Skip malloc/free replacements (defined before serialization).

    // Skip catastrophicFailureJmpBuf.

    // Skip userdata (serialize it outside the VM and hook it up
    // before/after).

    NKI_WRAPSERIALIZE(
        nkiSerializeExternalTypes(vm, writer, userdata, writeMode));

    return nktrue;
}

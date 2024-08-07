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

    nkuint32_t len = nkiStrlen(str);
    if(len >= NK_UINT_MAX) {
        nkiAddError(vm, "String too long to serialize.");
        return nkfalse;
    }

    // Write length.
    NKI_SERIALIZE_BASIC(nkuint32_t, len);

    // Write actual string.
    NKI_SERIALIZE_DATA(str, len);

    return nktrue;
}

nkbool nkiSerializeString_loadInPlace(
    struct NKVM *vm,
    NKVMSerializationWriter writer,
    void *userdata,
    nkuint32_t maxBufferSize,
    char *outData)
{
    nkbool writeMode = nkfalse;
    nkuint32_t len = 0;

    // Read length.
    NKI_SERIALIZE_BASIC(nkuint32_t, len);
    if(len >= maxBufferSize) { // ">=" Because of null terminator.
        nkiAddError(vm, "String too long to deserialize.");
        return nkfalse;
    }

    // Read actual string.
    if(!writer(outData, len, userdata, writeMode)) {
        return nkfalse;
    }

    outData[len] = 0;

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
    if(len >= NK_UINT_MAX) { // ">=" Because of null terminator.
        nkiAddError(vm, "String too long to deserialize.");
        return nkfalse;
    }

    *outData = (char *)nkiMalloc(vm, len + 1);

    // Read actual string.
    if(!writer(*outData, len, userdata, writeMode)) {
        nkiFree(vm, *outData);
        return nkfalse;
    }

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
            char *errorText = NULL;
            struct NKError *newError = NULL;
            NKI_SERIALIZE_STRING(errorText);
            newError = (struct NKError *)nkiMalloc(
                vm, sizeof(struct NKError));
            newError->errorText = errorText;
            newError->next = NULL;
            *lastPtr = newError;
            lastPtr = &newError->next;
            vm->errorState.lastError = newError;
        }

    }

    return nktrue;
}

#define NKI_SERIALIZE_PUSHSERIALIZERSTATE()                             \
    NKVMSerializationWriter oldWriter = vm->serializationState.writer;  \
    void *oldUserdata = vm->serializationState.userdata;                \
    nkbool oldWriteMode = vm->serializationState.writeMode;             \
    vm->serializationState.writer = writer;                             \
    vm->serializationState.userdata = userdata;                         \
    vm->serializationState.writeMode = writeMode;

#define NKI_SERIALIZE_POPSERIALIZERSTATE()              \
    vm->serializationState.userdata = oldUserdata;      \
    vm->serializationState.writer = oldWriter;          \
    vm->serializationState.writeMode = oldWriteMode;

#define NKI_SERIALIZE_WRAPCALLBACK(x)           \
    do {                                        \
        NKI_SERIALIZE_PUSHSERIALIZERSTATE();    \
        x;                                      \
        NKI_SERIALIZE_POPSERIALIZERSTATE();     \
    } while(0)

nkbool nkiSerializeObject(
    struct NKVM *vm, struct NKVMObject *object,
    NKVMSerializationWriter writer, void *userdata,
    nkbool writeMode)
{
    // Note: objectTableIndex is set in calling function.

    NKI_SERIALIZE_BASIC(nkuint32_t, object->size);
    NKI_SERIALIZE_BASIC(nkuint32_t, object->externalHandleCount);

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

    NKI_SERIALIZE_BASIC(nkuint32_t, object->lastGCPass);
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
            nkiMemset(&key, 0, sizeof(key));
            NKI_SERIALIZE_BASIC(struct NKValue, key);

            value = nkiVmObjectFindOrAddEntry(vm, object, &key, nkfalse);

            if(value) {
                NKI_SERIALIZE_BASIC(struct NKValue, *value);
            }

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
    nkuint32_t capacity = vm->objectTable.capacity;

    // Ensure capacity is a power of two.
    NKI_SERIALIZE_BASIC(nkuint32_t, capacity);
    if(!nkiIsPow2(capacity)) {
        nkiAddError(vm, "Object table capacity is not a power of two.");
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
        vm->objectTable.objectTable = NULL;

        vm->objectTable.objectTable =
            (struct NKVMObject **)nkiMallocArray(
                vm, sizeof(struct NKVMObject *), capacity);
        nkiMemset(
            vm->objectTable.objectTable, 0,
            sizeof(struct NKVMObject *) * capacity);

        vm->objectTable.capacity = capacity;

        // Free the holes.
        while(vm->objectTable.tableHoles) {
            struct NKVMTableHole *hole = vm->objectTable.tableHoles;
            vm->objectTable.tableHoles = hole->next;
            nkiFree(vm, hole);
        }
    }

    NKI_SERIALIZE_BASIC(nkuint32_t, objectCount);
    if(objectCount > vm->objectTable.capacity) {
        nkiAddError(vm, "Object count exceeds object table capacity.");
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

            struct NKVMObject *object = NULL;
            nkuint32_t index = 0;

            NKI_SERIALIZE_BASIC(nkuint32_t, index);

            // Juggle allocated objects after the dumb serialization
            // wrapper that would return without deallocating.
            object = (struct NKVMObject *)nkiMalloc(
                vm, sizeof(struct NKVMObject));
            nkiVmObjectInit(object, index);

            // Thanks AFL! Holy crap I'm an idiot for letting this one
            // slide by.
            if(index >= vm->objectTable.capacity) {
                nkiAddError(vm, "Object index exceeds object table capacity.");
                nkiFree(vm, object);
                return nkfalse;
            }

            if(vm->objectTable.objectTable[index]) {
                nkiAddError(vm, "Tried to load two object into the same location.");
                nkiFree(vm, object);
                return nkfalse;
            }

            vm->objectTable.objectTable[index] = object;

            if(!nkiSerializeObject(
                    vm, object,
                    writer, userdata, writeMode))
            {
                // Note: We are NOT going to free the partially
                // constructed object here. After nkiSerializeObject
                // starts, we're going to consider it a valid object
                // as far as the VM is concerned and it will be
                // cleaned up in the normal VM destroy.
                return nkfalse;
            }
        }
    }

    // Recreate holes for read mode.
    if(!writeMode) {
        nkuint32_t i;
        for(i = 0; i < vm->objectTable.capacity; i++) {
            if(!vm->objectTable.objectTable[i]) {
                struct NKVMTableHole *hole =
                    (struct NKVMTableHole *)nkiMalloc(
                        vm, sizeof(struct NKVMTableHole));
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
            nkiAddError(vm, "String table too large.");
            return nkfalse;
        }

        vm->stringTable.stringTable =
            (struct NKVMString **)nkiMallocArray(
                vm, sizeof(struct NKVMString*), vm->stringTable.capacity);

        // Note: Memset takes a size_t here, which on 64-bit bit can
        // take higher values than our nkuint32_t type passed into
        // nkiMalloc, meaning that this command can take larger values
        // than it's possible for us to allocate, even with the same
        // equation!
        nkiMemset(vm->stringTable.stringTable, 0,
            vm->stringTable.capacity * sizeof(struct NKVMString*));
    }

    // Thanks AFL!
    if(!nkiIsPow2(vm->stringTable.capacity)) {
        nkiAddError(vm, "String table capacity is not a power of two.");
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
                    char *strTmp = vm->stringTable.stringTable[i]->str;
                    NKI_SERIALIZE_BASIC(nkuint32_t, i);
                    NKI_SERIALIZE_STRING(strTmp);
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
                    nkiAddError(vm, "String index exceeds string table capacity.");
                    return nkfalse;
                }

                NKI_SERIALIZE_STRING(tmpStr);
                {
                    nkuint32_t stringLen = nkiStrlen(tmpStr);
                    nkuint32_t structAndPaddingSize = sizeof(*vm->stringTable.stringTable[index]) + 1;
                    nkuint32_t size = stringLen + structAndPaddingSize;

                    if(stringLen >= NK_UINT_MAX - structAndPaddingSize) {
                        nkiFree(vm, tmpStr);
                        nkiAddError(vm, "A string is longer than the addressable space to load it into.");
                        return nkfalse;
                    }

                    // Thanks AFL! Bail out if the binary has two
                    // strings in the same slot.
                    if(vm->stringTable.stringTable[index]) {
                        nkiFree(vm, tmpStr);
                        nkiAddError(vm, "Two strings occupy the same slot in the string table.");
                        return nkfalse;
                    }

                    // Allocate new string entry.
                    vm->stringTable.stringTable[index] =
                        (struct NKVMString *)nkiMalloc(vm, size);

                    // Clear it out.
                    nkiMemset(vm->stringTable.stringTable[index], 0, size);

                    // Copy the string data in.
                    nkiMemcpy(vm->stringTable.stringTable[index]->str, tmpStr, nkiStrlen(tmpStr) + 1);

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
                        struct NKVMTableHole *newHole =
                            (struct NKVMTableHole *)nkiMalloc(
                                vm, sizeof(struct NKVMTableHole));
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
        nkiAddError(vm, "Instruction address mask is too large.");
        return nkfalse;
    }

    if(!nkiIsPow2(vm->instructionAddressMask + 1)) {
        nkiAddError(vm, "Instruction address mask is not a power of two minus one.");
        return nkfalse;
    }

    if(instructionLimitSearch > vm->instructionAddressMask) {
        nkiAddError(vm, "Too many instructions for given instruction address mask.");
        return nkfalse;
    }

    // Recreate the entire instruction space if we're reading.
    if(!writeMode) {
        nkiFree(vm, vm->instructions);

        // Thanks AFL!
        vm->instructions = (struct NKInstruction *)nkiMallocArray(
            vm,
            sizeof(struct NKInstruction),
            vm->instructionAddressMask + 1);

        nkiMemset(vm->instructions, 0, sizeof(struct NKInstruction) * (vm->instructionAddressMask + 1));
    }

    // Load/save the actual instructions themselves.
    {
        nkuint32_t i;
        for(i = 0; i <= instructionLimitSearch; i++) {
            NKI_SERIALIZE_BASIC(struct NKInstruction, vm->instructions[i]);
        }
    }

    return nktrue;
}

nkbool nkiSerializeStatics(struct NKVM *vm, NKVMSerializationWriter writer, void *userdata, nkbool writeMode)
{
    nkuint32_t staticAddressMask = vm->staticAddressMask;

    // Check that a valid mask is 2^n-1. (Or just find a better way to
    // store it.)
    NKI_SERIALIZE_BASIC(nkuint32_t, staticAddressMask);
    if(!nkiIsPow2(staticAddressMask + 1)) {
        nkiAddError(vm, "Static space address mask is not a power of two minus one.");
        return nkfalse;
    }

    if(staticAddressMask >= NK_UINT_MAX) {
        nkiAddError(vm, "Static space address mask is too large.");
        return nkfalse;
    }

    vm->staticAddressMask = staticAddressMask;

    // Make new static space for read mode.
    if(!writeMode) {
        nkiFree(vm, vm->staticSpace);
        // Set to NULL first in case nkiMallocArray throws an error.
        vm->staticSpace = NULL;
        vm->staticSpace = (struct NKValue *)nkiMallocArray(
            vm, sizeof(struct NKValue), vm->staticAddressMask + 1);
    }

    // Read/write the static space.
    NKI_SERIALIZE_DATA(
        vm->staticSpace,
        sizeof(struct NKValue) * (vm->staticAddressMask + 1));

    return nktrue;
}

nkbool nkiSerializeStack(
    struct NKVM *vm,
    struct NKVMStack *stack,
    NKVMSerializationWriter writer,
    void *userdata,
    nkbool writeMode)
{
    nkuint32_t stackSize = stack->size;
    nkuint32_t stackCapacity = stack->capacity;

    NKI_SERIALIZE_BASIC(nkuint32_t, stackSize);
    NKI_SERIALIZE_BASIC(nkuint32_t, stackCapacity);
    if(!nkiIsPow2(stackCapacity)) {
        nkiAddError(vm, "Stack capacity is not a power of two.");
        return nkfalse;
    }

    // Check stack capacity limit.
    if(stackCapacity > vm->limits.maxStackSize) {
        nkiAddError(vm, "Stack capacity is too large for limit.");
        return nkfalse;
    }

    // Thanks AFL! Stack size = 0 means stack index mask becomes
    // 0xffffffff, even though there's no stack.
    if(stackCapacity < 1) {
        nkiAddError(vm, "Stack capacity is zero.");
        return nkfalse;
    }

    stack->indexMask = stack->capacity - 1;

    // Make new stack space for read mode.
    if(!writeMode) {
        //     nkiFree(vm, stack->values);
        //     stack->values = NULL;

        //     stack->values = nkiMallocArray(vm,
        //         sizeof(struct NKValue), stack->capacity);

        nkiVmStackClear(vm, stack, nktrue);
        stack->values = (struct NKValue *)nkiReallocArray(
            vm, stack->values, sizeof(struct NKValue),
            stackCapacity);

        stack->capacity = stackCapacity;
        stack->indexMask = stackCapacity - 1;
        stack->size = stackSize;

        // Thanks AFL!
        if(!stack->values) {
            return nkfalse;
        }

        nkiMemset(stack->values, 0,
            sizeof(struct NKValue) * (stack->capacity));
    }

    // Thanks AFL!
    if(stack->capacity < stack->size) {
        return nkfalse;
    }

    NKI_SERIALIZE_DATA(
        stack->values,
        sizeof(struct NKValue) * (stack->size));

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

// This is the inner part of the function table serialization. If this
// fails we can still clean up functionIdMapping in the outer function.
nkbool nkiSerializeFunctionTable_inner(
    struct NKVM *vm, NKVMSerializationWriter writer,
    void *userdata, nkbool writeMode,
    NKVMExternalFunctionID *functionIdMapping,
    nkuint32_t tmpExternalFunctionCount)
{
    nkuint32_t i;

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

                if(!nkiStrcmp(tmpExternalFunc.name, vm->externalFunctionTable[n].name)) {
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
    {
        nkuint32_t tmpFunctionCount = vm->functionCount;
        NKI_SERIALIZE_BASIC(nkuint32_t, tmpFunctionCount);

        // Reallocate internal function table for read mode.
        if(!writeMode) {

            struct NKVMFunction *newTable =
                (struct NKVMFunction *)nkiMallocArray(
                    vm, sizeof(struct NKVMFunction),
                    tmpFunctionCount);

            if(!newTable) {
                return nkfalse;
            }

            nkiFree(vm, vm->functionTable);
            vm->functionTable = newTable;
            vm->functionCount = tmpFunctionCount;
            nkiMemset(vm->functionTable, 0, sizeof(struct NKVMFunction) * vm->functionCount);
        }
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
                    nkiAddError(vm, "External function ID is outside external function count.");
                    return nkfalse;
                }

                if(vm->functionTable[i].externalFunctionId.id == NK_INVALID_VALUE) {
                    nkiAddError(vm, "Cannot find new mapping for external function during deserialization.");
                    return nkfalse;
                }
            }
        }
    }

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
    nkbool ret;
    nkuint32_t tmpExternalFunctionCount = vm->externalFunctionCount;

    NKI_SERIALIZE_BASIC(nkuint32_t, tmpExternalFunctionCount);

    // Allocate space for a temporary table where we'll keep track of
    // the mapping of the external function IDs in the serialized data
    // to the ones we already have already in the VM.
    if(!writeMode) {
        functionIdMapping = (NKVMExternalFunctionID *)nkiMallocArray(
            vm, sizeof(NKVMInternalFunctionID), tmpExternalFunctionCount);
        nkiMemset(
            functionIdMapping, NK_INVALID_VALUE,
            tmpExternalFunctionCount * sizeof(NKVMInternalFunctionID));
    }

    ret = nkiSerializeFunctionTable_inner(
        vm, writer, userdata, writeMode,
        functionIdMapping,
        tmpExternalFunctionCount);

    if(!writeMode) {
        nkiFree(vm, functionIdMapping);
    }

    return ret;
}

nkbool nkiSerializeGlobalsList(
    struct NKVM *vm, NKVMSerializationWriter writer,
    void *userdata, nkbool writeMode)
{
    nkuint32_t i;
    nkuint32_t globalVariableCount = vm->globalVariableCount;

    // If we're loading a file, nuke whatever global variable list
    // might be in the VM already.
    if(!writeMode) {
        for(i = 0; i < vm->globalVariableCount; i++) {
            nkiFree(vm, vm->globalVariables[i].name);
        }
        nkiFree(vm, vm->globalVariables);
        vm->globalVariables = NULL;
    }

    NKI_SERIALIZE_BASIC(nkuint32_t, globalVariableCount);

    // Realloc if we're reading in.
    if(!writeMode) {
        vm->globalVariables = (struct NKGlobalVariableRecord *)nkiMallocArray(
            vm, sizeof(vm->globalVariables[0]),
            globalVariableCount);

        // We don't need to check for overflow here, because
        // nkiMallocArray would have longjmp'd away by now if there
        // was one.
        nkiMemset(vm->globalVariables, 0,
            sizeof(vm->globalVariables[0]) * globalVariableCount);

        vm->globalVariableCount = globalVariableCount;
    }

    // Load/save each global name and index.
    for(i = 0; i < vm->globalVariableCount; i++) {
        NKI_SERIALIZE_STRING(vm->globalVariables[i].name);
        NKI_SERIALIZE_BASIC(nkuint32_t, vm->globalVariables[i].staticPosition);
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

    nkuint32_t longestTypeNameLength = 0;
    char *rawTypeMappingBuf = NULL;
    char *nameTempBuf = NULL;

    if(!writeMode) {
        for(i = 0; i < vm->externalTypeCount; i++) {

            // The serializer can't really handle >32-bit lengths anyway,
            // but just be aware that we might be truncating something.
            // Obviously that's a degenerate case for names, but we still
            // need to protect against bad data.

            if(vm->externalTypes && vm->externalTypes[i].name) {
                nkuint32_t nameLen = nkiStrlen(vm->externalTypes[i].name);
                if(nameLen > longestTypeNameLength) {
                    longestTypeNameLength = nameLen;
                }
            }
        }
    }

    NKI_SERIALIZE_BASIC(nkuint32_t, serializedTypeCount);

    if(serializedTypeCount != vm->externalTypeCount) {
        nkiAddError(vm, "External type count in binary does not match VM.");
        return nkfalse;
    }

    // Allocate an array if we have to map types between the binary
    // and existing types. Also make room (in this single allocation)
    // for the longest type name we could possibly use.
    if(!writeMode) {

        nkuint32_t typeMappingAllocationSize = serializedTypeCount * sizeof(nkuint32_t);
        nkuint32_t fullTempBufferAllocationSize = typeMappingAllocationSize + longestTypeNameLength + 1;

        // Check for overflow in the typeMappingAllocationSize
        // calculation.
        if(serializedTypeCount >= ~(nkuint32_t)0 / sizeof(nkuint32_t)) {
            return nkfalse;
        }

        // Check for overflow in fullTempBufferAllocationSize.
        if(longestTypeNameLength + 1 >= ~(nkuint32_t)0 - typeMappingAllocationSize) {
            return nkfalse;
        }

        if(longestTypeNameLength + 1 == 0) {
            return nkfalse;
        }

        rawTypeMappingBuf = (char *)nkiMalloc(vm, fullTempBufferAllocationSize);
        typeMapping = (nkuint32_t*)rawTypeMappingBuf;
        nameTempBuf = rawTypeMappingBuf + typeMappingAllocationSize;
    }

    for(i = 0; i < serializedTypeCount; i++) {

        char *typeName = nameTempBuf;
        if(writeMode) {
            if(vm->externalTypes && vm->externalTypes[i].name) {
                typeName = vm->externalTypes[i].name;
            } else {
                nkiFree(vm, rawTypeMappingBuf);
                return nkfalse;
            }
        }

        if(writeMode) {

            if(!nkiSerializeString_save(vm, writer, userdata, typeName)) {
                nkiFree(vm, rawTypeMappingBuf);
                return nkfalse;
            }

        } else {

            if(!nkiSerializeString_loadInPlace(
                    vm, writer, userdata,
                    longestTypeNameLength + 1,
                    typeName))
            {
                nkiFree(vm, rawTypeMappingBuf);
                return nkfalse;
            }

            // FIXME: Add type remapping back in. Possibly just by
            // deleting this block of code...

            // FIXME: Or don't add type remapping back in, because
            // some external structures may have held onto values
            // representing those types, and we can't update those.

            // No more type remapping. Verify that the loaded type
            // matches what we have already in the VM.
            if(!vm->externalTypes ||
                !vm->externalTypes[i].name ||
                nkiStrcmp(typeName, vm->externalTypes[i].name))
            {
                nkiAddError(vm, "Type name mismatch inside binary.");
                nkiFree(vm, rawTypeMappingBuf);
                return nkfalse;
            }
        }

        if(!writeMode) {

            nkuint32_t n;

            // Search for an existing type with the same name as the
            // one we're loading.
            for(n = 0; n < vm->externalTypeCount; n++) {

                if(vm->externalTypes &&
                    vm->externalTypes[n].name &&
                    !nkiStrcmp(vm->externalTypes[n].name, typeName))
                {
                    typeMapping[i] = n;

                    // No more type remapping.
                    assert(i == n);

                    break;
                }
            }

            if(n == vm->externalTypeCount) {
                nkiAddError(vm, "Could not find a matching type for deserialized external data type.");
                nkiFree(vm, rawTypeMappingBuf);
                return nkfalse;
            }
        }
    }

    // If we're loading, go through the entire (JUST LOADED) object
    // table and reassign every external type ID based on the mapping
    // we figured out.
    if(!writeMode) {

        for(i = 0; i < vm->objectTable.capacity; i++) {
            if(vm->objectTable.objectTable[i]) {
                if(vm->objectTable.objectTable[i]->externalDataType.id != NK_INVALID_VALUE) {
                    if(vm->objectTable.objectTable[i]->externalDataType.id < serializedTypeCount) {
                        vm->objectTable.objectTable[i]->externalDataType.id =
                            typeMapping[vm->objectTable.objectTable[i]->externalDataType.id];
                    } else {
                        nkiFree(vm, rawTypeMappingBuf);
                        return nkfalse;
                    }
                }
            }
        }

        nkiFree(vm, rawTypeMappingBuf);
    }

    return nktrue;
}

nkbool nkiSerializeExternalSubsystemData(
    struct NKVM *vm, NKVMSerializationWriter writer,
    void *userdata, nkbool writeMode)
{
    nkuint32_t externalSubsystemDataCount = 0;
    nkuint32_t n;

    // First count up the existing subsystem data.
    for(n = 0; n < nkiVmExternalSubsystemHashTableSize; n++) {
        struct NKVMExternalSubsystemData *data = vm->subsystemDataTable[n];
        while(data) {
            externalSubsystemDataCount++;
            data = data->nextInHashTable;
        }
    }

    // Save/load the subsystem data count.
    NKI_SERIALIZE_BASIC(nkuint32_t, externalSubsystemDataCount);

    if(writeMode) {

        for(n = 0; n < nkiVmExternalSubsystemHashTableSize; n++) {
            struct NKVMExternalSubsystemData *data = vm->subsystemDataTable[n];

            while(data) {

                NKI_SERIALIZE_STRING(data->name);

                if(data->serializationCallback) {
                    NKI_SERIALIZE_WRAPCALLBACK(
                        data->serializationCallback(vm, data->data));
                }

                data = data->nextInHashTable;
            }
        }

    } else {

        for(n = 0; n < externalSubsystemDataCount; n++) {
            char *name = NULL;
            struct NKVMExternalSubsystemData *data = NULL;

            NKI_SERIALIZE_STRING(name);
            data = nkiFindExternalSubsystemData(vm, name, nkfalse);
            nkiFree(vm, name);

            if(!data) {
                nkiAddError(vm, "External subsystem for serialized external subsystem data cannot be found.");
                return nkfalse;
            }

            if(data->serializationCallback) {
                NKI_SERIALIZE_WRAPCALLBACK(
                    data->serializationCallback(vm, data->data));
            }

        }
    }

    return nktrue;
}

nkbool nkiSerializeExternalObjects(
    struct NKVM *vm, NKVMSerializationWriter writer,
    void *userdata, nkbool writeMode)
{
    nkuint32_t i;

    for(i = 0; i < vm->objectTable.capacity; i++) {

        struct NKVMObject *object = vm->objectTable.objectTable[i];

        // Run any external data serialization routines.
        if(object) {
            if(object->externalDataType.id != NK_INVALID_VALUE) {
                if(object->externalDataType.id < vm->externalTypeCount) {

                    NKVMExternalObjectSerializationCallback serializationCallback =
                        vm->externalTypes[object->externalDataType.id].serializationCallback;

                    if(serializationCallback) {

                        struct NKValue val;
                        nkiMemset(&val, 0, sizeof(val));
                        val.type = NK_VALUETYPE_OBJECTID;
                        val.objectId = i;

                        NKI_SERIALIZE_WRAPCALLBACK(
                            serializationCallback(vm, &val, object->externalData));
                    }

                } else {
                    nkiAddError(vm, "External type value out of range.");
                }
            }
        }
    }

    return nktrue;
}

nkbool nkiSerializeSourceFileList(
    struct NKVM *vm, NKVMSerializationWriter writer,
    void *userdata, nkbool writeMode)
{
    nkuint32_t i;
    nkuint32_t sourceFileCount = vm->sourceFileCount;
    char **sourceFileList = vm->sourceFileList;

    NKI_SERIALIZE_BASIC(nkuint32_t, sourceFileCount);

    if(!writeMode) {

        // Tear down old list.
        nkiVmClearSourceFileList(vm);

        // Allocate new empty list to fill.
        sourceFileList = (char**)nkiMallocArray(vm, sizeof(char*), sourceFileCount);
        for(i = 0; i < sourceFileCount; i++) {
            sourceFileList[i] = NULL;
        }

        vm->sourceFileList = sourceFileList;
        vm->sourceFileCount = sourceFileCount;
    }

    // Fill the list or write it out.
    for(i = 0; i < sourceFileCount; i++) {
        NKI_SERIALIZE_STRING(sourceFileList[i]);
    }

    return nktrue;
}

nkbool nkiSerializePositionMarkerList(
    struct NKVM *vm, NKVMSerializationWriter writer,
    void *userdata, nkbool writeMode)
{
    nkuint32_t i;
    nkuint32_t positionMarkerCount = vm->positionMarkerCount;
    struct NKVMFilePositionMarker *positionMarkerList = vm->positionMarkerList;

    NKI_SERIALIZE_BASIC(nkuint32_t, positionMarkerCount);

    if(!writeMode) {

        // Tear down old list.
        nkiFree(vm, vm->positionMarkerList);
        vm->positionMarkerList = NULL;
        vm->positionMarkerCount = 0;

        // Allocate new empty list to fill.
        positionMarkerList = (struct NKVMFilePositionMarker*)nkiMallocArray(
            vm, sizeof(struct NKVMFilePositionMarker),
            positionMarkerCount);

        for(i = 0; i < positionMarkerCount; i++) {
            positionMarkerList[i].fileIndex = NK_INVALID_VALUE;
            positionMarkerList[i].lineNumber = 0;
            positionMarkerList[i].instructionIndex = NK_INVALID_VALUE;
        }

        vm->positionMarkerList = positionMarkerList;
        vm->positionMarkerCount = positionMarkerCount;
    }

    // Fill the list or write it out.
    for(i = 0; i < positionMarkerCount; i++) {
        NKI_SERIALIZE_BASIC(
            struct NKVMFilePositionMarker,
            positionMarkerList[i]);
    }

    return nktrue;
}

nkbool nkiSerializeActiveCoroutines(
    struct NKVM *vm, NKVMSerializationWriter writer,
    void *userdata, nkbool writeMode)
{
    if(writeMode) {

        struct NKVMExecutionContext *context = vm->currentExecutionContext;

        while(context) {
            NKI_SERIALIZE_BASIC(struct NKValue, context->coroutineObject);
            context = context->parent;
        }

    } else {

        // Load all object IDs and link them.

        struct NKValue inValue;
        struct NKVMExecutionContext *currentContext = NULL;
        struct NKVMExecutionContext *chainEnd = NULL;

        do {

            struct NKVMExecutionContext *executionContext = NULL;
            struct NKVMObject *object = NULL;

            NKI_SERIALIZE_BASIC(struct NKValue, inValue);

            object = nkiVmGetObjectFromValue(vm, &inValue);

            // Check to see if it's an object ID or nil. If it's an
            // object, look it up and ensure the type matches. If it
            // does, add it to the chain. If it's nil, then link to
            // root context. Otherwise, throw an error.

            if(inValue.type == NK_VALUETYPE_NIL) {
                executionContext = &vm->rootExecutionContext;
            } else if(inValue.type == NK_VALUETYPE_OBJECTID &&
                object &&
                object->externalDataType.id == vm->internalObjectTypes.coroutine.id)
            {
                executionContext = (struct NKVMExecutionContext*)object->externalData;
            } else {
                // Invalid coroutine object.
                nkiAddError(vm, "Invalid object in coroutine chain.");
                return nkfalse;
            }

            // The first context we find is the current context that's
            // executing in the VM.
            if(!currentContext) {
                currentContext = executionContext;
            }

            // All coroutines in the list of active coroutines must be
            // in the running state.
            if(executionContext->coroutineState != NK_COROUTINE_RUNNING) {
                nkiAddError(vm, "Dormant coroutine recorded in active coroutine chain.");
                executionContext->coroutineState = NK_COROUTINE_RUNNING;
            }

            // The chainEnd is the last one we've linked. We'll keep
            // linking more and replacing that pointer as we go.
            if(!chainEnd) {
                chainEnd = executionContext;
            } else {

                // Check parent to make sure we aren't making a loop.
                if(chainEnd->parent) {
                    nkiAddError(vm, "Corrupt coroutine chain.");
                    return nkfalse;
                }

                if(chainEnd == &vm->rootExecutionContext) {
                    nkiAddError(vm, "Corrupt coroutine chain.");
                    return nkfalse;
                }

                // Add this to the chain.
                chainEnd->parent = executionContext;
                chainEnd = executionContext;
            }

        } while(inValue.type != NK_VALUETYPE_NIL);

        // Set current context.
        vm->currentExecutionContext = currentContext;
    }

    return nktrue;
}

nkbool nkiSerializeExecutionContext(
    struct NKVM *vm,
    struct NKVMExecutionContext *context,
    nkbool serializeCoroutineData,
    NKVMSerializationWriter writer,
    void *userdata, nkbool writeMode)
{
    // Serialize instruction pointer.
    NKI_SERIALIZE_BASIC(
        nkuint32_t,
        context->instructionPointer);

    // Serialize stack.
    NKI_WRAPSERIALIZE(
        nkiSerializeStack(
            vm,
            &context->stack,
            writer,
            userdata,
            writeMode));

    // Coroutine data.
    if(serializeCoroutineData) {

        // Coroutine state.
        NKI_SERIALIZE_BASIC(
            nkuint32_t,
            context->coroutineState);

    }

    return nktrue;
}

// ABI-compatibility-breaking version change history:
// ----------------------------------------------------------------------
//   1-2 - ???
//   3   - Coroutines added
//   4   - '.' object call changed to '->'.
//   5   - Coroutine "is_finished" instruction added.

#define NKI_VERSION 5

nkbool nkiVmSerialize(struct NKVM *vm, NKVMSerializationWriter writer, void *userdata, nkbool writeMode)
{
    // Serialize format marker.
    {
        const char *formatMarker = "\0NKVM";
        char formatMarkerTmp[5];
        nkiMemcpy(formatMarkerTmp, formatMarker, 5);
        NKI_SERIALIZE_DATA(formatMarkerTmp, 5);
        if(nkiMemcmp(formatMarkerTmp, formatMarker, 5)) {
            nkiAddError(vm, "Wrong binary format.");
            return nkfalse;
        }
    }

    // Serialize version number.
    {
        nkuint32_t version = NKI_VERSION;
        NKI_SERIALIZE_BASIC(nkuint32_t, version);
        if(version != NKI_VERSION) {
            nkiAddError(vm, "Wrong binary VM version.");
            return nkfalse;
        }
    }

    NKI_WRAPSERIALIZE(
        nkiSerializeInstructions(vm, writer, userdata, writeMode));

    NKI_WRAPSERIALIZE(
        nkiSerializeErrorState(vm, writer, userdata, writeMode));

    NKI_WRAPSERIALIZE(
        nkiSerializeStatics(vm, writer, userdata, writeMode));

    // Root execution context.
    NKI_WRAPSERIALIZE(
        nkiSerializeExecutionContext(
            vm, &vm->rootExecutionContext, nkfalse,
            writer, userdata, writeMode));

    // String table.
    NKI_WRAPSERIALIZE(
        nkiSerializeStringTable(vm, writer, userdata, writeMode));

    // Garbage collector state. FIXME: Maybe we should just GC before
    // saving and after loading.
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

    // Serialized external subsystem data.
    NKI_WRAPSERIALIZE(
        nkiSerializeExternalSubsystemData(vm, writer, userdata, writeMode));

    // Individual external objects.
    NKI_WRAPSERIALIZE(
        nkiSerializeExternalObjects(vm, writer, userdata, writeMode));

    // Source file list.
    NKI_WRAPSERIALIZE(
        nkiSerializeSourceFileList(vm, writer, userdata, writeMode));

    // Serialize file/line markers.
    NKI_WRAPSERIALIZE(
        nkiSerializePositionMarkerList(vm, writer, userdata, writeMode));

    // Serialize active coroutines.
    NKI_WRAPSERIALIZE(
        nkiSerializeActiveCoroutines(vm, writer, userdata, writeMode));

    return nktrue;
}

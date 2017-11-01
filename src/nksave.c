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
    NKI_WRAPSERIALIZE(writer((void*)(data), (size), userdata, writeMode)); \
    printf(" ")

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
    if(lenFull > NK_UINT_MAX) return nkfalse;

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
    if(len == ~(nkuint32_t)0) {
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
    nkuint32_t errorCount = nkiVmGetErrorCount(vm);
    struct NKError *err = vm->errorState.firstError;

    printf("\nErrorState: ");

    // Save error count.
    NKI_SERIALIZE_BASIC(nkuint32_t, errorCount);

    // FIXME: Not set up to deserialize!
    if(!writeMode) {
        return nktrue;
    }

    // // Save each error.
    // while(err) {
    //     char *tmp = err->errorText;
    //     NKI_SERIALIZE_STRING(tmp);
    //     err = err->next;
    // }

    return nktrue;
}
nkbool nkiSerializeObject(
    struct NKVM *vm, struct NKVMObject *object,
    NKVMSerializationWriter writer, void *userdata,
    nkbool writeMode)
{
    printf("\n  Object: ");
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
            struct NKValue key = {0};
            struct NKValue *value;
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
            if(vm->objectTable.objectsWithExternalHandles) {
                vm->objectTable.objectsWithExternalHandles->previousExternalHandleListPtr =
                    &object->nextObjectWithExternalHandles;
            }
            object->previousExternalHandleListPtr = &vm->objectTable.objectsWithExternalHandles;
            object->nextObjectWithExternalHandles = vm->objectTable.objectsWithExternalHandles;
            vm->objectTable.objectsWithExternalHandles = object;
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

    printf("\nObjectTable: ");

    // FIXME: Ensure objectTableCapacity is a power of two, or find a
    // better way to store it!
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->objectTable.objectTableCapacity);
    if(!nkiIsPow2(vm->objectTable.objectTableCapacity)) {
        printf("!nkiIsPow2(vm->objectTable.objectTableCapacity)\n");
        return nkfalse;
    }

    if(writeMode) {

        // Count up objects if we're writing.
        nkuint32_t i;
        for(i = 0; i < vm->objectTable.objectTableCapacity; i++) {
            if(vm->objectTable.objectTable[i]) {
                objectCount++;
            }
        }

    } else {

        // Reallocate the object table if we're reading.
        nkiFree(vm, vm->objectTable.objectTable);
        vm->objectTable.objectTable =
            nkiMallocArray(vm, sizeof(struct NKVMObject *), vm->objectTable.objectTableCapacity);
        memset(
            vm->objectTable.objectTable, 0,
            sizeof(struct NKVMObject *) * vm->objectTable.objectTableCapacity);

        // Free the holes.
        while(vm->objectTable.tableHoles) {
            struct NKVMObjectTableHole *hole = vm->objectTable.tableHoles;
            vm->objectTable.tableHoles = hole->next;
            nkiFree(vm, hole);
        }
    }

    printf("\nObjectCount: ");

    NKI_SERIALIZE_BASIC(nkuint32_t, objectCount);
    if(objectCount > vm->objectTable.objectTableCapacity) {
        printf("\nBAD COUNT: %u >= %u\n", objectCount, vm->objectTable.objectTableCapacity);
        return nkfalse;
    }

    printf("\nObjects: ");

    if(writeMode) {

        nkuint32_t i;
        for(i = 0; i < vm->objectTable.objectTableCapacity; i++) {
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
            if(object->objectTableIndex >= vm->objectTable.objectTableCapacity) {
                printf("Object outside of table capacity.\n");
                return nkfalse;
            }

            vm->objectTable.objectTable[object->objectTableIndex] = object;
            NKI_WRAPSERIALIZE(
                nkiSerializeObject(
                    vm, object,
                    writer, userdata, writeMode));
        }
    }

    printf("\nRebuild holes: ");

    // Recreate holes for read mode.
    if(!writeMode) {
        nkuint32_t i;
        for(i = 0; i < vm->objectTable.objectTableCapacity; i++) {
            if(!vm->objectTable.objectTable[i]) {
                struct NKVMObjectTableHole *hole =
                    nkiMalloc(vm, sizeof(struct NKVMObjectTableHole));
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
    nkuint32_t capacity = vm->stringTable.stringTableCapacity;

    // FIXME: Remove this.
    nkiCheckStringTableHoles(vm);

    printf("\nStringTable: ");
    printf("\n  Capacity: ");
    NKI_SERIALIZE_BASIC(nkuint32_t, capacity);
    printf("\n  Decoded capacity: %u\n", capacity);

    // Destroy and recreate the string table with the specified
    // capacity, if we're in READ mode.
    if(!writeMode) {
        nkiVmStringTableDestroy(vm);
        vm->stringTable.stringTableCapacity = capacity;
        printf("\n  Assigned capacity: %u\n", vm->stringTable.stringTableCapacity);

        // Thanks AFL! String table size can cause a 32-bit integer
        // overflow after being multiplied by the size of the string
        // pointer. (FIXME: We might not need this after the addition
        // of nkiMallocArray.)
        if(~(nkuint32_t)0 / sizeof(struct NKVMString*) <= vm->stringTable.stringTableCapacity) {
            printf("String table too large!\n");
            return nkfalse;
        }

        vm->stringTable.stringTable =
            nkiMallocArray(vm, sizeof(struct NKVMString*), vm->stringTable.stringTableCapacity);

        // Note: Memset takes a size_t here, which on 64-bit bit can
        // take higher values than our nkuint32_t type passed into
        // nkiMalloc, meaning that this command can take larger values
        // than it's possible for us to allocate, even with the same
        // equation!
        memset(vm->stringTable.stringTable, 0,
            vm->stringTable.stringTableCapacity * sizeof(struct NKVMString*));
    }

    // Thanks AFL!
    if(!nkiIsPow2(vm->stringTable.stringTableCapacity)) {
        printf("Non-power-of-two string table capacity!\n");
        return nkfalse;
    }

    printf("\n");

    // FIXME: Remove this.
    nkiCheckStringTableHoles(vm);

    {
        nkuint32_t actualCount = 0;

        // Count up the number of actual entries.
        if(writeMode) {
            nkuint32_t i;
            for(i = 0; i < vm->stringTable.stringTableCapacity; i++) {
                if(vm->stringTable.stringTable[i]) {
                    actualCount++;
                }
            }
        }
        printf("\n  Count: ");
        NKI_SERIALIZE_BASIC(nkuint32_t, actualCount);
        printf("\n");

        if(writeMode) {

            nkuint32_t i;
            for(i = 0; i < vm->stringTable.stringTableCapacity; i++) {
                if(vm->stringTable.stringTable[i]) {
                    char *tmp = vm->stringTable.stringTable[i]->str;
                    printf("\n  Object: ");
                    NKI_SERIALIZE_BASIC(nkuint32_t, i);
                    printf("\n    String: ");
                    NKI_SERIALIZE_STRING(tmp);
                    printf("\n    lastGCPass: ");
                    NKI_SERIALIZE_BASIC(nkuint32_t, vm->stringTable.stringTable[i]->lastGCPass);
                    printf("\n    DontGC: ");
                    NKI_SERIALIZE_BASIC(nkbool, vm->stringTable.stringTable[i]->dontGC);
                }
            }

        } else {

            nkuint32_t n;

            // nkiVmStringTableDestroy(vm);
            // nkiVmStringTableInit(vm);

            // Delete all hole objects. We'll recreate them later.
            while(vm->stringTable.tableHoles) {
                struct NKVMStringTableHole *hole = vm->stringTable.tableHoles;
                vm->stringTable.tableHoles = hole->next;
                nkiFree(vm, hole);
            }

            for(n = 0; n < actualCount; n++) {

                nkuint32_t index = 0;
                char *tmpStr = NULL;

                printf("\n  Object: ");
                NKI_SERIALIZE_BASIC(nkuint32_t, index);
                if(index >= vm->stringTable.stringTableCapacity) {
                    printf("index >= vm->stringTable.stringTableCapacity\n");
                    return nkfalse;
                }

                printf("\n    String: ");
                NKI_SERIALIZE_STRING(tmpStr);
                {
                    // FIXME: Check overflow here.
                    nkuint32_t size =
                        strlen(tmpStr) + sizeof(*vm->stringTable.stringTable[index]) + 1;

                    // Thanks AFL! Bail out if the binary has two
                    // strings in the same slot.
                    if(vm->stringTable.stringTable[index]) {
                        printf("Two strings in the same slot!\n");
                        return nkfalse;
                    }

                    vm->stringTable.stringTable[index] =
                        nkiMalloc(vm, size);


                    assert(vm->stringTable.stringTable[index]);

                    // printf("size: %u\n", size);

                    memset(vm->stringTable.stringTable[index], 0, size);
                    memcpy(vm->stringTable.stringTable[index]->str, tmpStr, strlen(tmpStr) + 1);
                    nkiFree(vm, tmpStr);

                    // FIXME: Remove this.
                    assert(vm->stringTable.stringTable[index]);

                    printf("\n    lastGCPass: ");
                    NKI_SERIALIZE_BASIC(nkuint32_t, vm->stringTable.stringTable[index]->lastGCPass);

                    printf("\n    DontGC: ");
                    NKI_SERIALIZE_BASIC(nkbool, vm->stringTable.stringTable[index]->dontGC);
                    vm->stringTable.stringTable[index]->stringTableIndex = index;
                    vm->stringTable.stringTable[index]->hash =
                        nkiStringHash(vm->stringTable.stringTable[index]->str);

                    {
                        struct NKVMString *hashBucket =
                            vm->stringTable.stringsByHash[
                                vm->stringTable.stringTable[index]->hash & (nkiVmStringHashTableSize - 1)];
                        vm->stringTable.stringTable[index]->nextInHashBucket = hashBucket;
                        vm->stringTable.stringsByHash[
                            vm->stringTable.stringTable[index]->hash & (nkiVmStringHashTableSize - 1)] =
                            vm->stringTable.stringTable[index];
                    }
                }

            }

            // // Reverse the order of the linked lists in the hash table
            // // for consistency with what was saved.
            // for(n = 0; n < nkiVmStringHashTableSize; n++) {

            //     struct NKVMString **origList =
            //         &vm->stringTable.stringsByHash[n];

            //     struct NKVMString *tmp1 = NULL;
            //     struct NKVMString *newList = NULL;

            //     while(*origList) {
            //         tmp1 = *origList;
            //         *origList = tmp1->nextInHashBucket;
            //         tmp1->nextInHashBucket = newList;
            //         newList = tmp1;
            //     }

            //     *origList = newList;
            // }

            // FIXME: Remove this.
            nkiCheckStringTableHoles(vm);

            // Delete and recreate hole objects.
            while(vm->stringTable.tableHoles) {
                struct NKVMStringTableHole *hole = vm->stringTable.tableHoles;
                vm->stringTable.tableHoles = hole->next;
                nkiFree(vm, hole);
            }
            // FIXME: Remove this.
            nkiCheckStringTableHoles(vm);
            {
                nkuint32_t i;
                for(i = 0; i < vm->stringTable.stringTableCapacity; i++) {
                    if(!vm->stringTable.stringTable[i]) {
                        struct NKVMStringTableHole *newHole = nkiMalloc(vm, sizeof(struct NKVMStringTableHole));
                        newHole->index = i;
                        newHole->next = vm->stringTable.tableHoles;
                        vm->stringTable.tableHoles = newHole;

                        printf("ADSF: Made new hole: %d\n", i);
                    }
                }
            }

            // FIXME: Remove this.
            nkiCheckStringTableHoles(vm);

        }

    }

    // FIXME: Remove this.
    nkiCheckStringTableHoles(vm);

    printf("\nStringtable complete\n");

    return nktrue;
}

nkbool nkiSerializeInstructions(struct NKVM *vm, NKVMSerializationWriter writer, void *userdata, nkbool writeMode)
{
    printf("\nInstructions: ");
    {
        // Find the actual end of the instruction buffer.
        nkuint32_t instructionLimitSearch = vm->instructionAddressMask;
        if(writeMode) {
            while(instructionLimitSearch && vm->instructions[instructionLimitSearch].opcode == NK_OP_NOP) {
                instructionLimitSearch--;
            }
        }

        // Record how much information we're going to store in the
        // stream.
        NKI_SERIALIZE_BASIC(nkuint32_t, instructionLimitSearch);

        // We still need the real instruction address mask. There's a
        // chance that some of that NK_OP_OP on the end was really
        // something like a literal value instead of a real opcode,
        // and if we cut off the address mask before it, that would
        // read the wrong value.
        NKI_SERIALIZE_BASIC(nkuint32_t, vm->instructionAddressMask);

        // Thanks AFL! Instruction address mask of 2^32-1 means that
        // the actual allocation is 2^32, which overflows to zero, and
        // we're left in a situation where we think we have a giant
        // instruction buffer when we really have a NULL pointer.
        if(vm->instructionAddressMask == ~(nkuint32_t)0) {
            printf("instruction address mask is 2^32-1.\n");
            return nkfalse;
        }

        if(!nkiIsPow2(vm->instructionAddressMask + 1)) {
            printf("instruction address mask is not 2^n-1.\n");
            return nkfalse;
        }

        if(instructionLimitSearch > vm->instructionAddressMask) {
            printf("Too many instructions for given instruction address mask.\n");
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

        printf("Address mask:     %u\n", vm->instructionAddressMask);
        printf("ILS:              %u\n", instructionLimitSearch);
        printf("vm->instructions: %p\n", vm->instructions);

        printf("\n");
        {
            nkuint32_t i;
            for(i = 0; i <= instructionLimitSearch; i++) {
                printf("  ");
              // #if NK_VM_DEBUG
              //   NKI_SERIALIZE_DATA(&vm->instructions[i], 4);
              // #else
                NKI_SERIALIZE_BASIC(struct NKInstruction, vm->instructions[i]);
              // #endif
                printf("\n");
            }
        }
    }

    return nktrue;
}

#define NKI_VERSION 1

nkbool nkiVmSerialize(struct NKVM *vm, NKVMSerializationWriter writer, void *userdata, nkbool writeMode)
{
    // Clean up before serializing.

    // FIXME: Remove this.
    nkiCheckStringTableHoles(vm);

    printf("\nVM serialize:\n");

    // Serialize format marker.
    {
        const char *formatMarker = "\0NKVM";
        char formatMarkerTmp[5];
        memcpy(formatMarkerTmp, formatMarker, 5);
        NKI_SERIALIZE_DATA(formatMarkerTmp, 5);
        if(memcmp(formatMarkerTmp, formatMarker, 5)) {
            return nkfalse;
        }
    }

    // Serialize version number.
    {
        nkuint32_t version = NKI_VERSION;
        NKI_SERIALIZE_BASIC(nkuint32_t, version);
        if(version != NKI_VERSION) {
            printf("version != NKI_VERSION\n");
            return nkfalse;
        }
    }

    NKI_WRAPSERIALIZE(
        nkiSerializeInstructions(vm, writer, userdata, writeMode));

    NKI_WRAPSERIALIZE(
        nkiSerializeErrorState(vm, writer, userdata, writeMode));

    // FIXME: Check that a valid mask is 2^n-1. (Or just find a better
    // way to store it.)
    printf("\nStaticSpaceSize: ");
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->staticAddressMask);
    if(!nkiIsPow2(vm->staticAddressMask + 1)) {
        printf("!nkiIsPow2(vm->staticAddressMask + 1)\n");
        return nkfalse;
    }

    printf("\nStaticSpace: ");

    // Make new static space for read mode.
    if(!writeMode) {
        nkiFree(vm, vm->staticSpace);
        vm->staticSpace = nkiMallocArray(vm, sizeof(struct NKValue), vm->staticAddressMask + 1);
    }

    // Read/write the static space.
    NKI_SERIALIZE_DATA(
        vm->staticSpace,
        sizeof(struct NKValue) * (vm->staticAddressMask + 1));

    printf("\nStackSize: ");
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->stack.size);
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->stack.capacity);
    if(!nkiIsPow2(vm->stack.capacity)) {
        printf("!nkiIsPow2(vm->stack.capacity)\n");
        return nkfalse;
    }

    // Thanks AFL! Stack size = 0 means stack index mask becomes
    // 0xffffffff, even though there's no stack.
    if(vm->stack.capacity < 1) {
        printf("Zero stack size not allowed.\n");
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

    printf("\nIP: ");
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->instructionPointer);

    NKI_WRAPSERIALIZE(nkiSerializeStringTable(vm, writer, userdata, writeMode));

    // GC state. Exposing the garbage collector parameters to someone
    // writing a malicious binary is an issue if the memory usage
    // limits are not set, but right now I think we should go with the
    // most consistency between serialized and deserialized versions
    // of stuff.
    printf("GC stuff:\n");
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->lastGCPass);
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->gcInterval);

    // gcInterval needs a hard limit here because the instruction
    // count limit is only enforced on garbage collection intervals.
    if(vm->gcInterval > 65536) {
        vm->gcInterval = 65536;
    }

    NKI_SERIALIZE_BASIC(nkuint32_t, vm->gcCountdown);
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->gcNewObjectInterval);
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->gcNewObjectCountdown);
    printf("\nGC stuff end:\n");

    // Save the external function table or match up existing external
    // functions to loaded external functions at read time.
    {

        NKVMExternalFunctionID *functionIdMapping = NULL;
        nkuint32_t tmpExternalFunctionCount = vm->externalFunctionCount;

        printf("\nExternalFunctionTable: ");
        {
            NKI_SERIALIZE_BASIC(nkuint32_t, tmpExternalFunctionCount);

            printf("\n");
            {
                nkuint32_t i;
                if(!writeMode) {
                    functionIdMapping = nkiMallocArray(
                        vm, sizeof(NKVMInternalFunctionID), tmpExternalFunctionCount);
                    // printf("tmpExternalFunctionCount: %u\n", tmpExternalFunctionCount);
                    // assert(functionIdMapping);
                    memset(
                        functionIdMapping, NK_INVALID_VALUE,
                        tmpExternalFunctionCount * sizeof(NKVMInternalFunctionID));
                }

                for(i = 0; i < tmpExternalFunctionCount; i++) {

                    struct NKVMExternalFunction tmpExternalFunc = {0};
                    if(writeMode) {
                        tmpExternalFunc = vm->externalFunctionTable[i];
                    }

                    printf("  ");

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
                            if(!strcmp(tmpExternalFunc.name, vm->externalFunctionTable[n].name)) {
                                functionIdMapping[i].id = n;
                                vm->externalFunctionTable[n].internalFunctionId =
                                    tmpExternalFunc.internalFunctionId;
                                break;
                            }
                        }

                        nkiFree(vm, tmpExternalFunc.name);
                    }

                    printf("\n");

                }

            }
        }

        printf("\nFunctionTable: ");
        NKI_SERIALIZE_BASIC(nkuint32_t, vm->functionCount);
        printf("\n");
        {
            nkuint32_t i;

            // Reallocate internal function table for read mode.
            if(!writeMode) {
                nkiFree(vm, vm->functionTable);
                vm->functionTable = nkiMallocArray(vm, sizeof(struct NKVMFunction), vm->functionCount);
                memset(vm->functionTable, 0, sizeof(struct NKVMFunction) * vm->functionCount);
            }

            for(i = 0; i < vm->functionCount; i++) {
                printf("  ");
                NKI_SERIALIZE_BASIC(nkuint32_t, vm->functionTable[i].argumentCount);
                NKI_SERIALIZE_BASIC(nkuint32_t, vm->functionTable[i].firstInstructionIndex);
                NKI_SERIALIZE_BASIC(NKVMExternalFunctionID, vm->functionTable[i].externalFunctionId);

                // Remap external function ID.
                if(!writeMode) {
                    // printf("id:    %u\n", vm->functionTable[i].externalFunctionId.id);
                    // printf("count: %u\n", vm->externalFunctionCount);
                    if(vm->functionTable[i].externalFunctionId.id != NK_INVALID_VALUE) {

                        // Thanks AFL! There was a <= instead of a <
                        // here.
                        if(vm->functionTable[i].externalFunctionId.id < tmpExternalFunctionCount) {
                            vm->functionTable[i].externalFunctionId =
                                functionIdMapping[vm->functionTable[i].externalFunctionId.id];
                        } else {
                            printf("vm->functionTable[i].externalFunctionId.id > tmpExternalFunctionCount\n");
                            return nkfalse;
                        }
                    }
                }
                printf("\n");
            }
        }

        if(!writeMode) {
            nkiFree(vm, functionIdMapping);
        }
    }

    // Serialize object table and objects. This MUST happen after the
    // functions, because deserialization routines are set up there.
    printf("\nObject table start\n");
    NKI_WRAPSERIALIZE(nkiSerializeObjectTable(vm, writer, userdata, writeMode));
    printf("\nObject table end\n");

    // TODO: Deal with the fact that we stuck all the global variable
    // names in one big chunk of memory for some dumb reason.
    printf("\nGlobalVariables: ");
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->globalVariableCount);
    printf("\n");
    {
        nkuint32_t i;

        if(!writeMode) {
            nkiFree(vm, vm->globalVariables);
            nkiFree(vm, vm->globalVariableNameStorage);
            vm->globalVariables = nkiMallocArray(
                vm, sizeof(vm->globalVariables[0]),
                vm->globalVariableCount);
        }

        for(i = 0; i < vm->globalVariableCount; i++) {
            printf("  ");
            NKI_SERIALIZE_STRING(vm->globalVariables[i].name);
            NKI_SERIALIZE_BASIC(nkuint32_t, vm->globalVariables[i].staticPosition);
            printf("\n");
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
    }

    // Skip limits (defined before serialization in application, also
    // nullifies safety precautions if we just take the serialized
    // data's word for it).

    // Skip memory usage and all allocation tracking (generated by nki
    // malloc interface).

    // Skip malloc/free replacements (defined before serialization).

    // Skip catastrophicFailureJmpBuf.

    // Skip userdata (serialize it outside the VM and hook it up
    // before/after).

    // TODO: Match up internal/external types. Will this mean going
    // through and fixing up objects afterwards?
    printf("\nExternalTypeNames: ");
    {
        nkuint32_t serializedTypeCount = vm->externalTypeCount;
        NKI_SERIALIZE_BASIC(nkuint32_t, serializedTypeCount);

        printf("\n");

        {
            nkuint32_t i;

            nkuint32_t *typeMapping = NULL;
            if(!writeMode) {
                typeMapping = nkiMallocArray(vm, sizeof(nkuint32_t), serializedTypeCount);
            }

            for(i = 0; i < serializedTypeCount; i++) {
                char *typeName = NULL;
                if(writeMode) {
                    typeName = vm->externalTypeNames[i];
                }

                printf("  ");
                NKI_SERIALIZE_STRING(typeName);
                printf("\n");

                if(!writeMode) {

                    nkuint32_t n;

                    for(n = 0; n < vm->externalTypeCount; n++) {
                        if(!strcmp(vm->externalTypeNames[n], typeName)) {
                            typeMapping[i] = n;
                            break;
                        }
                    }

                    if(n == vm->externalTypeCount) {
                        // Couldn't find a matching type to the one we
                        // loaded.
                        return nkfalse;
                    }

                    nkiFree(vm, typeName);
                }
            }

            if(!writeMode) {

                nkuint32_t i;

                for(i = 0; i < vm->objectTable.objectTableCapacity; i++) {
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
        }
    }

    return nktrue;
}

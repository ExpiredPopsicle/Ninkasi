#include "nkcommon.h"

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

    // Save each error.
    while(err) {
        NKI_SERIALIZE_STRING(err->errorText);
        err = err->next;
    }

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
    NKI_SERIALIZE_BASIC(NKVMInternalFunctionID, object->gcCallback);
    NKI_SERIALIZE_BASIC(NKVMInternalFunctionID, object->serializationCallback);
    NKI_SERIALIZE_BASIC(NKVMExternalDataTypeID, object->externalDataType);

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
            nkiMalloc(vm, sizeof(struct NKVMObject *) * vm->objectTable.objectTableCapacity);
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

    NKI_SERIALIZE_BASIC(nkuint32_t, objectCount);
    if(objectCount >= vm->objectTable.objectTableCapacity) {
        return nkfalse;
    }

    if(writeMode) {

        nkuint32_t i;
        for(i = 0; i < vm->objectTable.objectTableCapacity; i++) {
            if(vm->objectTable.objectTable[i]) {
                NKI_WRAPSERIALIZE(
                    nkiSerializeObject(
                        vm, vm->objectTable.objectTable[i],
                        writer, userdata, writeMode));
            }
        }

    } else {

        nkuint32_t i;
        for(i = 0; i < objectCount; i++) {
            struct NKVMObject *object = nkiMalloc(vm, sizeof(struct NKVMObject));
            memset(object, 0, sizeof(struct NKVMObject));
            vm->objectTable.objectTable[i] = object;
            NKI_WRAPSERIALIZE(
                nkiSerializeObject(
                    vm, vm->objectTable.objectTable[i],
                    writer, userdata, writeMode));
        }
    }

    // Recreate holes for read mode.
    if(!writeMode) {
        nkuint32_t i;
        for(i = 0; i < vm->objectTable.objectTableCapacity; i++) {
            if(!vm->objectTable.objectTable[i]) {
                struct NKVMObjectTableHole *hole =
                    nkiMalloc(vm, sizeof(struct NKVMObjectTableHole));
                hole->next = vm->objectTable.tableHoles;
                hole->index = i;
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

    printf("\nStringTable: ");
    printf("\n  Capacity: ");
    NKI_SERIALIZE_BASIC(nkuint32_t, capacity);

    // Destroy and recreate the string table with the specified capacity, if we're in READ mode.
    if(!writeMode) {
        nkiVmStringTableDestroy(vm);
        vm->stringTable.stringTableCapacity = capacity;
        vm->stringTable.stringTable =
            nkiMalloc(vm, vm->stringTable.stringTableCapacity * sizeof(struct NKVMString*));
        memset(vm->stringTable.stringTable, 0,
            vm->stringTable.stringTableCapacity * sizeof(struct NKVMString*));
    }

    printf("\n");

    {
        nkuint32_t i;
        nkuint32_t actualCount = 0;

        // Count up the number of actual entries.
        if(writeMode) {
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

            for(i = 0; i < vm->stringTable.stringTableCapacity; i++) {
                if(vm->stringTable.stringTable[i]) {
                    printf("\n  Object: ");
                    char *tmp = vm->stringTable.stringTable[i]->str;
                    NKI_SERIALIZE_BASIC(nkuint32_t, i);
                    printf("\n    String: ");
                    NKI_SERIALIZE_STRING(tmp);
                    printf("\n    DontGC: ");
                    NKI_SERIALIZE_BASIC(nkbool, vm->stringTable.stringTable[i]->dontGC);
                }
            }

        } else {

            nkuint32_t n;

            for(n = 0; n < actualCount; n++) {

                nkuint32_t i = 0;
                char *tmpStr = NULL;

                printf("\n  Object: ");
                NKI_SERIALIZE_BASIC(nkuint32_t, i);
                if(i >= vm->stringTable.stringTableCapacity) {
                    return nkfalse;
                }

                printf("\n    String: ");
                NKI_SERIALIZE_STRING(tmpStr);
                {
                    // FIXME: Check overflow here.
                    nkuint32_t size =
                        strlen(tmpStr) + sizeof(*vm->stringTable.stringTable[i]) + 1;
                    vm->stringTable.stringTable[i] =
                        nkiMalloc(vm, size);


                    assert(vm->stringTable.stringTable[i]);

                    printf("size: %u\n", size);

                    memset(vm->stringTable.stringTable[i], 0, size);
                    memcpy(vm->stringTable.stringTable[i]->str, tmpStr, strlen(tmpStr) + 1);
                    nkiFree(vm, tmpStr);

                    assert(vm->stringTable.stringTable[i]);

                    printf("\n    DontGC: ");
                    NKI_SERIALIZE_BASIC(nkbool, vm->stringTable.stringTable[i]->dontGC);
                    vm->stringTable.stringTable[i]->stringTableIndex = i;
                    vm->stringTable.stringTable[i]->hash =
                        nkiStringHash(vm->stringTable.stringTable[i]->str);

                    {
                        struct NKVMString *hashBucket =
                            vm->stringTable.stringsByHash[
                                vm->stringTable.stringTable[i]->hash & (nkiVmStringHashTableSize - 1)];
                        vm->stringTable.stringTable[i]->nextInHashBucket = hashBucket;
                        vm->stringTable.stringsByHash[
                            vm->stringTable.stringTable[i]->hash & (nkiVmStringHashTableSize - 1)] =
                            vm->stringTable.stringTable[i];
                    }
                }

            }

            // Create hole objects.
            for(i = 0; i < vm->stringTable.stringTableCapacity; i++) {
                if(!vm->stringTable.stringTable[i]) {
                    struct NKVMStringTableHole *newHole = nkiMalloc(vm, sizeof(struct NKVMStringTableHole));
                    newHole->index = i;
                    newHole->next = vm->stringTable.tableHoles;
                    vm->stringTable.tableHoles = newHole;
                }
            }


        }

    }

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

        // Recreate the entire instruction space if we're reading.
        if(!writeMode) {
            nkuint32_t bufSize = sizeof(struct NKInstruction) * (vm->instructionAddressMask + 1);
            nkiFree(vm, vm->instructions);
            vm->instructions = nkiMalloc(
                vm,
                bufSize);
            memset(vm->instructions, 0, bufSize);
        }

        printf("\n");
        {
            nkuint32_t i;
            for(i = 0; i < instructionLimitSearch; i++) {
                printf("  ");
              #if NK_VM_DEBUG
                NKI_SERIALIZE_DATA(&vm->instructions[i], 4);
              #else
                NKI_SERIALIZE_BASIC(struct NKInstruction, vm->instructions[i]);
              #endif
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

    printf("\nVM serialize: ");

    // Serialize version number.
    {
        nkuint32_t version = NKI_VERSION;
        NKI_SERIALIZE_BASIC(nkuint32_t, version);
        if(version != NKI_VERSION) {
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

    printf("\nStaticSpace: ");

    // Make new static space for read mode.
    if(!writeMode) {
        nkiFree(vm, vm->staticSpace);
        vm->staticSpace = nkiMalloc(vm, sizeof(struct NKValue) * (vm->staticAddressMask + 1));
    }

    // Read/write the static space.
    NKI_SERIALIZE_DATA(
        vm->staticSpace,
        sizeof(struct NKValue) * (vm->staticAddressMask + 1));

    printf("\nStackSize: ");
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->stack.size);

    // Make new stack space for read mode.
    if(!writeMode) {
        nkiFree(vm, vm->stack.values);
        vm->stack.values = nkiMalloc(vm,
            sizeof(struct NKValue) * (vm->stack.size));
        memset(vm->stack.values, 0,
            sizeof(struct NKValue) * (vm->stack.size));
    }

    NKI_SERIALIZE_DATA(
        vm->stack.values,
        sizeof(struct NKValue) * (vm->stack.size));

    printf("\nIP: ");
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->instructionPointer);

    NKI_WRAPSERIALIZE(nkiSerializeStringTable(vm, writer, userdata, writeMode));

    NKI_WRAPSERIALIZE(nkiSerializeObjectTable(vm, writer, userdata, writeMode));

    // Skip GC state (serialized data doesn't get to decide anything
    // about the GC).

    // Save the external function table or match up existing external
    // functions to loaded external functions at read time.
    printf("\nExternalFunctionTable: ");
    {
        nkuint32_t tmpExternalFunctionCount = vm->externalFunctionCount;
        NKI_SERIALIZE_BASIC(nkuint32_t, tmpExternalFunctionCount);

        printf("\n");
        {
            nkuint32_t i;
            NKVMExternalFunctionID *functionIdMapping = NULL;
            if(!writeMode) {
                functionIdMapping = nkiMalloc(
                    vm, tmpExternalFunctionCount * sizeof(NKVMInternalFunctionID));
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
                }

                printf("\n");

            }

            if(!writeMode) {
                nkiFree(vm, functionIdMapping);
            }
        }
    }

    // TODO: Reallocate internal function table for read mode.
    printf("\nFunctionTable: ");
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->functionCount);
    printf("\n");
    {
        nkuint32_t i;
        for(i = 0; i < vm->functionCount; i++) {
            printf("  ");
            NKI_SERIALIZE_BASIC(nkuint32_t, vm->functionTable[i].argumentCount);
            NKI_SERIALIZE_BASIC(nkuint32_t, vm->functionTable[i].firstInstructionIndex);
            NKI_SERIALIZE_BASIC(NKVMExternalFunctionID, vm->functionTable[i].externalFunctionId);
            printf("\n");
        }
    }

    // TODO: Deal with the fact that we stuck all the global variable
    // names in one big chunk of memory for some dumb reason.
    printf("\nGlobalVariables: ");
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->globalVariableCount);
    printf("\n");
    {
        nkuint32_t i;
        for(i = 0; i < vm->globalVariableCount; i++) {
            printf("  ");
            NKI_SERIALIZE_STRING(vm->globalVariables[i].name);
            NKI_SERIALIZE_BASIC(nkuint32_t, vm->globalVariables[i].staticPosition);
            printf("\n");
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
    NKI_SERIALIZE_BASIC(nkuint32_t, vm->externalTypeCount);
    printf("\n");
    {
        nkuint32_t i;
        for(i = 0; i < vm->externalTypeCount; i++) {
            printf("  ");
            NKI_SERIALIZE_STRING(vm->externalTypeNames[i]);
            printf("\n");
        }
    }

    return nktrue;
}

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

#define NK_DEBUG_SPAM 0

// FIXME: We don't really need this anymore.
int nkiDbgWriteLine(const char *fmt, ...)
{
  #if NK_DEBUG_SPAM
    va_list args;
    int ret;
    va_start(args, fmt);
    {
        nkint32_t i;
        for(i = 0; i < nkiDbgIndentLevel; i++) {
            printf("  ");
        }
        printf("\033[2m");
        ret = vprintf(fmt, args);
        printf("\033[0m");
        printf("\n");
    }
    va_end(args);
    return ret;
  #else
    return 0;
  #endif
}

// ----------------------------------------------------------------------
// State dump stuff.

void nkiDbgAppendEscaped(nkuint32_t bufSize, char *dst, const char *src)
{
    nkuint32_t i = nkiStrlen(dst);
    while(i + 2 < bufSize && *src) {
        switch(*src) {
            case '\n':
                dst[i++] = '\\';
                dst[i++] = 'n';
                break;
            case '\t':
                dst[i++] = '\\';
                dst[i++] = 't';
                break;
            case '\"':
                dst[i++] = '\\';
                dst[i++] = '\"';
                break;
            default:
                dst[i++] = *src;
                break;
        }
        src++;
    }
    dst[i] = 0;
}

void nkiDbgDumpRaw(FILE *stream, void *data, nkuint32_t len)
{
    nkuint32_t i;
    const char *lookup = "0123456789abcdef";
    const char *ptr = (const char *)data;
    for(i = 0; i < len; i++) {
        fputc(lookup[(ptr[i] & 0xf0) >> 4], stream);
        fputc(lookup[(ptr[i] & 0xf)], stream);
    }
}

void nkiDbgDumpState(struct NKVM *vm, FILE *stream)
{
    nkuint32_t i;

    fprintf(stream, "Errors:\n");
    {
        struct NKError *err = vm->errorState.firstError;
        while(err) {
            fprintf(stream, "  %s\n", err->errorText);
            err = err->next;
        }
    }

    fprintf(stream, "Stack:\n");
    fprintf(stream, "  Capacity:  " NK_PRINTF_UINT32 "\n", vm->currentExecutionContext->stack.capacity);
    fprintf(stream, "  Size:      " NK_PRINTF_UINT32 "\n", vm->currentExecutionContext->stack.size);
    fprintf(stream, "  IndexMask: " NK_PRINTF_UINT32 "\n", vm->currentExecutionContext->stack.indexMask);
    fprintf(stream, "  Elements:\n");
    for(i = 0; i < vm->currentExecutionContext->stack.size; i++) {
        fprintf(stream, "    " NK_PRINTF_UINT32 ": ", i);
        nkiValueDump(vm, &vm->currentExecutionContext->stack.values[i], stream);
        fprintf(stream, "\n");
    }

    fprintf(stream, "Statics:\n");
    fprintf(stream, "  staticAddressMask: " NK_PRINTF_UINT32 "\n", vm->staticAddressMask);
    fprintf(stream, "  Elements:\n");
    for(i = 0; i <= vm->staticAddressMask; i++) {
        fprintf(stream, "    " NK_PRINTF_UINT32 ": ", i);
        nkiValueDump(vm, &vm->staticSpace[i], stream);
        fprintf(stream, "\n");
    }

    fprintf(stream, "Instructions:\n");
    fprintf(stream, "  instructionAddressMask: " NK_PRINTF_UINT32 "\n", vm->instructionAddressMask);
    fprintf(stream, "  instructionPointer:     " NK_PRINTF_UINT32 "\n", vm->currentExecutionContext->instructionPointer);
    nkiDbgDumpListing(vm, NULL, stream);

    fprintf(stream, "String table:\n");
    fprintf(stream, "  capacity: " NK_PRINTF_UINT32 "\n", vm->stringTable.capacity);
    fprintf(stream, "  holes:\n");
    {
        char *holeTracker = (char *)nkiMalloc(
            vm, vm->stringTable.capacity);
        struct NKVMTableHole *hole = vm->stringTable.tableHoles;

        nkiMemset(holeTracker, 0, vm->stringTable.capacity);

        while(hole) {
            assert(!holeTracker[hole->index]);
            holeTracker[hole->index] = 1;
            hole = hole->next;
        }

        for(i = 0; i < vm->stringTable.capacity; i++) {
            if(holeTracker[i]) {
                fprintf(stream, "    " NK_PRINTF_UINT32 "\n", i);
            }
        }

        nkiFree(vm, holeTracker);
    }
    {
        // Thanks AFL! The size and count were swapped, so the size
        // would sometimes be zero and cause a divide by zero in the
        // checking.
        nkuint32_t *bucketTracker = (nkuint32_t *)nkiMallocArray(
            vm, sizeof(nkuint32_t), vm->stringTable.capacity);
        for(i = 0; i < nkiVmStringHashTableSize; i++) {
            struct NKVMString *str = vm->stringsByHash[i];
            while(str) {
                bucketTracker[str->stringTableIndex] = i;
                str = str->nextInHashBucket;
            }
        }
        nkiFree(vm, bucketTracker);
    }
    fprintf(stream, "  strings:\n");
    for(i = 0; i < vm->stringTable.capacity; i++) {
        if(vm->stringTable.stringTable[i]) {
            fprintf(stream, "    string " NK_PRINTF_UINT32 ":\n", i);
            fprintf(stream, "      stringTableIndex: " NK_PRINTF_UINT32 "\n", vm->stringTable.stringTable[i]->stringTableIndex);
            fprintf(stream, "      dontGC:           " NK_PRINTF_UINT32 "\n", (nkuint32_t)(vm->stringTable.stringTable[i]->dontGC ? 1 : 0));
            fprintf(stream, "      hash:             " NK_PRINTF_UINT32 "\n", vm->stringTable.stringTable[i]->hash);
            fprintf(stream, "      data:             ");
            fprintf(stream, "      lastGCPass:       " NK_PRINTF_UINT32 "\n", vm->stringTable.stringTable[i]->lastGCPass);
            nkiDbgDumpRaw(stream, vm->stringTable.stringTable[i]->str, nkiStrlen(vm->stringTable.stringTable[i]->str));
            fprintf(stream, "\n");
        }
    }

    fprintf(stream, "objectTable:\n");
    fprintf(stream, "  capacity: " NK_PRINTF_UINT32 "\n", vm->objectTable.capacity);
    fprintf(stream, "  holes:\n");
    {
        char *holeTracker = (char *)nkiMalloc(
            vm, vm->objectTable.capacity);
        struct NKVMTableHole *hole = vm->objectTable.tableHoles;

        nkiMemset(holeTracker, 0, vm->objectTable.capacity);

        while(hole) {
            assert(!holeTracker[hole->index]);
            holeTracker[hole->index] = 1;
            hole = hole->next;
        }

        for(i = 0; i < vm->objectTable.capacity; i++) {
            if(holeTracker[i]) {
                fprintf(stream, "    " NK_PRINTF_UINT32 "\n", i);
            }
        }

        nkiFree(vm, holeTracker);
    }
    fprintf(stream, "  objects:\n");
    for(i = 0; i < vm->objectTable.capacity; i++) {
        if(vm->objectTable.objectTable[i]) {
            struct NKVMObject *ob = vm->objectTable.objectTable[i];
            fprintf(stream, "    " NK_PRINTF_UINT32 "\n", i);
            fprintf(stream, "      index: " NK_PRINTF_UINT32 "\n", ob->objectTableIndex);
            fprintf(stream, "      lastGCPass: " NK_PRINTF_UINT32 "\n", ob->lastGCPass);
            fprintf(stream, "      size: " NK_PRINTF_UINT32 "\n", ob->size);
            fprintf(stream, "      external handles: " NK_PRINTF_UINT32 "\n", ob->externalHandleCount);
            fprintf(stream, "      hashbuckets:\n");
            {
                nkuint32_t n;
                for(n = 0; n < nkiVMObjectHashBucketCount; n++) {
                    // FIXME: Output values here in a deterministic
                    // order, instead of just however they showed up
                    // in the list?
                    struct NKVMObjectElement *el = ob->hashBuckets[n];
                    if(el) {
                        fprintf(stream, "        " NK_PRINTF_UINT32 ":\n", n);
                        while(el) {
                            fprintf(stream, "          key: ");
                            nkiValueDump(vm, &el->key, stream);
                            fprintf(stream, "\n          value: ");
                            nkiValueDump(vm, &el->value, stream);
                            fprintf(stream, "\n");
                            el = el->next;
                        }
                    }
                }
            }
            fprintf(stream, "      externalDataType: " NK_PRINTF_UINT32 "\n", ob->externalDataType.id);
        }
    }

    // Functions.
    fprintf(stream, "functions: " NK_PRINTF_UINT32 "\n", vm->functionCount);
    for(i = 0; i < vm->functionCount; i++) {
        fprintf(stream, "  " NK_PRINTF_UINT32 ":\n", i);
        fprintf(stream, "    argumentCount: " NK_PRINTF_UINT32 "\n", vm->functionTable[i].argumentCount);
        fprintf(stream, "    firstInstructionIndex: " NK_PRINTF_UINT32 "\n", vm->functionTable[i].firstInstructionIndex);
        fprintf(stream, "    externalFunctionId: " NK_PRINTF_UINT32 "\n", vm->functionTable[i].externalFunctionId.id);
    }

    // External functions.
    fprintf(stream, "external functions: " NK_PRINTF_UINT32 "\n", vm->externalFunctionCount);
    for(i = 0; i < vm->externalFunctionCount; i++) {

        fprintf(stream, "  " NK_PRINTF_UINT32 ":\n", i);
        fprintf(stream, "    name: %s\n", vm->externalFunctionTable[i].name);
        // fprintf(stream, "    CFunctionCallback: %p\n", vm->externalFunctionTable[i].CFunctionCallback);
        fprintf(stream, "    internalFunctionId: " NK_PRINTF_UINT32 "\n", vm->externalFunctionTable[i].internalFunctionId.id);
        fprintf(stream, "    argumentCount: " NK_PRINTF_UINT32 "\n", vm->externalFunctionTable[i].argumentCount);
        fprintf(stream, "    argTypes: " NK_PRINTF_UINT32 "\n", vm->externalFunctionTable[i].argumentCount);

        if(vm->externalFunctionTable[i].argumentCount != NK_INVALID_VALUE) {
            nkuint32_t n;
            for(n = 0; n < vm->externalFunctionTable[i].argumentCount; n++) {

                fprintf(
                    stream, "      " NK_PRINTF_UINT32 " int: %s",
                    n,
                    nkiValueTypeGetName(vm->externalFunctionTable[i].argTypes[n]));

                if(vm->externalFunctionTable[i].argTypes) {
                    fprintf(
                        stream, ", ext: " NK_PRINTF_UINT32,
                        vm->externalFunctionTable[i].argExternalTypes[n].id);
                }

                fprintf(stream, "\n");
            }
        }
    }

    // Global variables.
    fprintf(stream, "globals: " NK_PRINTF_UINT32 "\n", vm->globalVariableCount);
    for(i = 0; i < vm->globalVariableCount; i++) {
        fprintf(stream, "  " NK_PRINTF_UINT32 ":\n", i);
        fprintf(stream, "    name: %s\n", vm->globalVariables[i].name);
        fprintf(stream, "    staticPosition: " NK_PRINTF_UINT32 "\n", vm->globalVariables[i].staticPosition);
    }

    // External types.
    fprintf(stream, "externalTypes: " NK_PRINTF_UINT32 "\n", vm->externalTypeCount);
    for(i = 0; i < vm->externalTypeCount; i++) {
        fprintf(stream, "  " NK_PRINTF_UINT32 ":\n", i);
        fprintf(stream, "    name: %s\n", vm->externalTypes[i].name);
    }

    // GC stuff? I dunno if we should include that in the serialized
    // state, but we can. Also, we should dump it here regardless of
    // serialization.
    fprintf(stream, "Gc stuff:\n");
    fprintf(stream, "  lastGCPass: " NK_PRINTF_UINT32 "\n", vm->gcInfo.lastGCPass);
    fprintf(stream, "  gcInterval: " NK_PRINTF_UINT32 "\n", vm->gcInfo.gcInterval);
    fprintf(stream, "  gcCountdown: " NK_PRINTF_UINT32 "\n", vm->gcInfo.gcCountdown);
    fprintf(stream, "  gcNewObjectInterval: " NK_PRINTF_UINT32 "\n", vm->gcInfo.gcNewObjectInterval);
    fprintf(stream, "  gcNewObjectCountdown: " NK_PRINTF_UINT32 "\n", vm->gcInfo.gcNewObjectCountdown);

    // TODO: Memory limits (even if not serialized).

    // Check allocations? Same number?
    fprintf(stream, "Current memory usage: " NK_PRINTF_UINT32 "\n", vm->currentMemoryUsage);
}

void nkiCheckStringTableHoles(struct NKVM *vm)
{
    char *holeTracker = (char *)malloc(vm->stringTable.capacity);
    struct NKVMTableHole *hole = vm->stringTable.tableHoles;

    printf("Checking holes\n");

    nkiMemset(holeTracker, 0, vm->stringTable.capacity);

    // printf("HOLE LIST...\n");
    while(hole) {
        // printf("HOLE: %d\n", hole->index);
        assert(hole->index < vm->stringTable.capacity);
        assert(!vm->stringTable.stringTable[hole->index]);
        assert(!holeTracker[hole->index]);
        holeTracker[hole->index] = 1;
        hole = hole->next;
    }

    free(holeTracker);

    {
        nkuint32_t n;
        for(n = 0; n < vm->stringTable.capacity; n++) {
            if(vm->stringTable.stringTable[n]) {
                assert(vm->stringTable.stringTable[n]->stringTableIndex == n);
            }
        }

        for(n = 0; n < nkiVmStringHashTableSize; n++) {

            struct NKVMString *str = vm->stringsByHash[n];

            while(str) {
                assert(vm->stringTable.stringTable[str->stringTableIndex] == str);
                str = str->nextInHashBucket;
            }
        }
    }

    printf("Checking holes complete\n");

}

void nkiVmStringTableDump(struct NKVM *vm)
{
    struct NKVMTable *table = &vm->stringTable;
    nkuint32_t i;
    printf("String table dump...\n");

    printf("  Hash table...\n");
    for(i = 0; i < nkiVmStringHashTableSize  ; i++) {
        struct NKVMString *str = vm->stringsByHash[i];
        printf("    %.2x\n", i);
        while(str) {
            printf("      %s\n", str->str);
            str = str->nextInHashBucket;
        }
    }

    printf("  Main table...\n");
    for(i = 0; i < table->capacity; i++) {
        struct NKVMString *str = table->stringTable[i];
        printf("    %.2x\n", i);
        printf("      %s\n", str ? str->str : "<null>");
    }
}

void nkiVmObjectTableDump(struct NKVM *vm)
{
    nkuint32_t index;
    printf("Object table dump...\n");
    for(index = 0; index < vm->objectTable.capacity; index++) {
        if(vm->objectTable.objectTable[index]) {

            struct NKVMObject *ob = vm->objectTable.objectTable[index];
            nkuint32_t bucket;

            printf("%4u ", index);
            printf("Object\n");

            for(bucket = 0; bucket < nkiVMObjectHashBucketCount; bucket++) {
                struct NKVMObjectElement *el = ob->hashBuckets[bucket];
                printf("      Hash bucket %u\n", bucket);
                while(el) {
                    printf("        ");
                    nkiValueDump(vm, &el->key, stdout);
                    printf(" = ");
                    nkiValueDump(vm, &el->value, stdout);
                    printf("\n");
                    el = el->next;
                }
            }
            printf("\n");
        }
    }
}

void nkiVmStaticDump(struct NKVM *vm)
{
    nkuint32_t i = 0;
    while(1) {

        printf("%3d: ", i);
        nkiValueDump(vm, &vm->staticSpace[i], stdout);
        printf("\n");

        if(i == vm->staticAddressMask) {
            break;
        }
        i++;
    }
}

void nkiVmStackDump(struct NKVM *vm)
{
    nkuint32_t i;
    struct NKVMStack *stack = &vm->currentExecutionContext->stack;
    for(i = 0; i < stack->size; i++) {
        printf("%3d: ", i);
        nkiValueDump(vm, nkiVmStackPeek(vm, i), stdout);
        printf("\n");
    }
}

nkbool nkiValueDump(
    struct NKVM *vm, struct NKValue *value, FILE *stream)
{
    switch(value->type) {

        case NK_VALUETYPE_INT:
            fprintf(stream, NK_PRINTF_INT32, value->intData);
            break;

        case NK_VALUETYPE_FLOAT:
            fprintf(stream, "%f", value->floatData);
            break;

        case NK_VALUETYPE_STRING: {
            const char *str = nkiVmStringTableGetStringById(
                &vm->stringTable,
                value->stringTableEntry);
            char escapedBuf[80];
            if(!str) str = "<bad id>";
            escapedBuf[0] = 0;
            nkiDbgAppendEscaped(sizeof(escapedBuf), escapedBuf, str);
            fprintf(stream, "string:" NK_PRINTF_UINT32 ":\"%s\"", value->stringTableEntry, escapedBuf);
        } break;

        case NK_VALUETYPE_NIL:
            fprintf(stream, "<nil>");
            break;

        case NK_VALUETYPE_FUNCTIONID:
            fprintf(stream, "<function:" NK_PRINTF_UINT32 ">", value->functionId.id);
            break;

        case NK_VALUETYPE_OBJECTID:
            fprintf(stream, "<object:" NK_PRINTF_UINT32 ">", value->objectId);
            break;

        default:
            fprintf(stream,
                "nkiValueDump unimplemented for type %s",
                nkiValueTypeGetName(value->type));
            return nkfalse;
    }
    return nktrue;
}

void nkiVmObjectTableSanityCheck(struct NKVM *vm)
{
    if(vm) {
        struct NKVMTable *table = &vm->objectTable;
        if(table) {
            if(table->objectTable) {
                nkuint32_t i;
                for(i = 0; i < table->capacity; i++) {
                    if(table->objectTable[i]) {
                        assert(table->objectTable[i]->objectTableIndex == i);
                    }
                }
            }
        }
    }
}

void nkiExternalHandleSanityCheck(struct NKVM *vm)
{
    if(vm) {
        struct NKVMTable *table = &vm->objectTable;
        if(table) {
            if(table->objectTable) {
                nkuint32_t i;
                for(i = 0; i < table->capacity; i++) {
                    struct NKVMObject *ob = table->objectTable[i];
                    if(ob) {
                        assert(ob->objectTableIndex == i);
                        if(ob->externalHandleCount) {
                            assert(ob->previousExternalHandleListPtr);
                        } else {
                            assert(!ob->nextObjectWithExternalHandles);
                            assert(!ob->previousExternalHandleListPtr);
                        }
                    }
                }
            }
        }
    }
}

const char *nkiDbgGetNextLine(const char *text)
{
    while(*text) {
        if(*text == '\n') {
            return text + 1;
        }
        text++;
    }
    return NULL;
}

void nkiDbgAppendLine(nkuint32_t bufSize, char *dst, const char *input)
{
    nkuint32_t i;
    for(i = nkiStrlen(dst); i + 4 < bufSize; i++) {
        if(*input && *input != '\n') {
            dst[i] = *(input++);
        } else {
            dst[i] = 0;
            return;
        }
    }

    // Add '...' to the end to indicate that we just ran out of room.
    if(i + 4 == bufSize) {
        dst[i++] = '.';
        dst[i++] = '.';
        dst[i++] = '.';
    }

    dst[i] = 0;
}

void nkiDbgPadLine(nkuint32_t padding, char *dst, char value)
{
    nkuint32_t i;
    for(i = nkiStrlen(dst); i + 1 < padding; i++) {
        dst[i] = value;
    }
    dst[i] = 0;
}

nkuint32_t nkiDbgCountLines(const char *str)
{
    nkuint32_t c = 0;
    while(*str) {
        if(*str == '\n') {
            c++;
        }
        str++;
    }
    return c;
}

void nkiDbgDumpListing(struct NKVM *vm, const char *script, FILE *stream)
{
    nkuint32_t i;

    // FIXME: (markers) Debug source code output is broken.

    // nkuint32_t lastLine = 0;
    // nkuint32_t lineCount = script ? nkiDbgCountLines(script) : 0;
    // const char *linePtr = script;

    char lineBuf[80];
    char paramBuf[80];

    if(vm->instructions) {
        for(i = 0; i <= vm->instructionAddressMask; i++) {

            enum NKOpcode opcode = vm->instructions[i].opcode;
            struct NKInstruction *maybeParams =
                &vm->instructions[(i+1) & vm->instructionAddressMask];

            if(opcode == NK_OP_END) {
                break;
            }

            // Start off with the instruction address and instruction
            // name.
            sprintf(lineBuf, "%s " NK_PRINTF_UINT32,
                (i == vm->currentExecutionContext->instructionPointer ? ">" : " "),
                i);
            nkiDbgPadLine(7, lineBuf, ' ');

            nkiDbgAppendLine(sizeof(lineBuf), lineBuf, ": ");
            nkiDbgAppendLine(sizeof(lineBuf), lineBuf, nkiVmGetOpcodeName(opcode));

            // Format parameters.
            if(vm->instructions[i].opcode == NK_OP_PUSHLITERAL_INT) {
                i++;
                sprintf(paramBuf, " %d", maybeParams->opData_int);
            } else if(vm->instructions[i].opcode == NK_OP_PUSHLITERAL_FLOAT) {
                i++;
                sprintf(paramBuf, " %f", maybeParams->opData_float);
            } else if(vm->instructions[i].opcode == NK_OP_PUSHLITERAL_FUNCTIONID) {
                i++;
                sprintf(paramBuf, " %u", maybeParams->opData_functionId.id);
            } else if(vm->instructions[i].opcode == NK_OP_PUSHLITERAL_STRING) {
                const char *str = nkiVmStringTableGetStringById(&vm->stringTable, maybeParams->opData_string);
                i++;
                sprintf(paramBuf, " %d:\"", maybeParams->opData_string);
                nkiDbgAppendEscaped(sizeof(paramBuf), paramBuf, str ? str : "<bad string>");
                nkiDbgAppendLine(sizeof(paramBuf), paramBuf, "\"");
            } else {
                paramBuf[0] = 0;
            }

            // Append parameters.
            nkiDbgAppendLine(sizeof(lineBuf)/2, lineBuf, paramBuf);

            // FIXME: (markers) Debug source code output is broken as
            // of the introduction of file/line markers, and probably
            // since the introduction of file/line stuff entirely.

            // if(linePtr) {

            //     struct NKVMFilePositionMarker *marker =
            //         nkiVmFindSourceMarker(vm, i);

            //     if(marker && lastLine < marker->lineNumber && lastLine < lineCount) {

            //         while(lastLine < marker->lineNumber && lastLine < lineCount) {

            //             // Add the source code to this line and print
            //             // it.
            //             nkiDbgPadLine(sizeof(lineBuf)/2, lineBuf, ' ');
            //             nkiDbgAppendLine(sizeof(lineBuf), lineBuf, " ; ");
            //             nkiDbgAppendLine(sizeof(lineBuf), lineBuf, linePtr);
            //             fprintf(stream, "%s\n", lineBuf);

            //             // Move to the next line. Clear the lineBuf in
            //             // case we have multiple source lines for this
            //             // one line of assembly.
            //             linePtr = nkiDbgGetNextLine(linePtr);
            //             lastLine++;
            //             lineBuf[0] = 0;
            //         }

            //     } else {

            //         fprintf(stream, "%s\n", lineBuf);

            //     }

            // } else
            {

                fprintf(stream, "%s\n", lineBuf);

            }

        }
    }
}


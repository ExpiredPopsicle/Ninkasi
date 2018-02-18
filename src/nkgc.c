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

// Garbage collector implementation.

#include "nkcommon.h"

struct NKVMValueGCEntry
{
    struct NKVMObject *object;
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

void nkiVmGarbageCollect_markString(
    struct NKVMGCState *gcState,
    struct NKValue *value)
{
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
}

void nkiVmGarbageCollect_addObject(
    struct NKVMGCState *gcState,
    struct NKValue *value)
{
    struct NKVMObject *ob = nkiVmObjectTableGetEntryById(
        &gcState->vm->objectTable,
        value->objectId);

    if(ob) {
        if(ob->lastGCPass != gcState->currentGCPass) {
            struct NKVMValueGCEntry *newEntry = nkiVmGcStateMakeEntry(gcState);
            newEntry->object = ob;
        }
    } else {
        // Thanks AFL!
        nkiAddError(gcState->vm, -1, "Bad object reference in garbage collector.");
        return;
    }
}

void nkiVmGarbageCollect_markValue(
    struct NKVMGCState *gcState,
    struct NKValue *v)
{
    if(v->type == NK_VALUETYPE_OBJECTID) {

        // Add this object for a later pass to avoid over-recursion.
        nkiVmGarbageCollect_addObject(
            gcState, v);

    } else if(v->type == NK_VALUETYPE_STRING) {

        // Just a string. No risk of over-recursion. Mark it directly.
        nkiVmGarbageCollect_markString(
            gcState, v);

    }
}

void nkiVmGarbageCollect_markObject(
    struct NKVMGCState *gcState,
    struct NKVMObject *ob)
{
    nkuint32_t bucket;

    // Did we already get this one?
    if(ob->lastGCPass == gcState->currentGCPass) {
        return;
    }

    ob->lastGCPass = gcState->currentGCPass;

    // Iterate through all the hash buckets on this object.
    for(bucket = 0; bucket < nkiVMObjectHashBucketCount; bucket++) {

        // Iterate through the elements inside the hash bucket.
        struct NKVMObjectElement *el = ob->hashBuckets[bucket];
        while(el) {

            // Iterate between key and value for this element.
            nkuint32_t k;
            for(k = 0; k < 2; k++) {
                struct NKValue *v = k ? &el->value : &el->key;
                nkiVmGarbageCollect_markValue(gcState, v);
            }

            el = el->next;
        }
    }

}

void nkiVmGarbageCollect_markReferenced(
    struct NKVMGCState *gcState)
{
    while(gcState->openList) {

        // Remove the value from the list.
        struct NKVMValueGCEntry *currentEntry = gcState->openList;
        gcState->openList = gcState->openList->next;

        nkiVmGarbageCollect_markObject(gcState, currentEntry->object);

        currentEntry->next = gcState->closedList;
        gcState->closedList = currentEntry;
    }
}

void nkiVmGarbageCollect(struct NKVM *vm)
{
    struct NKVMGCState gcState;
    memset(&gcState, 0, sizeof(gcState));
    gcState.currentGCPass = ++vm->gcInfo.lastGCPass;
    gcState.vm = vm;

    // Iterate through objects with external handles.
    {
        struct NKVMObject *ob;
        for(ob = vm->objectsWithExternalHandles; ob;
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
    }
}


#include "common.h"

void errorStateAddError(
    struct ErrorState *es,
    int32_t lineNumber,
    const char *str)
{
    struct Error *newError = malloc(
        sizeof(struct Error));

    if(!newError) {
        return;
    }

    // if(!newError) {
    //     NK_CATASTROPHE();
    // }

    newError->errorText =
        malloc(strlen(str) + 2 + sizeof(lineNumber) * 8 + 1);
    newError->errorText[0] = 0;
    sprintf(newError->errorText, "%d: %s", lineNumber, str);

    newError->next = NULL;

    // Add error to the error list.
    if(es->lastError) {
        es->lastError->next = newError;
    }
    es->lastError = newError;
    if(!es->firstError) {
        es->firstError = newError;
    }
}

void errorStateSetAllocationFailFlag(
    struct ErrorState *es)
{
    es->allocationFailure = true;
}

void errorStateInit(struct ErrorState *es)
{
    es->firstError = NULL;
    es->lastError = NULL;
    es->allocationFailure = false;
}

void errorStateDestroy(struct ErrorState *es)
{
    struct Error *e = es->firstError;
    while(e) {
        struct Error *next = e->next;
        free(e->errorText);
        free(e);
        e = next;
    }
}

bool errorStateHasErrors(const struct ErrorState *es)
{
    return (es->firstError || es->allocationFailure);
}

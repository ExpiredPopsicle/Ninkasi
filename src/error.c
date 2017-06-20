#include "common.h"

void errorStateAddError(
    struct ErrorState *es,
    int32_t lineNumber,
    const char *str)
{
    struct DynString *fixedStr = dynStrCreate("");
    struct Error *newError = malloc(sizeof(struct Error));

    dynStrAppendInt32(fixedStr, lineNumber);
    dynStrAppend(fixedStr, ": ");
    dynStrAppend(fixedStr, str);

    newError->errorText = strdup(fixedStr->data);
    newError->next = NULL;

    dynStrDelete(fixedStr);

    // Add error to the error list.
    if(es->lastError) {
        es->lastError->next = newError;
    }
    es->lastError = newError;
    if(!es->firstError) {
        es->firstError = newError;
    }
}

void errorStateInit(struct ErrorState *es)
{
    es->firstError = NULL;
    es->lastError = NULL;
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
    return !!(es->firstError);
}

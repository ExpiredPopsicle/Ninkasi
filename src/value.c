#include "common.h"

bool value_dump(struct Value *value)
{
    // TODO: Function pointer table here?
    switch(value->type) {

        case VALUETYPE_INT:
            printf("%d", value->intData);
            break;

        default:
            printf(
                "value_dump unimplemented for type %s",
                valueTypeGetName(value->type));
            return false;
    }
    return true;
}

const char *valueTypeGetName(enum ValueType type)
{
    switch(type) {

        case VALUETYPE_INT:
            return "integer";

        default:
            // If you hit this case, then something isn't implemented
            // yet!
            return "unknown";
    }
}

int32_t valueToInt(struct VM *vm, struct Value *value)
{
    // TODO: De-reference references here.

    switch(value->type) {

        case VALUETYPE_INT:
            return value->intData;

        default: {
            struct DynString *ds = dynStrCreate("Cannot convert type ");
            dynStrAppend(ds, valueTypeGetName(value->type));
            dynStrAppend(ds, " to an integer.");
            errorStateAddError(
                &vm->errorState, -1,
                ds->data);
            dynStrDelete(ds);
            return 0;
        }
    }
}

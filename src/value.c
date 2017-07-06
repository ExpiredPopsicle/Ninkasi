#include "common.h"

bool value_dump(
    struct VM *vm, struct Value *value)
{
    // TODO: Function pointer table here?
    switch(value->type) {

        case VALUETYPE_INT:
            printf("%d", value->intData);
            break;

        case VALUETYPE_FLOAT:
            printf("%f", value->floatData);
            break;

        case VALUETYPE_STRING: {
            const char *str = vmStringTableGetStringById(
                &vm->stringTable,
                value->stringTableEntry);
            printf("%d:%s", value->stringTableEntry, str ? str : "<bad id>");
        } break;

        case VALUETYPE_FUNCTIONID:
            printf("<function:%u>", value->functionId);
            break;

        case VALUETYPE_OBJECTID:
            printf("<object:%u>", value->objectId);
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

        case VALUETYPE_FLOAT:
            return "float";

        case VALUETYPE_STRING:
            return "string";

        case VALUETYPE_FUNCTIONID:
            return "function";

        case VALUETYPE_OBJECTID:
            return "object";

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

        case VALUETYPE_FLOAT:
            return (int)value->floatData;

        case VALUETYPE_STRING:
            return atoi(valueToString(vm, value));

        case VALUETYPE_OBJECTID:
            // This is just for detection of the presence of objects
            // (like a null pointer check).
            return 1;

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

float valueToFloat(struct VM *vm, struct Value *value)
{
    // TODO: De-reference references here.

    switch(value->type) {

        case VALUETYPE_INT:
            return (float)value->intData;

        case VALUETYPE_FLOAT:
            return value->floatData;

        case VALUETYPE_STRING:
            return atoi(valueToString(vm, value));

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

const char *valueToString(struct VM *vm, struct Value *value)
{
    // TODO: De-reference references here.

    switch(value->type) {

        case VALUETYPE_STRING:
            return vmStringTableGetStringById(
                &vm->stringTable,
                value->stringTableEntry);

        case VALUETYPE_INT: {
            struct DynString *dynStr = dynStrCreate("");
            uint32_t id;

            dynStrAppendInt32(dynStr, value->intData);

            id = vmStringTableFindOrAddString(&vm->stringTable, dynStr->data);
            dynStrDelete(dynStr);

            return vmStringTableGetStringById(
                &vm->stringTable,
                id);
        }

        case VALUETYPE_FLOAT: {
            struct DynString *dynStr = dynStrCreate("");
            uint32_t id;

            dynStrAppendFloat(dynStr, value->floatData);

            id = vmStringTableFindOrAddString(&vm->stringTable, dynStr->data);
            dynStrDelete(dynStr);

            return vmStringTableGetStringById(
                &vm->stringTable,
                id);
        }

        default: {
            struct DynString *ds = dynStrCreate("Cannot convert type ");
            dynStrAppend(ds, valueTypeGetName(value->type));
            dynStrAppend(ds, " to a string.");
            errorStateAddError(
                &vm->errorState, -1,
                ds->data);
            dynStrDelete(ds);
            return 0;
        }
    }
}

int32_t value_compareType(
    struct Value *in1,
    struct Value *in2)
{
    if(in1->type < in2->type) {
        return -1;
    } else if(in1->type > in2->type) {
        return 1;
    }
    return 0;
}

int32_t value_compare(
    struct VM *vm,
    struct Value *in1,
    struct Value *in2,
    bool strictType)
{
    enum ValueType type = in1->type;

    if(strictType) {
        int32_t typeDiff = value_compareType(in1, in2);
        if(typeDiff) {
            return typeDiff;
        }
    }

    switch(type) {

        case VALUETYPE_INT: {
            int32_t other = valueToInt(vm, in2);
            if(in1->intData > other) {
                return 1;
            } else if(in1->intData == other) {
                return 0;
            } else {
                return -1;
            }
        } break;

        case VALUETYPE_FLOAT: {
            float other = valueToFloat(vm, in2);
            if(in1->floatData > other) {
                return 1;
            } else if(in1->floatData == other) {
                return 0;
            } else {
                return -1;
            }
        } break;

        case VALUETYPE_STRING: {
            const char *other = valueToString(vm, in2);
            const char *thisData = valueToString(vm, in1);

            // Shortcut it if we ended up with two of the same entry
            // in the string table.
            if(other == thisData) {
                return 0;
            }

            return strcmp(thisData, other);
        } break;

        case VALUETYPE_FUNCTIONID: {
            if(in2->type == VALUETYPE_FUNCTIONID &&
                in2->functionId == in1->functionId)
            {
                return 0;
            } else {
                return in1->functionId < in2->functionId ? -1 : 1;
            }
        }

        case VALUETYPE_OBJECTID: {
            if(in2->type == VALUETYPE_OBJECTID &&
                in2->objectId == in1->objectId)
            {
                return 0;
            } else {
                return in1->objectId < in2->objectId ? -1 : 1;
            }
        }

        default: {
            struct DynString *ds =
                dynStrCreate("Comparison unimplemented for type ");
            dynStrAppend(ds, valueTypeGetName(type));
            dynStrAppend(ds, ".");
            errorStateAddError(
                &vm->errorState, -1,
                ds->data);
            dynStrDelete(ds);
        } break;
    }

    return ~0;
}

#include "common.h"

nkbool nkiValueDump(
    struct NKVM *vm, struct NKValue *value)
{
    // TODO: Function pointer table here?
    switch(value->type) {

        case NK_VALUETYPE_INT:
            printf(NK_PRINTF_INT32, value->intData);
            break;

        case NK_VALUETYPE_FLOAT:
            printf("%f", value->floatData);
            break;

        case NK_VALUETYPE_STRING: {
            const char *str = nkiVmStringTableGetStringById(
                &vm->stringTable,
                value->stringTableEntry);
            printf("%d:%s", value->stringTableEntry, str ? str : "<bad id>");
        } break;

        case NK_VALUETYPE_NIL:
            printf("<nil>");
            break;

        case NK_VALUETYPE_FUNCTIONID:
            printf("<function:%u>", value->functionId);
            break;

        case NK_VALUETYPE_OBJECTID:
            printf("<object:%u>", value->objectId);
            break;

        default:
            printf(
                "nkiValueDump unimplemented for type %s",
                nkiValueTypeGetName(value->type));
            return nkfalse;
    }
    return nktrue;
}

const char *nkiValueTypeGetName(enum NKValueType type)
{
    switch(type) {

        case NK_VALUETYPE_INT:
            return "integer";

        case NK_VALUETYPE_FLOAT:
            return "float";

        case NK_VALUETYPE_STRING:
            return "string";

        case NK_VALUETYPE_FUNCTIONID:
            return "function";

        case NK_VALUETYPE_OBJECTID:
            return "object";

        case NK_VALUETYPE_NIL:
            return "nil";

        default:
            // If you hit this case, then something isn't implemented
            // yet!
            return "unknown";
    }
}

nkint32_t nkiValueToInt(struct NKVM *vm, struct NKValue *value)
{
    // TODO: De-reference references here.

    switch(value->type) {

        case NK_VALUETYPE_INT:
            return value->intData;

        case NK_VALUETYPE_FLOAT:
            return (int)value->floatData;

        case NK_VALUETYPE_STRING:
            return atoi(valueToString(vm, value));

        case NK_VALUETYPE_OBJECTID:
            // This is just for detection of the presence of objects
            // (like a null pointer check).
            return 1;

        case NK_VALUETYPE_NIL:
            return 0;

        default: {
            struct NKDynString *ds = nkiDynStrCreate(vm, "Cannot convert type ");
            nkiDynStrAppend(ds, nkiValueTypeGetName(value->type));
            nkiDynStrAppend(ds, " to an integer.");
            nkiAddError(
                vm, -1,
                ds->data);
            nkiDynStrDelete(ds);
            return 0;
        }
    }
}

float nkiValueToFloat(struct NKVM *vm, struct NKValue *value)
{
    // TODO: De-reference references here.

    switch(value->type) {

        case NK_VALUETYPE_INT:
            return (float)value->intData;

        case NK_VALUETYPE_FLOAT:
            return value->floatData;

        case NK_VALUETYPE_STRING:
            return atof(valueToString(vm, value));

        case NK_VALUETYPE_NIL:
            return 0.0f;

        default: {
            struct NKDynString *ds = nkiDynStrCreate(vm, "Cannot convert type ");
            nkiDynStrAppend(ds, nkiValueTypeGetName(value->type));
            nkiDynStrAppend(ds, " to an integer.");
            nkiAddError(
                vm, -1,
                ds->data);
            nkiDynStrDelete(ds);
            return 0;
        }
    }
}

const char *valueToString(struct NKVM *vm, struct NKValue *value)
{
    // TODO: De-reference references here.

    switch(value->type) {

        case NK_VALUETYPE_STRING:
            return nkiVmStringTableGetStringById(
                &vm->stringTable,
                value->stringTableEntry);

        case NK_VALUETYPE_INT: {
            struct NKDynString *dynStr = nkiDynStrCreate(vm, "");
            nkuint32_t id;

            nkiDynStrAppendInt32(dynStr, value->intData);

            id = nkiVmStringTableFindOrAddString(
                vm, dynStr->data);
            nkiDynStrDelete(dynStr);

            return nkiVmStringTableGetStringById(
                &vm->stringTable,
                id);
        }

        case NK_VALUETYPE_FLOAT: {
            struct NKDynString *dynStr = nkiDynStrCreate(vm, "");
            nkuint32_t id;

            nkiDynStrAppendFloat(dynStr, value->floatData);

            id = nkiVmStringTableFindOrAddString(
                vm, dynStr->data);
            nkiDynStrDelete(dynStr);

            return nkiVmStringTableGetStringById(
                &vm->stringTable,
                id);
        }

        default: {
            struct NKDynString *dynStr = nkiDynStrCreate(vm, "<");
            nkuint32_t id;

            nkiDynStrAppend(dynStr, nkiValueTypeGetName(value->type));
            nkiDynStrAppend(dynStr, ":");
            nkiDynStrAppendInt32(dynStr, valueHash(vm, value));
            nkiDynStrAppend(dynStr, ">");

            id = nkiVmStringTableFindOrAddString(
                vm, dynStr->data);
            nkiDynStrDelete(dynStr);

            return nkiVmStringTableGetStringById(
                &vm->stringTable,
                id);
        }
    }
}

nkint32_t value_compareType(
    struct NKValue *in1,
    struct NKValue *in2)
{
    if(in1->type < in2->type) {
        return -1;
    } else if(in1->type > in2->type) {
        return 1;
    }
    return 0;
}

nkint32_t value_compare(
    struct NKVM *vm,
    struct NKValue *in1,
    struct NKValue *in2,
    nkbool strictType)
{
    enum NKValueType type = in1->type;

    if(strictType) {
        nkint32_t typeDiff = value_compareType(in1, in2);
        if(typeDiff) {
            return typeDiff;
        }
    }

    switch(type) {

        case NK_VALUETYPE_INT: {
            nkint32_t other = nkiValueToInt(vm, in2);
            if(in1->intData > other) {
                return 1;
            } else if(in1->intData == other) {
                return 0;
            } else {
                return -1;
            }
        } break;

        case NK_VALUETYPE_FLOAT: {
            float other = nkiValueToFloat(vm, in2);
            if(in1->floatData > other) {
                return 1;
            } else if(in1->floatData == other) {
                return 0;
            } else {
                return -1;
            }
        } break;

        case NK_VALUETYPE_STRING: {
            const char *other = valueToString(vm, in2);
            const char *thisData = valueToString(vm, in1);

            // Shortcut it if we ended up with two of the same entry
            // in the string table.
            if(other == thisData) {
                return 0;
            }

            return strcmp(thisData, other);
        } break;

        case NK_VALUETYPE_FUNCTIONID: {
            if(in2->type == NK_VALUETYPE_FUNCTIONID &&
                in2->functionId == in1->functionId)
            {
                return 0;
            } else {
                return in1->functionId < in2->functionId ? -1 : 1;
            }
        }

        case NK_VALUETYPE_OBJECTID: {
            if(in2->type == NK_VALUETYPE_OBJECTID &&
                in2->objectId == in1->objectId)
            {
                return 0;
            } else {
                return in1->objectId < in2->objectId ? -1 : 1;
            }
        }

        default: {
            struct NKDynString *ds =
                nkiDynStrCreate(vm, "Comparison unimplemented for type ");
            nkiDynStrAppend(ds, nkiValueTypeGetName(type));
            nkiDynStrAppend(ds, ".");
            nkiAddError(
                vm, -1,
                ds->data);
            nkiDynStrDelete(ds);
        } break;
    }

    return ~0;
}

nkuint32_t valueHash(struct NKVM *vm, struct NKValue *value)
{
    nkuint32_t ret = 0;

    switch(value->type) {

        case NK_VALUETYPE_INT:
        case NK_VALUETYPE_FLOAT:
        case NK_VALUETYPE_FUNCTIONID:
        case NK_VALUETYPE_OBJECTID:
            ret = value->basicHashValue;
            break;

        case NK_VALUETYPE_STRING: {

            struct NKVMString *vmStr = nkiVmStringTableGetEntryById(
                &vm->stringTable,
                value->stringTableEntry);

            if(vmStr) {
                ret = vmStr->hash;
            } else {
                ret = 0;
            }

        } break;

        default:
            // Zero, I guess?
            break;
    }

    return ret;
}

void vmValueSetInt(struct NKVM *vm, struct NKValue *value, nkint32_t intData)
{
    value->type = NK_VALUETYPE_INT;
    value->intData = intData;
}

void vmValueSetFloat(struct NKVM *vm, struct NKValue *value, float floatData)
{
    value->type = NK_VALUETYPE_FLOAT;
    value->floatData = floatData;
}

void vmValueSetString(struct NKVM *vm, struct NKValue *value, const char *str)
{
    value->type = NK_VALUETYPE_STRING;
    value->stringTableEntry =
        nkiVmStringTableFindOrAddString(
            vm, str);
}


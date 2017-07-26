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

        case VALUETYPE_NIL:
            printf("<nil>");
            break;

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

        case VALUETYPE_NIL:
            return "nil";

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

        case VALUETYPE_NIL:
            return 0;

        default: {
            struct NKDynString *ds = nkiDynStrCreate(vm, "Cannot convert type ");
            nkiDynStrAppend(ds, valueTypeGetName(value->type));
            nkiDynStrAppend(ds, " to an integer.");
            nkiAddError(
                vm, -1,
                ds->data);
            nkiDynStrDelete(ds);
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
            return atof(valueToString(vm, value));

        case VALUETYPE_NIL:
            return 0.0f;

        default: {
            struct NKDynString *ds = nkiDynStrCreate(vm, "Cannot convert type ");
            nkiDynStrAppend(ds, valueTypeGetName(value->type));
            nkiDynStrAppend(ds, " to an integer.");
            nkiAddError(
                vm, -1,
                ds->data);
            nkiDynStrDelete(ds);
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
            struct NKDynString *dynStr = nkiDynStrCreate(vm, "");
            uint32_t id;

            nkiDynStrAppendInt32(dynStr, value->intData);

            id = vmStringTableFindOrAddString(
                vm, dynStr->data);
            nkiDynStrDelete(dynStr);

            return vmStringTableGetStringById(
                &vm->stringTable,
                id);
        }

        case VALUETYPE_FLOAT: {
            struct NKDynString *dynStr = nkiDynStrCreate(vm, "");
            uint32_t id;

            nkiDynStrAppendFloat(dynStr, value->floatData);

            id = vmStringTableFindOrAddString(
                vm, dynStr->data);
            nkiDynStrDelete(dynStr);

            return vmStringTableGetStringById(
                &vm->stringTable,
                id);
        }

        default: {
            struct NKDynString *dynStr = nkiDynStrCreate(vm, "<");
            uint32_t id;

            nkiDynStrAppend(dynStr, valueTypeGetName(value->type));
            nkiDynStrAppend(dynStr, ":");
            nkiDynStrAppendInt32(dynStr, valueHash(vm, value));
            nkiDynStrAppend(dynStr, ">");

            id = vmStringTableFindOrAddString(
                vm, dynStr->data);
            nkiDynStrDelete(dynStr);

            return vmStringTableGetStringById(
                &vm->stringTable,
                id);
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
            struct NKDynString *ds =
                nkiDynStrCreate(vm, "Comparison unimplemented for type ");
            nkiDynStrAppend(ds, valueTypeGetName(type));
            nkiDynStrAppend(ds, ".");
            nkiAddError(
                vm, -1,
                ds->data);
            nkiDynStrDelete(ds);
        } break;
    }

    return ~0;
}

uint32_t valueHash(struct VM *vm, struct Value *value)
{
    uint32_t ret = 0;

    switch(value->type) {

        case VALUETYPE_INT:
        case VALUETYPE_FLOAT:
        case VALUETYPE_FUNCTIONID:
        case VALUETYPE_OBJECTID:
            ret = value->basicHashValue;
            break;

        case VALUETYPE_STRING: {

            struct VMString *vmStr = vmStringTableGetEntryById(
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

void vmValueSetInt(struct VM *vm, struct Value *value, int32_t intData)
{
    value->type = VALUETYPE_INT;
    value->intData = intData;
}

void vmValueSetFloat(struct VM *vm, struct Value *value, float floatData)
{
    value->type = VALUETYPE_FLOAT;
    value->floatData = floatData;
}

void vmValueSetString(struct VM *vm, struct Value *value, const char *str)
{
    value->type = VALUETYPE_STRING;
    value->stringTableEntry =
        vmStringTableFindOrAddString(
            vm, str);
}


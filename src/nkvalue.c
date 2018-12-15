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

const char *nkiValueTypeGetName(enum NKValueType type)
{
    // Note: Don't do any heap allocations starting here. It's called
    // straight from some nkx functions without the error handler
    // setup.

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
    switch(value->type) {

        case NK_VALUETYPE_INT:
            return value->intData;

        case NK_VALUETYPE_FLOAT:
            return (int)value->floatData;

        case NK_VALUETYPE_STRING:
        {
            // Thanks AFL! Sometimes nkiValueToString returns NULL,
            // because nkiVmStringTableGetStringById can return NULL.
            const char *str = nkiValueToString(vm, value);
            if(str) {
                return atol(str);
            }
            return 0;
        }

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
    switch(value->type) {

        case NK_VALUETYPE_INT:
            return (float)value->intData;

        case NK_VALUETYPE_FLOAT:
            return value->floatData;

        case NK_VALUETYPE_STRING:
        {
            // Thanks AFL! Sometimes nkiValueToString returns NULL,
            // because nkiVmStringTableGetStringById can return NULL.
            const char *str = nkiValueToString(vm, value);
            if(str) {
                return atof(str);
            }
            return 0.0f;
        }

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

const char *nkiValueToString(struct NKVM *vm, struct NKValue *value)
{
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
            nkiDynStrAppendInt32(dynStr, nkiValueHash(vm, value));
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

nkint32_t nkiValueCompare(
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
            const char *other = nkiValueToString(vm, in2);
            const char *thisData = nkiValueToString(vm, in1);

            // Thanks AFL!
            if(!other || !thisData) {
                nkiAddError(vm, -1, "Bad string comparison.");
                return 1;
            }

            // Shortcut it if we ended up with two of the same entry
            // in the string table.
            if(other == thisData) {
                return 0;
            }

            return nkiStrcmp(thisData, other);
        } break;

        case NK_VALUETYPE_FUNCTIONID: {
            if(in2->type == NK_VALUETYPE_FUNCTIONID &&
                in2->functionId.id == in1->functionId.id)
            {
                return 0;
            } else {
                return in1->functionId.id < in2->functionId.id ? -1 : 1;
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

nkuint32_t nkiValueHash(struct NKVM *vm, struct NKValue *value)
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

void nkiValueSetInt(struct NKVM *vm, struct NKValue *value, nkint32_t intData)
{
    // Clear out full 32-bits even on systems where "type" is only 16
    // bits.
    value->type_uint = 0;
    value->type = NK_VALUETYPE_INT;
    value->intData = intData;
}

void nkiValueSetFloat(struct NKVM *vm, struct NKValue *value, float floatData)
{
    // Clear out full 32-bits even on systems where "type" is only 16
    // bits.
    value->type_uint = 0;
    value->type = NK_VALUETYPE_FLOAT;
    value->floatData = floatData;
}

void nkiValueSetString(struct NKVM *vm, struct NKValue *value, const char *str)
{
    // Clear out full 32-bits even on systems where "type" is only 16
    // bits.
    value->type_uint = 0;
    value->type = NK_VALUETYPE_STRING;
    value->stringTableEntry =
        nkiVmStringTableFindOrAddString(
            vm, str ? str : "");
}

void nkiValueSetFunction(struct NKVM *vm, struct NKValue *value, NKVMInternalFunctionID id)
{
    // Clear out full 32-bits even on systems where "type" is only 16
    // bits.
    value->type_uint = 0;
    value->type = NK_VALUETYPE_FUNCTIONID;
    value->functionId = id;
}


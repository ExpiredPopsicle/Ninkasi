#include "common.h"

bool value_dump(struct Value *value)
{
    // TODO: Function pointer table here?
    switch(value->type) {
        case VALUETYPE_INT:
            printf("%d", value->intData);
            break;
        default:
            // TODO: Normal error.
            assert(0);
            return false;
    }
    return true;
}



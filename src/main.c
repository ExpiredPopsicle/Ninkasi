#include <stdio.h>

typedef int int32_t;

enum LispValueType
{
    LISP_INT,
    LISP_FLOAT,
    LISP_CONS
};

struct LispValue
{
    enum LispValueType type;

    union
    {
        int32_t intData;
        float floatData;

        struct
        {
            struct LispValue *car;
            struct LispValue *cdr;
        };
    };
};

int main(int argc, char *argv[])
{



    return 0;
}


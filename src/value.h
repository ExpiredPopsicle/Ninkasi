#ifndef VALUE_H
#define VALUE_H

enum ValueType
{
    VALUETYPE_INT,
};

struct Value
{
    enum ValueType type;

    union
    {
        int32_t intData;
    };
};

bool value_dump(struct Value *value);


#endif

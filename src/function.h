#ifndef FUNCTION_H
#define FUNCTION_H

struct VMFunctionCallbackData
{
};

typedef void (*VMFunctionCallback)(struct VMFunctionCallbackData *data);

struct VMFunction
{
    uint32_t argumentCount;
    uint32_t firstInstructionIndex;

    bool isCFunction;
    VMFunctionCallback CFunctionCallback;
};

#endif

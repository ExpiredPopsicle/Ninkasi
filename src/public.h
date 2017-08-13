#ifndef NINKASI_PUBLIC_H
#define NINKASI_PUBLIC_H

#include "nktypes.h"
#include "value.h"

struct NKVM;
struct NKVMFunctionCallbackData;
// typedef void (*VMFunctionCallback)(struct NKVMFunctionCallbackData *data);
struct NKValue;

// ----------------------------------------------------------------------
// Public VM interface

// ----------------------------------------------------------------------
// Public compiler interface

struct NKCompilerState;

// nkbool nkiCompilerCompileScriptFile(
//     struct NKCompilerState *cs,
//     const char *scriptFilename);

// ----------------------------------------------------------------------
// Public types

// struct NKVMFunctionCallbackData
// {
//     struct NKVM *vm;

//     struct NKValue *arguments;
//     nkuint32_t argumentCount;

//     // Set this to something to return a value.
//     struct NKValue returnValue;

//     void *userData;
// };

#endif


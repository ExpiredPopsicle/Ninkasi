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

// ----------------------------------------------------------------------
// Types

typedef nkbool (*NKFunctionStyleExpressionEmitter)(
    struct NKCompilerState *cs,
    nkuint32_t argumentCount);

struct NKFunctionStyleExpression
{
    const char *name;
    NKFunctionStyleExpressionEmitter emitter;
};

// ----------------------------------------------------------------------
// Statics

static nkbool nkiCompilerFSE_coroutineCreate(
    struct NKCompilerState *cs,
    nkuint32_t argumentCount)
{
    // // Pop all args off.
    // nkiCompilerEmitPushLiteralInt(cs, argumentCount, nkfalse);
    // nkiCompilerAddInstructionSimple(cs, NK_OP_POPN, nkfalse);
    cs->context->stackFrameOffset -= argumentCount;

    // FIXME: Check underflow.
    nkiCompilerEmitPushLiteralInt(cs, argumentCount - 1, nkfalse);

    nkiCompilerAddInstructionSimple(cs, NK_OP_COROUTINE_CREATE, nktrue);

    return nktrue;
}

static nkbool nkiCompilerFSE_coroutineYield(
    struct NKCompilerState *cs,
    nkuint32_t argumentCount)
{
    // Fill in a value to pass back if none was given.
    if(argumentCount == 0) {
        nkiCompilerEmitPushNil(cs, nktrue);
    }

    if(argumentCount > 1) {
        nkiCompilerAddError(cs, "Too many arguments to yield().");
        return nkfalse;
    }

    nkiCompilerAddInstructionSimple(
        cs, NK_OP_COROUTINE_YIELD, nktrue);

    return nktrue;
}

static nkbool nkiCompilerFSE_coroutineResume(
    struct NKCompilerState *cs,
    nkuint32_t argumentCount)
{
    if(argumentCount > 2) {
        nkiCompilerAddError(cs, "Too many arguments to resume().");
        return nkfalse;
    }

    if(argumentCount < 1) {
        nkiCompilerAddError(cs, "Too few arguments to resume().");
        return nkfalse;
    }

    // Fill in a value to pass in if none was given.
    if(argumentCount == 1) {
        nkiCompilerEmitPushNil(cs, nktrue);
    }

    nkiCompilerAddInstructionSimple(
        cs, NK_OP_COROUTINE_RESUME, nktrue);

    return nktrue;
}

static nkbool nkiCompilerFSE_createObject(
    struct NKCompilerState *cs,
    nkuint32_t argumentCount)
{
    // TODO: Either take a list of key-value pairs, or just a list of
    // stuff to use like an array. We can handle this in a later
    // version.
    if(argumentCount) {
        nkiCompilerAddError(cs, "Too many arguments given to object().");
        return nkfalse;
    }

    nkiCompilerAddInstructionSimple(cs, NK_OP_CREATEOBJECT, nktrue);

    return nktrue;
}

static nkbool nkiCompilerFSE_len(
    struct NKCompilerState *cs,
    nkuint32_t argumentCount)
{
    if(argumentCount != 1) {
        nkiCompilerAddError(cs, "Incorrect number of arguments to len().");
        return nkfalse;
    }

    nkiCompilerAddInstructionSimple(cs, NK_OP_LEN, nktrue);

    return nktrue;
}

static struct NKFunctionStyleExpression nkiFunctionStyleExpressionList[] = {
    { "coroutine", nkiCompilerFSE_coroutineCreate },
    { "yield",     nkiCompilerFSE_coroutineYield  },
    { "resume",    nkiCompilerFSE_coroutineResume },
    { "object",    nkiCompilerFSE_createObject    },
    { "len",       nkiCompilerFSE_len             },
};

const nkuint32_t nkiFunctionStyleExpressionCount =
    sizeof(nkiFunctionStyleExpressionList) / sizeof(struct NKFunctionStyleExpression);

nkbool nkiCompilerIsFunctionStyleExpressionName(
    struct NKVM *vm, const char *str)
{
    nkuint32_t i;
    for(i = 0; i < nkiFunctionStyleExpressionCount; i++) {
        if(!nkiStrcmp(str, nkiFunctionStyleExpressionList[i].name)) {
            return nktrue;
        }
    }

    return nkfalse;
}

nkbool nkiCompilerEmitFunctionStyleExpression(
    struct NKCompilerState *cs,
    const char *name, nkuint32_t argumentCount)
{
    nkuint32_t i;
    for(i = 0; i < nkiFunctionStyleExpressionCount; i++) {
        if(!nkiStrcmp(name, nkiFunctionStyleExpressionList[i].name)) {
            return nkiFunctionStyleExpressionList[i].emitter(cs, argumentCount);
        }
    }
    return nkfalse;
}



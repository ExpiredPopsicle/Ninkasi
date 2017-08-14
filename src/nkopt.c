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
//   By Cliff "ExpiredPopsicle" Jolly
//     https://expiredpopsicle.com
//     https://intoxicoding.com
//     expiredpopsicle@gmail.com
//
// ----------------------------------------------------------------------
//
//   Copyright (c) 2017 Clifford Jolly
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

nkbool nkiCompilerCanOptimizeOperationWithConstants(struct NKExpressionAstNode *node)
{
    if(node->opOrValue->type == NK_TOKENTYPE_PLUS ||
        node->opOrValue->type == NK_TOKENTYPE_MINUS ||
        node->opOrValue->type == NK_TOKENTYPE_MULTIPLY ||
        node->opOrValue->type == NK_TOKENTYPE_DIVIDE)
    {
        return nktrue;
    }
    return nkfalse;
}

nkbool nkiCompilerIsImmediateValue(struct NKExpressionAstNode *node)
{
    if(node->opOrValue->type == NK_TOKENTYPE_INTEGER ||
        node->opOrValue->type == NK_TOKENTYPE_FLOAT)
    {
        return nktrue;
    }
    return nkfalse;
}

struct NKExpressionAstNode *nkiCompilerMakeImmediateExpressionNode(
    struct NKVM *vm,
    enum NKTokenType type,
    nkuint32_t lineNumber)
{
    struct NKExpressionAstNode *newNode =
        nkiMalloc(vm, sizeof(struct NKExpressionAstNode));
    struct NKToken *newToken =
        nkiMalloc(vm, sizeof(struct NKToken));
    memset(newNode, 0, sizeof(*newNode));
    memset(newToken, 0, sizeof(*newToken));
    newNode->ownedToken = nktrue;
    newNode->opOrValue = newToken;
    newToken->type = type;
    newToken->lineNumber = lineNumber;
    return newNode;
}

#define NK_APPLY_MATH()                                             \
    do {                                                            \
        switch((*node)->opOrValue->type) {                          \
                                                                    \
            case NK_TOKENTYPE_PLUS:                                 \
                /* Addition. */                                     \
                val = c0Val + c1Val;                                \
                break;                                              \
                                                                    \
            case NK_TOKENTYPE_MINUS:                                \
                if(!(*node)->children[1]) {                         \
                    /* Unary negation. */                           \
                    val = -c0Val;                                   \
                } else {                                            \
                    /* Subtraction. */                              \
                    val = c0Val - c1Val;                            \
                }                                                   \
                break;                                              \
                                                                    \
            case NK_TOKENTYPE_MULTIPLY:                             \
                /* Multiplication. */                               \
                val = c0Val * c1Val;                                \
                break;                                              \
                                                                    \
            case NK_TOKENTYPE_DIVIDE:                               \
                /* Division. */                                     \
                if(c1Val == 0) {                                    \
                    nkiCompilerDeleteExpressionNode(vm, newNode);   \
                    /* TODO: Raise error. */                        \
                    return;                                         \
                }                                                   \
                val = c0Val / c1Val;                                \
                break;                                              \
                                                                    \
            default:                                                \
                /* If you hit this, you forgot to */                \
                /* implement something. */                          \
                assert(0);                                          \
                break;                                              \
        }                                                           \
    } while(0)

void nkiCompilerOptimizeConstants(
    struct NKVM *vm, struct NKExpressionAstNode **node)
{
    // TODO: Remove some no-ops like multiply-by-one, divide-by-one,
    // add zero, subtract zero, etc. We can do this even if we don't
    // have both sides of the equation fully simplified. NOTE: Do NOT
    // eliminate the other side of a multiply-by-zero, or you might
    // optimize-away a function call.

    // First recurse into children and optimize them. Maybe they'll
    // become immediate values we can work with.
    if((*node)->children[0]) {
        nkiCompilerOptimizeConstants(vm, &(*node)->children[0]);
    }
    if((*node)->children[1]) {
        nkiCompilerOptimizeConstants(vm, &(*node)->children[1]);
    }

    if(nkiCompilerCanOptimizeOperationWithConstants((*node))) {

        // Make sure we have two immediate values to work with.
        nkbool canOptimize = nktrue;
        if((*node)->children[0] && !nkiCompilerIsImmediateValue((*node)->children[0])) {
            canOptimize = nkfalse;
        } else if((*node)->children[1] && !nkiCompilerIsImmediateValue((*node)->children[1])) {
            canOptimize = nkfalse;
        }

        if(!(*node)->children[0]) {
            canOptimize = nkfalse;
        }

        if(canOptimize) {

            // Make a new literal value node with the same type as
            // whatever we have on the left.
            struct NKExpressionAstNode *newNode =
                nkiCompilerMakeImmediateExpressionNode(
                    vm,
                    (*node)->children[0]->opOrValue->type,
                    (*node)->children[0]->opOrValue->lineNumber);

            nkiDbgWriteLine("Optimizing operator: %s", (*node)->opOrValue->str);

            switch((*node)->children[0]->opOrValue->type) {

                case NK_TOKENTYPE_INTEGER: {

                    // Fetch original values.
                    nkint32_t c0Val = (*node)->children[0] ? atoi((*node)->children[0]->opOrValue->str) : 0;
                    nkint32_t c1Val = (*node)->children[1] ? atoi((*node)->children[1]->opOrValue->str) : 0;
                    nkint32_t val = 0;
                    char tmp[32];

                    // Do the actual operation.
                    NK_APPLY_MATH();

                    // TODO: Use sprintf_s.

                    // Set the string for the result.
                    sprintf(tmp, "%d", val);
                    newNode->opOrValue->str = nkiStrdup(vm, tmp);

                    // Replace the original node.
                    nkiCompilerDeleteExpressionNode(vm, *node);
                    *node = newNode;

                } break;

                case NK_TOKENTYPE_FLOAT: {

                    // Fetch original values.
                    float c0Val = (*node)->children[0] ? atof((*node)->children[0]->opOrValue->str) : 0;
                    float c1Val = (*node)->children[1] ? atof((*node)->children[1]->opOrValue->str) : 0;
                    float val = 0;
                    char tmp[32];

                    // Do the actual operation.
                    NK_APPLY_MATH();

                    // TODO: Use sprintf_s.

                    // Set the string for the result.
                    sprintf(tmp, "%f", val);
                    newNode->opOrValue->str = nkiStrdup(vm, tmp);

                    // Replace the original node.
                    nkiCompilerDeleteExpressionNode(vm, *node);
                    *node = newNode;

                } break;

                default:
                    break;
            }

        } else {

            nkiDbgWriteLine("NOT optimizing operator: %s\n", (*node)->opOrValue->str);

            nkiDbgWriteLine("  Child1: ");
            nkiCompilerDumpExpressionAstNode((*node)->children[0]);
            nkiDbgWriteLine("\n");

            nkiDbgWriteLine("  Child2: ");
            nkiCompilerDumpExpressionAstNode((*node)->children[1]);
            nkiDbgWriteLine("\n");

        }
    }
}



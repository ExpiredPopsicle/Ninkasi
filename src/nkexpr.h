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

#ifndef NINKASI_EXPRESSN_H
#define NINKASI_EXPRESSN_H

#include "nktypes.h"

struct NKToken;
struct NKVM;
struct NKCompilerState;

// ----------------------------------------------------------------------
// Infix Expression Parsing Utilities.

/// Returns true if the token represents a valid prefix operator
/// ("++", "--", "-", etc).
nkbool nkiCompilerIsPrefixOperator(struct NKToken *token);

/// Returns ture if the token represents a valid postfix operator
/// ("[]", "()", etc).
nkbool nkiCompilerIsPostfixOperator(struct NKToken *token);

/// Returns true if the token is a valid end-of-sub-expression token
/// ("]", ")", etc).
nkbool nkiCompilerIsSubexpressionEndingToken(struct NKToken *token);

/// Returns true if the token is a valid end-of-expression token
/// (anythin recognized by nkiCompilerIsSubexpressionEndingToken, plus
/// ";", etc).
nkbool nkiCompilerIsExpressionEndingToken(struct NKToken *token);

/// Get the operator precedence of the operator represented by some
/// token type.
nkint32_t nkiCompilerGetPrecedence(enum NKTokenType t);

// ----------------------------------------------------------------------
// AST Manipulation.

/// A single node inside the AST.
struct NKExpressionAstNode
{
    struct NKToken *opOrValue;
    struct NKExpressionAstNode *children[2];
    struct NKExpressionAstNode *stackNext;

    // Nktrue if the token was generated for this ExpressionAstNode, and
    // should be deleted with it.
    nkbool ownedToken;

    // Nktrue if this is the first node in a chain of function call
    // argument nodes. The left side of this (children[0]) will be the
    // function lookup, and the right side will be the first argument.
    // The argument's node will have an expression on the left for the
    // argument itself, and the next argument on the right, and so on
    // until the right side argument is NULL.
    nkbool isRootFunctionCallNode;
};

/// Make a single AST node.
struct NKExpressionAstNode *nkiCompilerMakeImmediateExpressionNode(
    struct NKVM *vm,
    enum NKTokenType type,
    nkuint32_t lineNumber);

/// Delete an entire AST, including all children.
void nkiCompilerDeleteExpressionNode(struct NKVM *vm, struct NKExpressionAstNode *node);

/// Just parse an expression tree starting from the current token and
/// return an AST. The form of the AST coming out of this is only a
/// raw representation of what was in the script, and needs some
/// transformations and optimization applied.
struct NKExpressionAstNode *nkiCompilerParseExpression(struct NKCompilerState *cs);

/// Parse an expression tree, optimize it, and apply some internal
/// operator conversions, like turning increment/decrement operations
/// into read/(add/subtract)/store operations. Does NOT emit
/// instructions.
struct NKExpressionAstNode *nkiCompilerCompileExpressionWithoutEmit(struct NKCompilerState *cs);

/// Emit instructions for a parsed and converted AST.
nkbool nkiCompilerEmitExpression(struct NKCompilerState *cs, struct NKExpressionAstNode *node);

/// Parse an expression tree, optimize it, apply needed operator
/// transformations, and emit instructions into the VM.
nkbool nkiCompilerCompileExpression(struct NKCompilerState *cs);

#endif // NINKASI_EXPRESSN_H


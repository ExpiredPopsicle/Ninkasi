#ifndef NINKASI_EXPRESSN_H
#define NINKASI_EXPRESSN_H

#include "nktypes.h"

struct NKToken;
struct NKVM;
struct NKCompilerState;

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

nkbool nkiCompilerIsPrefixOperator(struct NKToken *token);
nkbool nkiCompilerIsPostfixOperator(struct NKToken *token);
nkbool nkiCompilerIsExpressionEndingToken(struct NKToken *token);
nkbool nkiCompilerIsSubexpressionEndingToken(struct NKToken *token);
nkint32_t nkiCompilerGetPrecedence(enum NKTokenType t);

struct NKExpressionAstNode *nkiCompilerMakeImmediateExpressionNode(
    struct NKVM *vm,
    enum NKTokenType type,
    nkuint32_t lineNumber);

void nkiCompilerDeleteExpressionNode(struct NKVM *vm, struct NKExpressionAstNode *node);
void nkiCompilerDumpExpressionAstNode(struct NKExpressionAstNode *node);



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


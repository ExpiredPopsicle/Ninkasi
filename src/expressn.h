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

nkbool isPrefixOperator(struct NKToken *token);
nkbool isPostfixOperator(struct NKToken *token);
nkbool isExpressionEndingToken(struct NKToken *token);
nkbool isSubexpressionEndingToken(struct NKToken *token);
nkint32_t getPrecedence(enum NKTokenType t);

struct NKExpressionAstNode *makeImmediateExpressionNode(
    struct NKVM *vm,
    enum NKTokenType type,
    nkuint32_t lineNumber);

void deleteExpressionNode(struct NKVM *vm, struct NKExpressionAstNode *node);
void dumpExpressionAstNode(struct NKExpressionAstNode *node);

struct NKExpressionAstNode *parseExpression(struct NKCompilerState *cs);


nkbool nkiCompilerCompileExpression(struct NKCompilerState *cs);
struct NKExpressionAstNode *nkiCompilerCompileExpressionWithoutEmit(struct NKCompilerState *cs);
nkbool emitExpression(struct NKCompilerState *cs, struct NKExpressionAstNode *node);

#endif // NINKASI_EXPRESSN_H


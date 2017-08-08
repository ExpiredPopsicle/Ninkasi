#ifndef NINKASI_EXPRESSN_H
#define NINKASI_EXPRESSN_H

#include "basetype.h"

struct NKToken;
struct NKVM;
struct NKCompilerState;

struct NKExpressionAstNode
{
    struct NKToken *opOrValue;
    struct NKExpressionAstNode *children[2];
    struct NKExpressionAstNode *stackNext;

    // True if the token was generated for this ExpressionAstNode, and
    // should be deleted with it.
    bool ownedToken;

    // True if this is the first node in a chain of function call
    // argument nodes. The left side of this (children[0]) will be the
    // function lookup, and the right side will be the first argument.
    // The argument's node will have an expression on the left for the
    // argument itself, and the next argument on the right, and so on
    // until the right side argument is NULL.
    bool isRootFunctionCallNode;
};

bool isPrefixOperator(struct NKToken *token);
bool isPostfixOperator(struct NKToken *token);
bool isExpressionEndingToken(struct NKToken *token);
bool isSubexpressionEndingToken(struct NKToken *token);
int32_t getPrecedence(enum NKTokenType t);

struct NKExpressionAstNode *makeImmediateExpressionNode(
    struct NKVM *vm,
    enum NKTokenType type,
    uint32_t lineNumber);

void deleteExpressionNode(struct NKVM *vm, struct NKExpressionAstNode *node);
void dumpExpressionAstNode(struct NKExpressionAstNode *node);

struct NKExpressionAstNode *parseExpression(struct NKCompilerState *cs);


bool nkiCompilerCompileExpression(struct NKCompilerState *cs);
struct NKExpressionAstNode *nkiCompilerCompileExpressionWithoutEmit(struct NKCompilerState *cs);
bool emitExpression(struct NKCompilerState *cs, struct NKExpressionAstNode *node);

#endif // NINKASI_EXPRESSN_H


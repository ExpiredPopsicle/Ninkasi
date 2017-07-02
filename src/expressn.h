#ifndef EXPRESSN_H
#define EXPRESSN_H

#include "basetype.h"

struct Token;
struct VM;
struct CompilerState;

struct ExpressionAstNode
{
    struct Token *opOrValue;
    struct ExpressionAstNode *children[2];
    struct ExpressionAstNode *stackNext;

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

bool isPrefixOperator(struct Token *token);
bool isPostfixOperator(struct Token *token);
bool isExpressionEndingToken(struct Token *token);
bool isSubexpressionEndingToken(struct Token *token);
int32_t getPrecedence(enum TokenType t);

struct ExpressionAstNode *makeImmediateExpressionNode(
    enum TokenType type,
    uint32_t lineNumber);

void deleteExpressionNode(struct ExpressionAstNode *node);
void dumpExpressionAstNode(struct ExpressionAstNode *node);

struct ExpressionAstNode *parseExpression(struct VM *vm, struct Token **currentToken);


bool compileExpression(struct CompilerState *cs);


#endif

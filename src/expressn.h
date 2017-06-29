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


bool compileExpression(struct CompilerState *cs, struct Token **currentToken);


#endif

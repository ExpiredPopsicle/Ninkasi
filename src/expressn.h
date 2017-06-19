#ifndef EXPRESSN_H
#define EXPRESSN_H

enum TokenType;

struct ExpressionAstNode
{
    struct Token *opOrValue;
    struct ExpressionAstNode *children[2];
    struct ExpressionAstNode *stackNext;

    // True if the token was generated for this ExpressionAstNode, and
    // should be deleted with it.
    bool ownedToken;
};

struct ExpressionAstNode *makeImmediateExpressionNode(enum TokenType type);
void deleteExpressionNode(struct ExpressionAstNode *node);

#endif

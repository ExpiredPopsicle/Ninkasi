#include "common.h"

bool canOptimizeOperationWithConstants(struct ExpressionAstNode *node)
{
    if(node->opOrValue->type == TOKENTYPE_PLUS ||
        node->opOrValue->type == TOKENTYPE_MINUS ||
        node->opOrValue->type == TOKENTYPE_MULTIPLY ||
        node->opOrValue->type == TOKENTYPE_DIVIDE)
    {
        return true;
    }
    return false;
}

bool isImmediateValue(struct ExpressionAstNode *node)
{
    if(node->opOrValue->type == TOKENTYPE_INTEGER ||
        node->opOrValue->type == TOKENTYPE_FLOAT)
    {
        return true;
    }
    return false;
}

struct ExpressionAstNode *makeImmediateExpressionNode(enum TokenType type)
{
    struct ExpressionAstNode *newNode = malloc(sizeof(struct ExpressionAstNode));
    struct Token *newToken = malloc(sizeof(struct Token));
    memset(newNode, 0, sizeof(*newNode));
    memset(newToken, 0, sizeof(*newToken));
    newNode->ownedToken = true;
    newNode->opOrValue = newToken;
    newToken->type = type;
    return newNode;
}

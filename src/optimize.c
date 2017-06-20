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

#define APPLY_MATH()                                        \
    do {                                                    \
        switch((*node)->opOrValue->type) {                  \
                                                            \
            case TOKENTYPE_PLUS:                            \
                /* Addition. */                             \
                val = c0Val + c1Val;                        \
                break;                                      \
                                                            \
            case TOKENTYPE_MINUS:                           \
                if(!(*node)->children[1]) {                 \
                    /* Unary negation. */                   \
                    val = -c0Val;                           \
                } else {                                    \
                    /* Subtraction. */                      \
                    val = c0Val - c1Val;                    \
                }                                           \
                break;                                      \
                                                            \
            case TOKENTYPE_MULTIPLY:                        \
                /* Multiplication. */                       \
                val = c0Val * c1Val;                        \
                break;                                      \
                                                            \
            case TOKENTYPE_DIVIDE:                          \
                /* Division. */                             \
                if(c1Val == 0) {                            \
                    deleteExpressionNode(newNode);          \
                    /* TODO: Raise error. */                \
                    return;                                 \
                }                                           \
                val = c0Val / c1Val;                        \
                break;                                      \
                                                            \
            default:                                        \
                /* If you hit this, you forgot to */        \
                /* implement something. */                  \
                assert(0);                                  \
                break;                                      \
        }                                                   \
    } while(0)

void optimizeConstants(struct ExpressionAstNode **node)
{
    // First recurse into children and optimize them. Maybe they'll
    // become immediate values we can work with.
    if((*node)->children[0]) {
        optimizeConstants(&(*node)->children[0]);
    }
    if((*node)->children[1]) {
        optimizeConstants(&(*node)->children[1]);
    }

    if(canOptimizeOperationWithConstants((*node))) {

        // Make sure we have two immediate values to work with.
        bool canOptimize = true;
        if((*node)->children[0] && !isImmediateValue((*node)->children[0])) {
            canOptimize = false;
        } else if((*node)->children[1] && !isImmediateValue((*node)->children[1])) {
            canOptimize = false;
        }

        if(canOptimize) {

            // Make a new literal value node with the same type as
            // whatever we have on the left.
            struct ExpressionAstNode *newNode =
                makeImmediateExpressionNode((*node)->children[0]->opOrValue->type);

            printf("Optimizing operator: %s\n", (*node)->opOrValue->str);

            switch((*node)->children[0]->opOrValue->type) {

                case TOKENTYPE_INTEGER: {

                    // Fetch original values.
                    int32_t c0Val = (*node)->children[0] ? atoi((*node)->children[0]->opOrValue->str) : 0;
                    int32_t c1Val = (*node)->children[1] ? atoi((*node)->children[1]->opOrValue->str) : 0;
                    int32_t val = 0;
                    char tmp[32];

                    // Do the actual operation.
                    APPLY_MATH();

                    // TODO: Use sprintf_s.

                    // Set the string for the result.
                    sprintf(tmp, "%d", val);
                    newNode->opOrValue->str = strdup(tmp);

                    // Replace the original node.
                    deleteExpressionNode(*node);
                    *node = newNode;

                } break;

                case TOKENTYPE_FLOAT: {

                    // Fetch original values.
                    float c0Val = (*node)->children[0] ? atof((*node)->children[0]->opOrValue->str) : 0;
                    float c1Val = (*node)->children[1] ? atof((*node)->children[1]->opOrValue->str) : 0;
                    float val = 0;
                    char tmp[32];

                    // Do the actual operation.
                    APPLY_MATH();

                    // TODO: Use sprintf_s.

                    // Set the string for the result.
                    sprintf(tmp, "%f", val);
                    newNode->opOrValue->str = strdup(tmp);

                    // Replace the original node.
                    deleteExpressionNode(*node);
                    *node = newNode;

                } break;

                default:
                    break;
            }

        } else {

            printf("NOT optimizing operator: %s\n", (*node)->opOrValue->str);

            printf("  Child1: ");
            dumpExpressionAstNode((*node)->children[0]);
            printf("\n");

            printf("  Child2: ");
            dumpExpressionAstNode((*node)->children[1]);
            printf("\n");

        }
    }
}



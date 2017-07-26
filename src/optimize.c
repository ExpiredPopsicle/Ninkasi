#include "common.h"

bool canOptimizeOperationWithConstants(struct NKExpressionAstNode *node)
{
    if(node->opOrValue->type == NK_TOKENTYPE_PLUS ||
        node->opOrValue->type == NK_TOKENTYPE_MINUS ||
        node->opOrValue->type == NK_TOKENTYPE_MULTIPLY ||
        node->opOrValue->type == NK_TOKENTYPE_DIVIDE)
    {
        return true;
    }
    return false;
}

bool isImmediateValue(struct NKExpressionAstNode *node)
{
    if(node->opOrValue->type == NK_TOKENTYPE_INTEGER ||
        node->opOrValue->type == NK_TOKENTYPE_FLOAT)
    {
        return true;
    }
    return false;
}

struct NKExpressionAstNode *makeImmediateExpressionNode(
    struct VM *vm,
    enum NKTokenType type,
    uint32_t lineNumber)
{
    struct NKExpressionAstNode *newNode =
        nkiMalloc(vm, sizeof(struct NKExpressionAstNode));
    struct NKToken *newToken =
        nkiMalloc(vm, sizeof(struct NKToken));
    memset(newNode, 0, sizeof(*newNode));
    memset(newToken, 0, sizeof(*newToken));
    newNode->ownedToken = true;
    newNode->opOrValue = newToken;
    newToken->type = type;
    newToken->lineNumber = lineNumber;
    return newNode;
}

#define APPLY_MATH()                                    \
    do {                                                \
        switch((*node)->opOrValue->type) {              \
                                                        \
            case NK_TOKENTYPE_PLUS:                        \
                /* Addition. */                         \
                val = c0Val + c1Val;                    \
                break;                                  \
                                                        \
            case NK_TOKENTYPE_MINUS:                       \
                if(!(*node)->children[1]) {             \
                    /* Unary negation. */               \
                    val = -c0Val;                       \
                } else {                                \
                    /* Subtraction. */                  \
                    val = c0Val - c1Val;                \
                }                                       \
                break;                                  \
                                                        \
            case NK_TOKENTYPE_MULTIPLY:                    \
                /* Multiplication. */                   \
                val = c0Val * c1Val;                    \
                break;                                  \
                                                        \
            case NK_TOKENTYPE_DIVIDE:                      \
                /* Division. */                         \
                if(c1Val == 0) {                        \
                    deleteExpressionNode(vm, newNode);  \
                    /* TODO: Raise error. */            \
                    return;                             \
                }                                       \
                val = c0Val / c1Val;                    \
                break;                                  \
                                                        \
            default:                                    \
                /* If you hit this, you forgot to */    \
                /* implement something. */              \
                assert(0);                              \
                break;                                  \
        }                                               \
    } while(0)

void optimizeConstants(struct VM *vm, struct NKExpressionAstNode **node)
{
    // TODO: Remove some no-ops like multiply-by-one, divide-by-one,
    // add zero, subtract zero, etc. We can do this even if we don't
    // have both sides of the equation fully simplified. NOTE: Do NOT
    // eliminate the other side of a multiply-by-zero, or you might
    // optimize-away a function call.

    // First recurse into children and optimize them. Maybe they'll
    // become immediate values we can work with.
    if((*node)->children[0]) {
        optimizeConstants(vm, &(*node)->children[0]);
    }
    if((*node)->children[1]) {
        optimizeConstants(vm, &(*node)->children[1]);
    }

    if(canOptimizeOperationWithConstants((*node))) {

        // Make sure we have two immediate values to work with.
        bool canOptimize = true;
        if((*node)->children[0] && !isImmediateValue((*node)->children[0])) {
            canOptimize = false;
        } else if((*node)->children[1] && !isImmediateValue((*node)->children[1])) {
            canOptimize = false;
        }

        if(!(*node)->children[0]) {
            canOptimize = false;
        }

        if(canOptimize) {

            // Make a new literal value node with the same type as
            // whatever we have on the left.
            struct NKExpressionAstNode *newNode =
                makeImmediateExpressionNode(
                    vm,
                    (*node)->children[0]->opOrValue->type,
                    (*node)->children[0]->opOrValue->lineNumber);

            dbgWriteLine("Optimizing operator: %s", (*node)->opOrValue->str);

            switch((*node)->children[0]->opOrValue->type) {

                case NK_TOKENTYPE_INTEGER: {

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
                    newNode->opOrValue->str = nkiStrdup(vm, tmp);

                    // Replace the original node.
                    deleteExpressionNode(vm, *node);
                    *node = newNode;

                } break;

                case NK_TOKENTYPE_FLOAT: {

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
                    newNode->opOrValue->str = nkiStrdup(vm, tmp);

                    // Replace the original node.
                    deleteExpressionNode(vm, *node);
                    *node = newNode;

                } break;

                default:
                    break;
            }

        } else {

            dbgWriteLine("NOT optimizing operator: %s\n", (*node)->opOrValue->str);

            dbgWriteLine("  Child1: ");
            dumpExpressionAstNode((*node)->children[0]);
            dbgWriteLine("\n");

            dbgWriteLine("  Child2: ");
            dumpExpressionAstNode((*node)->children[1]);
            dbgWriteLine("\n");

        }
    }
}



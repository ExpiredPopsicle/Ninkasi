#include "common.h"


// void *wrapMalloc(uint32_t size)
// {
//     void *v = malloc(size);
//     printf("Malloc: %p\n", v);
//     return v;
// }
// void wrapFree(void *v)
// {
//     printf("Free:   %p\n", v);
//     free(v);
// }
// #define malloc(x) wrapMalloc(x)
// #define free(x) wrapFree(x)



void deleteExpressionNode(struct ExpressionAstNode *node)
{
    if(node->ownedToken) {
        deleteToken(node->opOrValue);
        node->opOrValue = NULL;
    }

    if(node->children[0]) {
        deleteExpressionNode(node->children[0]);
    }
    if(node->children[1]) {
        deleteExpressionNode(node->children[1]);
    }

    free(node);
}

void dumpExpressionAstNode(struct ExpressionAstNode *node)
{
    if(!node) {
        printf("<NULL>");
        return;
    }

    if(isPrefixOperator(node->opOrValue) && !node->children[1]) {

        printf(node->opOrValue->str);
        printf("(");
        dumpExpressionAstNode(node->children[0]);
        printf(")");

    } else if(node->opOrValue->type == TOKENTYPE_BRACKET_OPEN) {

        printf("(");
        dumpExpressionAstNode(node->children[0]);
        printf(")[");
        dumpExpressionAstNode(node->children[1]);
        printf("]");

    } else {

        printf("(");
        if(node->children[0]) {
            printf("(");
            dumpExpressionAstNode(node->children[0]);
            printf(")");
        }

        printf("%s", node->opOrValue->str);

        if(node->children[1]) {
            printf("(");
            dumpExpressionAstNode(node->children[1]);
            printf(")");
        }
        printf(")");

    }
}


bool isSubexpressionEndingToken(struct Token *token)
{
    return !token ||
        token->type == TOKENTYPE_PAREN_CLOSE ||
        token->type == TOKENTYPE_BRACKET_CLOSE;
}

bool isExpressionEndingToken(struct Token *token)
{
    return !token || isSubexpressionEndingToken(token);
}

bool isPrefixOperator(struct Token *token)
{
    return token && (
        token->type == TOKENTYPE_MINUS);
}

bool isPostfixOperator(struct Token *token)
{
    return token && (
        token->type == TOKENTYPE_BRACKET_OPEN);
}

bool getPrecedence(enum TokenType t)
{
    // Reference:
    // http://en.cppreference.com/w/cpp/language/operator_precedence

    switch(t) {
        case TOKENTYPE_MULTIPLY:
        case TOKENTYPE_DIVIDE:
            return 5;
        case TOKENTYPE_PLUS:
        case TOKENTYPE_MINUS:
            return 6;
        default:
            // TODO: Remove this. If this case is hit, then you forgot
            // to implement some operator.
            assert(0);
    }

    return 17;
}


// TODO: Add line number.
// TODO: Add descriptive token type name.
// TODO: Add to error state instead of printing.
#define EXPECT_AND_SKIP(x)                                      \
    do {                                                        \
        if(!(*currentToken) || (*currentToken)->type != x) {    \
            dbgWriteLine("Error: Expected token type %d", x);   \
            return NULL;                                        \
        }                                                       \
        (*currentToken) = (*currentToken)->next;                \
    } while(0)

#define NEXT_TOKEN()                                \
    do {                                            \
        (*currentToken) = (*currentToken)->next;    \
    } while(0)

#define MAKE_OP(x)                                                      \
    struct ExpressionAstNode *astNode = malloc(sizeof(struct ExpressionAstNode)); \
    memset(astNode, 0, sizeof(*astNode));                               \
    astNode->stackNext = opStack;                                       \
    astNode->children[0] = NULL;                                        \
    astNode->children[1] = NULL;                                        \
    astNode->opOrValue = x;

#define PUSH_OP(x)                              \
    do {                                        \
        MAKE_OP(x);                             \
        opStack = astNode;                      \
    } while(0)

void reduce(
    struct ExpressionAstNode **opStack,
    struct ExpressionAstNode **valueStack)
{
    struct ExpressionAstNode *valNode1 = (*valueStack)->stackNext;
    struct ExpressionAstNode *valNode2 = (*valueStack);
    struct ExpressionAstNode *opNode   = (*opStack);

    assert(valNode1 && valNode2 && opNode);

    // Pop off the values.
    (*valueStack) = (*valueStack)->stackNext->stackNext;
    valNode1->stackNext = NULL;
    valNode2->stackNext = NULL;
    (*opStack) = opNode->stackNext;
    opNode->stackNext = NULL;

    // Assign children and push the operation onto the
    // value stack.
    opNode->children[0] = valNode1;
    opNode->children[1] = valNode2;
    opNode->stackNext = (*valueStack);
    (*valueStack) = opNode;
}

struct ExpressionAstNode *parseExpression(struct Token **currentToken)
{
    struct ExpressionAstNode *opStack = NULL;
    struct ExpressionAstNode *valueStack = NULL;

    while(!isExpressionEndingToken(*currentToken)) {

        struct ExpressionAstNode *lastPrefixOp   = NULL;
        struct ExpressionAstNode *firstPrefixOp  = NULL;
        struct ExpressionAstNode *lastPostfixOp  = NULL;
        struct ExpressionAstNode *firstPostfixOp = NULL;
        struct ExpressionAstNode *valueNode      = NULL;

        // Deal with prefix operators. Build up a list of them.
        while(isPrefixOperator(*currentToken)) {

            struct ExpressionAstNode *prefixNode = malloc(sizeof(struct ExpressionAstNode));
            memset(prefixNode, 0, sizeof(*prefixNode));

            // Add it to the end of our list of prefix operations.
            prefixNode->opOrValue = *currentToken;
            if(lastPrefixOp) {
                lastPrefixOp->children[0] = prefixNode;
            }
            lastPrefixOp = prefixNode;

            // Set it as the first if there's nothing else there.
            if(!firstPrefixOp) {
                firstPrefixOp = prefixNode;
            }

            dbgWriteLine("Parse prefix operator: %s", (*currentToken)->str);

            NEXT_TOKEN();
        }

        // Parse a value or sub-expression.
        if((*currentToken)->type == TOKENTYPE_PAREN_OPEN) {

            // Parse sub-expression.
            dbgWriteLine("Parse sub-expression.");
            dbgPush();
            NEXT_TOKEN();
            valueNode = parseExpression(currentToken);
            dbgPop();

            // Error check.
            if(!valueNode) {
                dbgWriteLine("Error: Subexpression parse failure.");
                return NULL;
            }

            // Make sure we ended on a closing parenthesis.
            if(!(*currentToken) || !isSubexpressionEndingToken(*currentToken)) {
                // TODO: Raise error flag.
                dbgWriteLine("Error: Bad expression end.");
                return NULL;
            }

            dbgWriteLine("Back from sub-expression. Current token: %s", (*currentToken)->str);

            // Skip expression-ending token.
            NEXT_TOKEN();

        } else {

            // Parse the actual value.
            if(!isExpressionEndingToken(*currentToken)) {

                valueNode = malloc(sizeof(struct ExpressionAstNode));
                memset(valueNode, 0, sizeof(*valueNode));
                valueNode->opOrValue = *currentToken;

                dbgWriteLine("Parse value: %s", (*currentToken)->str);
                NEXT_TOKEN();

            } else {
                // Prefix operator with no value. Why?
                // TODO: Raise error flag.
                dbgWriteLine("Error: Prefix with no value.");
                return NULL;
            }
        }

        // Deal with postfix operators.
        while(isPostfixOperator(*currentToken)) {

            struct ExpressionAstNode *postfixNode = malloc(sizeof(struct ExpressionAstNode));
            memset(postfixNode, 0, sizeof(*postfixNode));

            // Add it to the end of our list of postfix operations.
            postfixNode->opOrValue = *currentToken;
            if(lastPostfixOp) {
                postfixNode->children[0] = lastPostfixOp;
            }
            lastPostfixOp = postfixNode;

            // Set it as the first if there's nothing else there.
            if(!firstPostfixOp) {
                firstPostfixOp = postfixNode;
            }

            dbgWriteLine("Parse postfix operator: %s", (*currentToken)->str);

            // Handle index-into operator.
            if((*currentToken)->type == TOKENTYPE_BRACKET_OPEN) {

                dbgWriteLine("Handling index-into operator.");
                dbgPush();
                NEXT_TOKEN();
                postfixNode->children[1] = parseExpression(currentToken);
                dbgPop();

                // Error-check.
                if(!postfixNode->children[1]) {
                    dbgWriteLine("Error: Subexpression parse failure.");
                    return NULL;
                }

                EXPECT_AND_SKIP(TOKENTYPE_BRACKET_CLOSE);

                dbgWriteLine("Index-into operator complete.");

            } else {

                // TODO: Raise error flag.
                dbgWriteLine(
                    "Error: Unknown postfix operator (not implemented?): %s",
                    (*currentToken)->str);
                return NULL;

            }
        }

        // Attach prefix and postfix operations.
        if(firstPostfixOp) {
            assert(firstPostfixOp->children[0] == NULL);
            firstPostfixOp->children[0] = valueNode;
            valueNode = lastPostfixOp;
        }
        if(lastPrefixOp) {
            assert(lastPrefixOp->children[0] == NULL);
            lastPrefixOp->children[0] = valueNode;
            valueNode = firstPrefixOp;
        }

        // Push this value.
        valueNode->stackNext = valueStack;
        valueStack = valueNode;

        // Check to see if we're just done or not.
        if(isExpressionEndingToken(*currentToken)) {
            dbgWriteLine("Done parsing expression and maybe statement.");
            break;

        }

        // Not done yet. Parse the next operator.
        dbgWriteLine("Parse operator: %s", (*currentToken)->str);

        // Attempt to reduce.
        if(opStack) {

            if(getPrecedence((*currentToken)->type) > getPrecedence(opStack->opOrValue->type)) {

                reduce(&opStack, &valueStack);
                dbgWriteLine("Reduced!");

            } else {
                dbgWriteLine(
                    "We should NOT reduce! %s <= %s",
                    (*currentToken)->str, opStack->opOrValue->str);
            }
        }

        PUSH_OP(*currentToken);

        NEXT_TOKEN();
    }

    // Reduce all remaining operations.
    while(opStack) {
        reduce(&opStack, &valueStack);
        dbgWriteLine("Reduced at end!");
    }

    // Check that stacks are empty.
    if(opStack || valueStack->stackNext) {
        // TODO: Maybe make this a normal error, but be able to clean
        // up afterwards?
        assert(0);
    }

    printf("remaining values...\n");
    {
        struct ExpressionAstNode *n = valueStack;
        while(n) {
            dumpExpressionAstNode(n);
            n = n->stackNext;
            printf("\n");
        }
    }

    printf("remaining ops...\n");
    {
        struct ExpressionAstNode *n = opStack;
        while(n) {
            dumpExpressionAstNode(n);
            n = n->stackNext;
            printf("\n");
        }
    }

    dbgWriteLine("Expression parse success.");
    return valueStack;
}

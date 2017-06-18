#include "common.h"

// ----------------------------------------------------------------------

enum TokenType
{
    TOKENTYPE_INTEGER,
    TOKENTYPE_FLOAT,
    TOKENTYPE_PLUS,
    TOKENTYPE_MINUS,
    TOKENTYPE_MULTIPLY,
    TOKENTYPE_DIVIDE,
    TOKENTYPE_INCREMENT,
    TOKENTYPE_PAREN_OPEN,
    TOKENTYPE_PAREN_CLOSE,
    TOKENTYPE_BRACKET_OPEN,
    TOKENTYPE_BRACKET_CLOSE,

    TOKENTYPE_INVALID
};

struct Token
{
    enum TokenType type;
    char *str;
    struct Token *next;
};

bool isWhitespace(char c)
{
    if(c == ' ' || c == '\n' || c == '\t' || c == '\r') {
        return true;
    }
    return false;
}

bool isNumber(char c)
{
    return (c >= '0' && c <= '9');
}

struct TokenList
{
    struct Token *first;
    struct Token *last;
};

void destroyTokenList(struct TokenList *tokenList)
{
    struct Token *t = tokenList->first;
    while(t) {
        struct Token *next = t->next;
        free(t->str);
        free(t);
        t = next;
    }
    tokenList->first = NULL;
    tokenList->last = NULL;
}

void addToken(
    enum TokenType type,
    const char *str,
    struct TokenList *tokenList)
{
    struct Token *newToken = malloc(sizeof(struct Token));
    newToken->next = NULL;
    newToken->str = strdup(str);
    newToken->type = type;

    // Add us to the end of the list.
    if(tokenList->last) {
        tokenList->last->next = newToken;
    }
    tokenList->last = newToken;

    // This might be our first token.
    if(!tokenList->first) {
        tokenList->first = newToken;
    }

    printf("Token (%d): %s\n", type, str);
}

bool tokenize(const char *str, struct TokenList *tokenList)
{
    uint32_t len = strlen(str);
    uint32_t i = 0;

    while(i < len) {

        // Skip whitespace.
        while(i < len && isWhitespace(str[i])) {
            i++;
        }

        if(str[i] == '(') {

            addToken(TOKENTYPE_PAREN_OPEN, "(", tokenList);

        } else if(str[i] == ')') {

            addToken(TOKENTYPE_PAREN_CLOSE, ")", tokenList);

        } else if(str[i] == '[') {

            addToken(TOKENTYPE_BRACKET_OPEN, "[", tokenList);

        } else if(str[i] == ']') {

            addToken(TOKENTYPE_BRACKET_CLOSE, "]", tokenList);

        } else if(str[i] == '+') {

            // Check for "++".
            if(str[i+1] == '+') {
                addToken(TOKENTYPE_INCREMENT, "++", tokenList);
                i++;
            } else {
                addToken(TOKENTYPE_PLUS, "+", tokenList);
            }

        } else if(str[i] == '-') {

            addToken(TOKENTYPE_MINUS, "-", tokenList);

        } else if(str[i] == '*') {

            addToken(TOKENTYPE_MULTIPLY, "*", tokenList);

        } else if(str[i] == '/') {

            addToken(TOKENTYPE_MULTIPLY, "/", tokenList);

        } else if(isNumber(str[i])) {

            // Scan until we find the end of the number.
            uint32_t numberStart = i;
            bool isFloat = false;
            char tmp[256];

            while(i < len &&
                (isNumber(str[i]) || str[i] == '.') &&
                (i - numberStart) < sizeof(tmp) - 1)
            {
                tmp[i - numberStart] = str[i];
                if(str[i] == '.') {
                    isFloat = true;
                }
                i++;
            }

            // Add null terminator.
            tmp[i - numberStart] = 0;

            // Back up one character, so we don't eat the first
            // character of whatever comes next.
            i--;

            addToken(
                isFloat ? TOKENTYPE_FLOAT : TOKENTYPE_INTEGER,
                tmp,
                tokenList);

        } else {

            // TODO: Set error message.
            return false;

        }

        i++;
    }

    return true;
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

// enum TokenType getExpectedEndTokenType(struct Token *tok)
// {
//     switch(tok->type) {
//         case TOKENTYPE_BRACKET_OPEN:
//             return TOKENTYPE_BRACKET_CLOSE;
//         case TOKENTYPE_PAREN_OPEN:
//             return TOKENTYPE_PAREN_CLOSE;
//         default:
//             return ~0;
//     }
// }

int32_t dbgIndentLevel = 0;
int dbgWriteLine(const char *fmt, ...)
{
    va_list args;
    int ret;
    va_start(args, fmt);
    {
        int32_t i;
        for(i = 0; i < dbgIndentLevel; i++) {
            printf("  ");
        }
        ret = vprintf(fmt, args);
        printf("\n");
    }
    va_end(args);
    return ret;
}

void dbgPush(void)
{
    dbgIndentLevel++;
}

void dbgPop(void)
{
    dbgIndentLevel--;
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


struct ExpressionAstNode
{
    struct Token *opOrValue;
    struct ExpressionAstNode *children[2];
    struct ExpressionAstNode *stackNext;
};

#define MAKE_OP(x)                                                      \
    struct ExpressionAstNode *astNode = malloc(sizeof(struct ExpressionAstNode)); \
    astNode->stackNext = opStack;                                       \
    astNode->children[0] = NULL;                                        \
    astNode->children[1] = NULL;                                        \
    astNode->opOrValue = x;

#define PUSH_OP(x)                              \
    do {                                        \
        MAKE_OP(x);                             \
        opStack = astNode;                      \
    } while(0)

#define PUSH_VAL(x)                             \
    do {                                        \
        MAKE_OP(x);                             \
        valueStack = astNode;                   \
    } while(0)


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

    } else {

        if(node->children[0]) {
            dumpExpressionAstNode(node->children[0]);
        } else {
            // printf(" NULL1");
        }

        printf("%s", node->opOrValue->str);

        if(node->children[1]) {
            dumpExpressionAstNode(node->children[1]);
        } else {
            // printf(" NULL2");
        }

    }
}

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
            valueNode = firstPostfixOp;
        }
        if(lastPrefixOp) {
            assert(lastPrefixOp->children[0] == NULL);
            lastPrefixOp->children[0] = valueNode;
            valueNode = firstPrefixOp;
        }

        // Push this value.
        valueNode->stackNext = valueStack;
        valueStack = valueNode;




        // (Maybe) parse an operator.

        // Check to see if we're just done or not.
        if(isExpressionEndingToken(*currentToken)) {
            dbgWriteLine("Done parsing expression and maybe statement.");
            break;

        }

        // Not done yet. Parse the operator.
        dbgWriteLine("Parse operator: %s", (*currentToken)->str);

        // Attempt to reduce.
        if(opStack) {

            if(getPrecedence((*currentToken)->type) > getPrecedence(opStack->opOrValue->type)) {

                reduce(&opStack, &valueStack);
                dbgWriteLine(
                    "Reduced! %s > %s",
                    (*currentToken)->str, opStack->opOrValue->str);

            } else {
                dbgWriteLine(
                    "We should NOT reduce! %s <= %s",
                    (*currentToken)->str, opStack->opOrValue->str);
            }
        }

        PUSH_OP(*currentToken);

        NEXT_TOKEN();

        // TODO: Shift or reduce depending on last operator.
    }


    // TODO: Reduce all remaining operations?

    // TODO: Check that stacks are empty.


    while(opStack) {
        reduce(&opStack, &valueStack);
        dbgWriteLine("Reduced at end!");
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
    return NULL;
}







int main(int argc, char *argv[])
{
    struct VMStack stack;
    vmstack_init(&stack);

    // struct Value *t1 = vmstack_push(&stack);
    // struct Value *t2 = vmstack_push(&stack);

    // t1->type = VALUETYPE_INT;
    // t2->type = VALUETYPE_INT;
    // t1->intData = 123;
    // t2->intData = 456;

    vmstack_pushInt(&stack, 123);
    vmstack_pushInt(&stack, 456);

    opcode_add(&stack);

    vmstack_pushInt(&stack, 123);
    vmstack_pushInt(&stack, 456);

    vmstack_pushInt(&stack, 123);
    vmstack_pushInt(&stack, 456);

    vmstack_dump(&stack);

    vmstack_destroy(&stack);





    printf("Tokenize test...\n");
    struct TokenList tokenList;
    tokenList.first = NULL;
    tokenList.last = NULL;
    {
        // bool r = tokenize("(((123 + 456)[1 + 2][3])) * 789++ - -100 / ------300", &tokenList);
        bool r = tokenize("123 + 456 * 789 - -100 / ------300", &tokenList);
        // bool r = tokenize("123 + 456", &tokenList);
        assert(r);
    }
    {
        struct Token *tokenPtr = tokenList.first;
        parseExpression(&tokenPtr);
    }
    destroyTokenList(&tokenList);


    return 0;
}


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
        if(!currentToken || currentToken->type != x) {          \
            dbgWriteLine("Error: Expected token type %d", x); \
            return NULL;                                        \
        }                                                       \
        currentToken = currentToken->next;                      \
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




struct Token *parseExpression(struct Token *currentToken)
{
    while(!isExpressionEndingToken(currentToken)) {

        // Deal with prefix operators.
        while(isPrefixOperator(currentToken)) {
            dbgWriteLine("Parse prefix operator: %s", currentToken->str);
            currentToken = currentToken->next;
        }

        // Parse a value or sub-expression.
        if(currentToken->type == TOKENTYPE_PAREN_OPEN) {

            // Parse sub-expression.
            dbgWriteLine("Parse sub-expression.");
            dbgPush();
            currentToken = parseExpression(currentToken->next);
            dbgPop();

            // Make sure we ended on a closing parenthesis.
            if(!currentToken || !isSubexpressionEndingToken(currentToken)) {
                // TODO: Raise error flag.
                dbgWriteLine("Error: Bad expression end.");
                return NULL;
            }

            dbgWriteLine("Back from sub-expression. Current token: %s", currentToken->str);

            // Skip expression-ending token.
            currentToken = currentToken->next;

        } else {

            // Parse the actual value.
            if(!isExpressionEndingToken(currentToken)) {
                dbgWriteLine("Parse value: %s", currentToken->str);
                currentToken = currentToken->next;
            } else {
                // Prefix operator with no value. Why?
                // TODO: Raise error flag.
                dbgWriteLine("Error: Prefix with no value.");
                return NULL;
            }
        }

        // Deal with postfix operators.
        while(isPostfixOperator(currentToken)) {
            dbgWriteLine("Parse postfix operator: %s", currentToken->str);

            // Handle index-into operator.
            if(currentToken->type == TOKENTYPE_BRACKET_OPEN) {

                dbgWriteLine("Handling index-into operator.");
                dbgPush();
                currentToken = parseExpression(currentToken->next);
                dbgPop();

                EXPECT_AND_SKIP(TOKENTYPE_BRACKET_CLOSE);

                dbgWriteLine("Index-into operator complete.");

            } else {

                // TODO: Raise error flag.
                dbgWriteLine(
                    "Error: Unknown postfix operator (not implemented?): %s",
                    currentToken->str);
                return NULL;

            }
        }


        // TODO: Shift or reduce depending on last operator.



        // (Maybe) parse an operator.

        // Check to see if we're just done or not.
        if(isExpressionEndingToken(currentToken)) {

            dbgWriteLine("Done parsing expression and maybe statement.");
            break;

        }

        // Not done yet. Parse the operator.
        dbgWriteLine("Parse operator: %s", currentToken->str);
        currentToken = currentToken->next;
    }

    dbgWriteLine("Expression parse success.");
    return currentToken;
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
        bool r = tokenize("(((123 + 456)[1 + 2][3])) * 789++ - -100 / ------300", &tokenList);
        assert(r);
    }
    parseExpression(tokenList.first);
    destroyTokenList(&tokenList);


    return 0;
}


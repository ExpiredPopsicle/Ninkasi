#include "common.h"

static bool isWhitespace(char c)
{
    if(c == ' ' || c == '\n' || c == '\t' || c == '\r') {
        return true;
    }
    return false;
}

static bool isNumber(char c)
{
    return (c >= '0' && c <= '9');
}

void deleteToken(struct Token *token)
{
    free(token->str);
    free(token);
}

void destroyTokenList(struct TokenList *tokenList)
{
    struct Token *t = tokenList->first;
    while(t) {
        struct Token *next = t->next;
        deleteToken(t);
        t = next;
    }
    tokenList->first = NULL;
    tokenList->last = NULL;
}

void addToken(
    enum TokenType type,
    const char *str,
    int32_t lineNumber,
    struct TokenList *tokenList)
{
    struct Token *newToken = malloc(sizeof(struct Token));
    newToken->next = NULL;
    newToken->str = strdup(str);
    newToken->type = type;
    newToken->lineNumber = lineNumber;

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

bool tokenize(struct VM *vm, const char *str, struct TokenList *tokenList)
{
    uint32_t len = strlen(str);
    uint32_t i = 0;
    int32_t lineNumber = 1;

    while(i < len) {

        // Skip whitespace.
        while(i < len && isWhitespace(str[i])) {
            if(str[i] == '\n') {
                lineNumber++;
            }
            i++;
        }

        if(str[i] == '(') {

            addToken(TOKENTYPE_PAREN_OPEN, "(", lineNumber, tokenList);

        } else if(str[i] == ')') {

            addToken(TOKENTYPE_PAREN_CLOSE, ")", lineNumber, tokenList);

        } else if(str[i] == '[') {

            addToken(TOKENTYPE_BRACKET_OPEN, "[", lineNumber, tokenList);

        } else if(str[i] == ']') {

            addToken(TOKENTYPE_BRACKET_CLOSE, "]", lineNumber, tokenList);

        } else if(str[i] == '+') {

            // Check for "++".
            if(str[i+1] == '+') {
                addToken(TOKENTYPE_INCREMENT, "++", lineNumber, tokenList);
                i++;
            } else {
                addToken(TOKENTYPE_PLUS, "+", lineNumber, tokenList);
            }

        } else if(str[i] == '-') {

            addToken(TOKENTYPE_MINUS, "-", lineNumber, tokenList);

        } else if(str[i] == '*') {

            addToken(TOKENTYPE_MULTIPLY, "*", lineNumber, tokenList);

        } else if(str[i] == '/') {

            addToken(TOKENTYPE_DIVIDE, "/", lineNumber, tokenList);

        } else if(str[i] == '\"') {

            const char *strStart = &str[i+1];
            const char *strEnd   = &str[i+1];
            char *strTmp         = NULL;
            uint32_t len         = 0;

            while(*strEnd != 0 && *strEnd != '\"') {

                if(*strEnd == '\n') {
                    errorStateAddError(
                        &vm->errorState,
                        lineNumber, "Newline inside of quoted string.");
                    return false;
                }

                strEnd++;
            }

            len = (strEnd - strStart);
            strTmp = malloc(len + 1);
            memcpy(strTmp, strStart, len);
            strTmp[len] = 0;

            addToken(TOKENTYPE_STRING, strTmp, lineNumber, tokenList);

            free(strTmp);

            // Skip the whole string and end quote.
            i += len + 1;

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
                lineNumber,
                tokenList);

        } else {

            errorStateAddError(
                &vm->errorState,
                lineNumber, "Unknown token.");
            return false;

        }

        i++;
    }

    return true;
}

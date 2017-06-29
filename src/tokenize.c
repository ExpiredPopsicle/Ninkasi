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

char *tokenizerUnescapeString(const char *in)
{
    char *out = malloc(strlen(in) + 1);
    uint32_t len = strlen(in);
    uint32_t readIndex;
    uint32_t writeIndex = 0;

    for(readIndex = 0; readIndex < len; readIndex++) {

        if(in[readIndex] == '\\') {

            readIndex++;

            // TODO: Expand this list.

            switch(in[readIndex]) {
                case 'n':
                    out[writeIndex++] = '\n';
                    break;
                case 'r':
                    out[writeIndex++] = '\r';
                    break;
                case 't':
                    out[writeIndex++] = '\t';
                    break;
                case '"':
                    out[writeIndex++] = '\"';
                    break;
                default:
                    out[writeIndex++] = '\\';
                    out[writeIndex++] = in[readIndex];
                    break;
            }


        } else {
            out[writeIndex++] = in[readIndex];
        }
    }

    out[writeIndex] = 0;

    return out;
}

void consolidateStringLiterals(struct VM *vm, struct TokenList *tokenList)
{
    struct Token *tok = tokenList->first;

    while(tok) {

        if(tok->type == TOKENTYPE_STRING &&
            tok->next &&
            tok->next->type == TOKENTYPE_STRING)
        {
            // Found two strings in a row.
            struct Token *next = tok->next;

            // Make a concatenated string.
            char *newStr = malloc(strlen(tok->str) + strlen(tok->next->str) + 1);
            strcpy(newStr, tok->str);
            strcat(newStr, tok->next->str);

            // Replace this token's string with it.
            free(tok->str);
            tok->str = newStr;

            // Snip the next token out of the list.
            tok->next = next->next;
            deleteToken(next);

            // Fixup "last" pointer in the TokenList.
            if(tok->next == NULL) {
                assert(next == tokenList->last);
                tokenList->last = tok;
            }

        } else {

            // Only advance to the next one if we didn't do any
            // actions (because we may have to concatenate even more
            // stuff onto this token).
            tok = tok->next;

        }
    }
}

bool isValidIdentifierCharacter(char c, bool isFirstCharacter)
{
    if(!isFirstCharacter) {
        if(c >= '0' && c <= '9') {
            return true;
        }
    }

    if(c == '_') return true;
    if(c >= 'a' && c <= 'z') return true;
    if(c >= 'A' && c <= 'Z') return true;

    return false;
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
            char *strUnescaped   = NULL;
            uint32_t len         = 0;
            bool skipNextQuote   = false;

            while(*strEnd != 0 && (skipNextQuote || *strEnd != '\"')) {

                // We have to handle escaping here, but only for
                // strings. We'll do actual string escaping later.
                if(*strEnd == '\\') {
                    skipNextQuote = !skipNextQuote;
                } else {
                    skipNextQuote = false;
                }

                // In order to not mess up line counting, we're just
                // going to make newlines inside strings a bad thing.
                if(*strEnd == '\n') {
                    errorStateAddError(
                        &vm->errorState,
                        lineNumber, "Newline inside of quoted string.");
                    return false;
                }

                strEnd++;
            }

            // Copy the subsection of the string within the quotes.
            len = (strEnd - strStart);
            strTmp = malloc(len + 1);
            memcpy(strTmp, strStart, len);
            strTmp[len] = 0;

            strUnescaped = tokenizerUnescapeString(strTmp);

            addToken(TOKENTYPE_STRING, strUnescaped, lineNumber, tokenList);

            free(strTmp);
            free(strUnescaped);

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

        } else if(isValidIdentifierCharacter(str[i], true)) {

            uint32_t startIndex = i;
            char *tmp;

            i++;
            while(isValidIdentifierCharacter(str[i], false)) {
                i++;
            }

            tmp = malloc((i - startIndex) + 1);
            memcpy(tmp, str + startIndex, (i - startIndex));
            tmp[(i - startIndex)] = 0;

            addToken(TOKENTYPE_IDENTIFIER, tmp, lineNumber, tokenList);

            free(tmp);

            i--;

        } else if(!str[i]) {

            // Looks like we hit the end of the string after skipping
            // the whitespace. Do nothing.

        } else {

            errorStateAddError(
                &vm->errorState,
                lineNumber, "Unknown token.");

            return false;
        }

        i++;
    }

    consolidateStringLiterals(vm, tokenList);

    return true;
}

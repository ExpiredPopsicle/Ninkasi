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

void deleteToken(struct VM *vm, struct NKToken *token)
{
    if(token) {
        nkiFree(vm, token->str);
        nkiFree(vm, token);
    }
}

void destroyTokenList(
    struct VM *vm, struct NKTokenList *tokenList)
{
    struct NKToken *t = tokenList->first;
    while(t) {
        struct NKToken *next = t->next;
        deleteToken(vm, t);
        t = next;
    }
    tokenList->first = NULL;
    tokenList->last = NULL;
}

void addToken(
    struct VM *vm,
    enum NKTokenType type,
    const char *str,
    int32_t lineNumber,
    struct NKTokenList *tokenList)
{
    struct NKToken *newToken =
        nkiMalloc(vm, sizeof(struct NKToken));
    newToken->next = NULL;
    newToken->str = nkiStrdup(vm, str);
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
}

char *tokenizerUnescapeString(
    struct VM *vm,
    const char *in)
{
    // +2 for weird backslash-before null-terminator, and the regular
    // null terminator.
    char *out = nkiMalloc(vm, strlen(in) + 2);
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

void consolidateStringLiterals(struct VM *vm, struct NKTokenList *tokenList)
{
    struct NKToken *tok = tokenList->first;

    while(tok) {

        if(tok->type == NK_TOKENTYPE_STRING &&
            tok->next &&
            tok->next->type == NK_TOKENTYPE_STRING)
        {
            // Found two strings in a row.
            struct NKToken *next = tok->next;

            // Make a concatenated string.
            char *newStr = nkiMalloc(vm, strlen(tok->str) + strlen(tok->next->str) + 1);
            strcpy(newStr, tok->str);
            strcat(newStr, tok->next->str);

            // Replace this token's string with it.
            nkiFree(vm, tok->str);
            tok->str = newStr;

            // Snip the next token out of the list.
            tok->next = next->next;
            deleteToken(vm, next);

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

bool tokenize(struct VM *vm, const char *str, struct NKTokenList *tokenList)
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

            addToken(vm, NK_TOKENTYPE_PAREN_OPEN, "(", lineNumber, tokenList);

        } else if(str[i] == ')') {

            addToken(vm, NK_TOKENTYPE_PAREN_CLOSE, ")", lineNumber, tokenList);

        } else if(str[i] == '[') {

            addToken(vm, NK_TOKENTYPE_BRACKET_OPEN, "[", lineNumber, tokenList);

        } else if(str[i] == ']') {

            addToken(vm, NK_TOKENTYPE_BRACKET_CLOSE, "]", lineNumber, tokenList);

        } else if(str[i] == '{') {

            addToken(vm, NK_TOKENTYPE_CURLYBRACE_OPEN, "{", lineNumber, tokenList);

        } else if(str[i] == '}') {

            addToken(vm, NK_TOKENTYPE_CURLYBRACE_CLOSE, "}", lineNumber, tokenList);

        } else if(str[i] == '+') {

            // Check for "++".
            if(str[i+1] == '+') {
                addToken(vm, NK_TOKENTYPE_INCREMENT, "++", lineNumber, tokenList);
                i++;
            } else {
                addToken(vm, NK_TOKENTYPE_PLUS, "+", lineNumber, tokenList);
            }

        } else if(str[i] == '-') {

            // Check for "--".
            if(str[i+1] == '-') {
                addToken(vm, NK_TOKENTYPE_DECREMENT, "--", lineNumber, tokenList);
                i++;
            } else {
                addToken(vm, NK_TOKENTYPE_MINUS, "-", lineNumber, tokenList);
            }

        } else if(str[i] == '*') {

            addToken(vm, NK_TOKENTYPE_MULTIPLY, "*", lineNumber, tokenList);

        } else if(str[i] == '%') {

            addToken(vm, NK_TOKENTYPE_MODULO, "%", lineNumber, tokenList);

        } else if(str[i] == '.') {

            addToken(vm, NK_TOKENTYPE_DOT, ".", lineNumber, tokenList);

        } else if(str[i] == '/') {

            if(str[i+1] == '/') {

                // This is a comment. Run to the end of the line and
                // skip everything.
                while(str[i] && str[i] != '\n') {
                    i++;
                }
                i--;

            } else {
                addToken(vm, NK_TOKENTYPE_DIVIDE, "/", lineNumber, tokenList);
            }

        } else if(str[i] == ';') {

            addToken(vm, NK_TOKENTYPE_SEMICOLON, ";", lineNumber, tokenList);

        } else if(str[i] == '=') {

            if(str[i+1] == '=') {
                if(str[i+2] == '=') {
                    addToken(vm, NK_TOKENTYPE_EQUALWITHSAMETYPE, "===", lineNumber, tokenList);
                    i += 2;
                } else {
                    addToken(vm, NK_TOKENTYPE_EQUAL, "==", lineNumber, tokenList);
                    i++;
                }
            } else {
                addToken(vm, NK_TOKENTYPE_ASSIGNMENT, "=", lineNumber, tokenList);
            }

        } else if(str[i] == ',') {

            addToken(vm, NK_TOKENTYPE_COMMA, ",", lineNumber, tokenList);

        } else if(str[i] == '>') {

            if(str[i+1] == '=') {
                addToken(vm, NK_TOKENTYPE_GREATERTHANOREQUAL, ">=", lineNumber, tokenList);
                i++;
            } else {
                addToken(vm, NK_TOKENTYPE_GREATERTHAN, ">", lineNumber, tokenList);
            }

        } else if(str[i] == '<') {

            if(str[i+1] == '=') {
                addToken(vm, NK_TOKENTYPE_LESSTHANOREQUAL, "<=", lineNumber, tokenList);
                i++;
            } else {
                addToken(vm, NK_TOKENTYPE_LESSTHAN, "<", lineNumber, tokenList);
            }

        } else if(str[i] == '!') {

            if(str[i+1] == '=') {
                addToken(vm, NK_TOKENTYPE_NOTEQUAL, "!=", lineNumber, tokenList);
                i++;
            } else {
                addToken(vm, NK_TOKENTYPE_NOT, "!", lineNumber, tokenList);
            }

        } else if(str[i] == '&' && str[i+1] == '&') {

            i++;
            addToken(vm, NK_TOKENTYPE_AND, "&&", lineNumber, tokenList);

        } else if(str[i] == '|' && str[i+1] == '|') {

            i++;
            addToken(vm, NK_TOKENTYPE_OR, "||", lineNumber, tokenList);

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
                    nkiAddError(vm, lineNumber, "Newline inside of quoted string.");
                    return false;
                }

                strEnd++;
            }

            // Copy the subsection of the string within the quotes.
            len = (strEnd - strStart);
            strTmp = nkiMalloc(vm, len + 1);
            memcpy(strTmp, strStart, len);
            strTmp[len] = 0;

            strUnescaped = tokenizerUnescapeString(vm, strTmp);

            addToken(vm, NK_TOKENTYPE_STRING, strUnescaped, lineNumber, tokenList);

            nkiFree(vm, strTmp);
            nkiFree(vm, strUnescaped);

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
                vm,
                isFloat ? NK_TOKENTYPE_FLOAT : NK_TOKENTYPE_INTEGER,
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

            tmp = nkiMalloc(vm, (i - startIndex) + 1);
            memcpy(tmp, str + startIndex, (i - startIndex));
            tmp[(i - startIndex)] = 0;

            if(!strcmp(tmp, "var")) {
                addToken(vm, NK_TOKENTYPE_VAR, tmp, lineNumber, tokenList);
            } else if(!strcmp(tmp, "function")) {
                addToken(vm, NK_TOKENTYPE_FUNCTION, tmp, lineNumber, tokenList);
            } else if(!strcmp(tmp, "return")) {
                addToken(vm, NK_TOKENTYPE_RETURN, tmp, lineNumber, tokenList);
            } else if(!strcmp(tmp, "if")) {
                addToken(vm, NK_TOKENTYPE_IF, tmp, lineNumber, tokenList);
            } else if(!strcmp(tmp, "else")) {
                addToken(vm, NK_TOKENTYPE_ELSE, tmp, lineNumber, tokenList);
            } else if(!strcmp(tmp, "while")) {
                addToken(vm, NK_TOKENTYPE_WHILE, tmp, lineNumber, tokenList);
            } else if(!strcmp(tmp, "for")) {
                addToken(vm, NK_TOKENTYPE_FOR, tmp, lineNumber, tokenList);
            } else if(!strcmp(tmp, "newobject")) {
                addToken(vm, NK_TOKENTYPE_NEWOBJECT, tmp, lineNumber, tokenList);
            } else if(!strcmp(tmp, "nil")) {
                addToken(vm, NK_TOKENTYPE_NIL, tmp, lineNumber, tokenList);
            } else if(!strcmp(tmp, "break")) {
                addToken(vm, NK_TOKENTYPE_BREAK, tmp, lineNumber, tokenList);
            } else {
                addToken(vm, NK_TOKENTYPE_IDENTIFIER, tmp, lineNumber, tokenList);
            }

            nkiFree(vm, tmp);

            i--;

        } else if(!str[i]) {

            // Looks like we hit the end of the string after skipping
            // the whitespace. Do nothing.

        } else {

            nkiAddError(
                vm, lineNumber, "Unknown token.");

            return false;
        }

        i++;
    }

    consolidateStringLiterals(vm, tokenList);

    return true;
}

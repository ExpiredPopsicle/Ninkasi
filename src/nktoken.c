// ----------------------------------------------------------------------
//
//        ▐ ▄ ▪   ▐ ▄ ▄ •▄  ▄▄▄· .▄▄ · ▪
//       •█▌▐███ •█▌▐██▌▄▌▪▐█ ▀█ ▐█ ▀. ██
//       ▐█▐▐▌▐█·▐█▐▐▌▐▀▀▄·▄█▀▀█ ▄▀▀▀█▄▐█·
//       ██▐█▌▐█▌██▐█▌▐█.█▌▐█ ▪▐▌▐█▄▪▐█▐█▌
//       ▀▀ █▪▀▀▀▀▀ █▪·▀  ▀ ▀  ▀  ▀▀▀▀ ▀▀▀
//
// ----------------------------------------------------------------------
//
//   Ninkasi 0.01
//
//   By Cliff "ExpiredPopsicle" Jolly
//     https://expiredpopsicle.com
//     https://intoxicoding.com
//     expiredpopsicle@gmail.com
//
// ----------------------------------------------------------------------
//
//   Copyright (c) 2017 Clifford Jolly
//
//   Permission is hereby granted, free of charge, to any person
//   obtaining a copy of this software and associated documentation files
//   (the "Software"), to deal in the Software without restriction,
//   including without limitation the rights to use, copy, modify, merge,
//   publish, distribute, sublicense, and/or sell copies of the Software,
//   and to permit persons to whom the Software is furnished to do so,
//   subject to the following conditions:
//
//   The above copyright notice and this permission notice shall be
//   included in all copies or substantial portions of the Software.
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
//   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
//   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//   SOFTWARE.
//
// -------------------------- END HEADER -------------------------------------

#include "nkcommon.h"

nkbool nkiCompilerIsWhitespace(char c)
{
    if(c == ' ' || c == '\n' || c == '\t' || c == '\r') {
        return nktrue;
    }
    return nkfalse;
}

nkbool nkiCompilerIsNumber(char c)
{
    return (c >= '0' && c <= '9');
}

void nkiCompilerDeleteToken(struct NKVM *vm, struct NKToken *token)
{
    if(token) {
        nkiFree(vm, token->str);
        nkiFree(vm, token);
    }
}

void nkiCompilerDestroyTokenList(
    struct NKVM *vm, struct NKTokenList *tokenList)
{
    struct NKToken *t = tokenList->first;
    while(t) {
        struct NKToken *next = t->next;
        nkiCompilerDeleteToken(vm, t);
        t = next;
    }
    tokenList->first = NULL;
    tokenList->last = NULL;
}

void nkiCompilerAddToken(
    struct NKVM *vm,
    enum NKTokenType type,
    const char *str,
    nkint32_t lineNumber,
    struct NKTokenList *tokenList)
{
    struct NKToken *newToken =
        (struct NKToken *)nkiMalloc(
            vm, sizeof(struct NKToken));
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

char *nkiCompilerTokenizerUnescapeString(
    struct NKVM *vm,
    const char *in)
{
    // +2 for weird backslash-before null-terminator, and the regular
    // null terminator.
    char *out = (char *)nkiMalloc(vm, strlen(in) + 2);
    nkuint32_t len = strlen(in);
    nkuint32_t readIndex;
    nkuint32_t writeIndex = 0;

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

void nkiCompilerConsolidateStringLiterals(struct NKVM *vm, struct NKTokenList *tokenList)
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
            char *newStr = (char *)nkiMalloc(vm, strlen(tok->str) + strlen(tok->next->str) + 1);
            strcpy(newStr, tok->str);
            strcat(newStr, tok->next->str);

            // Replace this token's string with it.
            nkiFree(vm, tok->str);
            tok->str = newStr;

            // Snip the next token out of the list.
            tok->next = next->next;
            nkiCompilerDeleteToken(vm, next);

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

nkbool nkiCompilerIsValidIdentifierCharacter(char c, nkbool isFirstCharacter)
{
    if(!isFirstCharacter) {
        if(c >= '0' && c <= '9') {
            return nktrue;
        }
    }

    if(c == '_') return nktrue;
    if(c >= 'a' && c <= 'z') return nktrue;
    if(c >= 'A' && c <= 'Z') return nktrue;

    return nkfalse;
}

nkbool nkiCompilerTokenize(struct NKVM *vm, const char *str, struct NKTokenList *tokenList)
{
    nkuint32_t len = strlen(str);
    nkuint32_t i = 0;
    nkint32_t lineNumber = 1;

    while(i < len) {

        // Skip whitespace.
        while(i < len && nkiCompilerIsWhitespace(str[i])) {
            if(str[i] == '\n') {
                lineNumber++;
            }
            i++;
        }

        if(str[i] == '(') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_PAREN_OPEN, "(", lineNumber, tokenList);

        } else if(str[i] == ')') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_PAREN_CLOSE, ")", lineNumber, tokenList);

        } else if(str[i] == '[') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_BRACKET_OPEN, "[", lineNumber, tokenList);

        } else if(str[i] == ']') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_BRACKET_CLOSE, "]", lineNumber, tokenList);

        } else if(str[i] == '{') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_CURLYBRACE_OPEN, "{", lineNumber, tokenList);

        } else if(str[i] == '}') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_CURLYBRACE_CLOSE, "}", lineNumber, tokenList);

        } else if(str[i] == '+') {

            // Check for "++".
            if(str[i+1] == '+') {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_INCREMENT, "++", lineNumber, tokenList);
                i++;
            } else {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_PLUS, "+", lineNumber, tokenList);
            }

        } else if(str[i] == '-') {

            // Check for "--".
            if(str[i+1] == '-') {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_DECREMENT, "--", lineNumber, tokenList);
                i++;
            } else {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_MINUS, "-", lineNumber, tokenList);
            }

        } else if(str[i] == '*') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_MULTIPLY, "*", lineNumber, tokenList);

        } else if(str[i] == '%') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_MODULO, "%", lineNumber, tokenList);

        } else if(str[i] == '.') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_DOT, ".", lineNumber, tokenList);

        } else if(str[i] == '/') {

            if(str[i+1] == '/') {

                // This is a comment. Run to the end of the line and
                // skip everything.
                while(str[i] && str[i] != '\n') {
                    i++;
                }
                i--;

            } else {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_DIVIDE, "/", lineNumber, tokenList);
            }

        } else if(str[i] == ';') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_SEMICOLON, ";", lineNumber, tokenList);

        } else if(str[i] == '=') {

            if(str[i+1] == '=') {
                if(str[i+2] == '=') {
                    nkiCompilerAddToken(vm, NK_TOKENTYPE_EQUALWITHSAMETYPE, "===", lineNumber, tokenList);
                    i += 2;
                } else {
                    nkiCompilerAddToken(vm, NK_TOKENTYPE_EQUAL, "==", lineNumber, tokenList);
                    i++;
                }
            } else {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_ASSIGNMENT, "=", lineNumber, tokenList);
            }

        } else if(str[i] == ',') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_COMMA, ",", lineNumber, tokenList);

        } else if(str[i] == '>') {

            if(str[i+1] == '=') {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_GREATERTHANOREQUAL, ">=", lineNumber, tokenList);
                i++;
            } else {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_GREATERTHAN, ">", lineNumber, tokenList);
            }

        } else if(str[i] == '<') {

            if(str[i+1] == '=') {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_LESSTHANOREQUAL, "<=", lineNumber, tokenList);
                i++;
            } else {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_LESSTHAN, "<", lineNumber, tokenList);
            }

        } else if(str[i] == '!') {

            if(str[i+1] == '=') {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_NOTEQUAL, "!=", lineNumber, tokenList);
                i++;
            } else {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_NOT, "!", lineNumber, tokenList);
            }

        } else if(str[i] == '&' && str[i+1] == '&') {

            i++;
            nkiCompilerAddToken(vm, NK_TOKENTYPE_AND, "&&", lineNumber, tokenList);

        } else if(str[i] == '|' && str[i+1] == '|') {

            i++;
            nkiCompilerAddToken(vm, NK_TOKENTYPE_OR, "||", lineNumber, tokenList);

        } else if(str[i] == '\"') {

            const char *strStart = &str[i+1];
            const char *strEnd   = &str[i+1];
            char *strTmp         = NULL;
            char *strUnescaped   = NULL;
            nkuint32_t len         = 0;
            nkbool skipNextQuote   = nkfalse;

            while(*strEnd != 0 && (skipNextQuote || *strEnd != '\"')) {

                // We have to handle escaping here, but only for
                // strings. We'll do actual string escaping later.
                if(*strEnd == '\\') {
                    skipNextQuote = !skipNextQuote;
                } else {
                    skipNextQuote = nkfalse;
                }

                // In order to not mess up line counting, we're just
                // going to make newlines inside strings a bad thing.
                if(*strEnd == '\n') {
                    nkiAddError(vm, lineNumber, "Newline inside of quoted string.");
                    return nkfalse;
                }

                strEnd++;
            }

            // Copy the subsection of the string within the quotes.
            len = (strEnd - strStart);
            strTmp = (char *)nkiMalloc(vm, len + 1);
            memcpy(strTmp, strStart, len);
            strTmp[len] = 0;

            strUnescaped = nkiCompilerTokenizerUnescapeString(vm, strTmp);

            nkiCompilerAddToken(vm, NK_TOKENTYPE_STRING, strUnescaped, lineNumber, tokenList);

            nkiFree(vm, strTmp);
            nkiFree(vm, strUnescaped);

            // Skip the whole string and end quote.
            i += len + 1;

        } else if(nkiCompilerIsNumber(str[i])) {

            // Scan until we find the end of the number.
            nkuint32_t numberStart = i;
            nkbool isFloat = nkfalse;
            char tmp[256];

            while(i < len &&
                (nkiCompilerIsNumber(str[i]) || str[i] == '.') &&
                (i - numberStart) < sizeof(tmp) - 1)
            {
                tmp[i - numberStart] = str[i];
                if(str[i] == '.') {
                    isFloat = nktrue;
                }
                i++;
            }

            // Add null terminator.
            tmp[i - numberStart] = 0;

            // Back up one character, so we don't eat the first
            // character of whatever comes next.
            i--;

            nkiCompilerAddToken(
                vm,
                isFloat ? NK_TOKENTYPE_FLOAT : NK_TOKENTYPE_INTEGER,
                tmp,
                lineNumber,
                tokenList);

        } else if(nkiCompilerIsValidIdentifierCharacter(str[i], nktrue)) {

            nkuint32_t startIndex = i;
            char *tmp;

            i++;
            while(nkiCompilerIsValidIdentifierCharacter(str[i], nkfalse)) {
                i++;
            }

            tmp = (char *)nkiMalloc(vm, (i - startIndex) + 1);
            memcpy(tmp, str + startIndex, (i - startIndex));
            tmp[(i - startIndex)] = 0;

            if(!strcmp(tmp, "var")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_VAR, tmp, lineNumber, tokenList);
            } else if(!strcmp(tmp, "function")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_FUNCTION, tmp, lineNumber, tokenList);
            } else if(!strcmp(tmp, "return")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_RETURN, tmp, lineNumber, tokenList);
            } else if(!strcmp(tmp, "if")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_IF, tmp, lineNumber, tokenList);
            } else if(!strcmp(tmp, "else")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_ELSE, tmp, lineNumber, tokenList);
            } else if(!strcmp(tmp, "while")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_WHILE, tmp, lineNumber, tokenList);
            } else if(!strcmp(tmp, "do")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_DO, tmp, lineNumber, tokenList);
            } else if(!strcmp(tmp, "for")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_FOR, tmp, lineNumber, tokenList);
            } else if(!strcmp(tmp, "newobject")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_NEWOBJECT, tmp, lineNumber, tokenList);
            } else if(!strcmp(tmp, "nil")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_NIL, tmp, lineNumber, tokenList);
            } else if(!strcmp(tmp, "break")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_BREAK, tmp, lineNumber, tokenList);
            } else {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_IDENTIFIER, tmp, lineNumber, tokenList);
            }

            nkiFree(vm, tmp);

            i--;

        } else if(!str[i]) {

            // Looks like we hit the end of the string after skipping
            // the whitespace. Do nothing.

        } else {

            nkiAddError(
                vm, lineNumber, "Unknown token.");

            return nkfalse;
        }

        i++;
    }

    nkiCompilerConsolidateStringLiterals(vm, tokenList);

    return nktrue;
}

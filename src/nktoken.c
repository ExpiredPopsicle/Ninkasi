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
//   By Kiri "ExpiredPopsicle" Jolly
//     https://expiredpopsicle.com
//     https://intoxicoding.com
//     expiredpopsicle@gmail.com
//
// ----------------------------------------------------------------------
//
//   Copyright (c) 2017 Kiri Jolly
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
    nkuint32_t fileIndex,
    nkint32_t actualLineNumber,
    struct NKTokenList *tokenList)
{
    struct NKToken *newToken =
        (struct NKToken *)nkiMalloc(
            vm, sizeof(struct NKToken));
    newToken->next = NULL;
    newToken->str = nkiStrdup(vm, str);
    newToken->type = type;
    newToken->lineNumber = lineNumber;
    newToken->fileIndex = fileIndex;
    newToken->actualLineNumber = actualLineNumber;

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
    nkuint32_t readIndex;
    nkuint32_t writeIndex = 0;
    char *out;

    // Truncate the string if we must to prevent overflow.
    nkuint32_t len = nkiStrlen(in);
    if(len > ~(nkuint32_t)0 - 2) {
        len = ~(nkuint32_t)0 - 2;
        nkiAddError(vm, "String too long to escape.");
    }

    // +2 for weird backslash-before null-terminator, and the regular
    // null terminator.
    out = (char *)nkiMalloc(vm, len + 2);
    writeIndex = 0;

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
            nkuint32_t len1 = nkiStrlen(tok->str);
            nkuint32_t len2 = nkiStrlen(tok->next->str);
            char *newStr;

            // Truncate string for safety.
            if(len2 > ~(nkuint32_t)0 - len1) {
                len2 = ~(nkuint32_t)0 - len1;
                nkiAddError(vm, "String too long to concatenate.");
            }

            newStr = (char *)nkiMalloc(vm, len1 + len2 + 1);
            nkiStrcpy(newStr, tok->str);
            nkiStrcat(newStr, tok->next->str);

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

    // All your UTF-8 is valid for identifiers now.
    if((unsigned char)c >= 0x80) return nktrue;

    return nkfalse;
}

void nkiCompilerPreprocessorSkipWhitespace(
    const char *str,
    nkuint32_t *k)
{
    while(str[*k] && nkiCompilerIsWhitespace(str[*k])) {

        // Bail out when we get to the end of the line.
        if(str[*k] == '\n') {
            break;
        }

        (*k)++;
    }
}

void nkiCompilerFreeStringList(
    struct NKVM *vm,
    char **list,
    nkuint32_t count)
{
    nkuint32_t k;
    for(k = 0; k < count; k++) {
        nkiFree(vm, list[k]);
    }
    nkiFree(vm, list);
}

nkbool nkiCompilerGetPreprocessorTokens(
    struct NKVM *vm,
    const char *str,
    nkuint32_t lineNumber,
    nkuint32_t fileIndex,
    char ***tokenListOut,
    nkuint32_t *tokenCountOut)
{
    nkuint32_t k = 0;
    nkuint32_t tokenStart;
    nkuint32_t tokenEnd;
    nkbool tokenWasQuoted;

    char **tokens = NULL;
    nkuint32_t tokenCount = 0;

    // Skip '#'.
    if(str[k] == '#') {
        k++;
    }

    nkiCompilerPreprocessorSkipWhitespace(str, &k);

    while(str[k] && str[k] != '\n') {

        tokenWasQuoted = nkfalse;
        tokenStart = k;
        tokenEnd = k;

        if(nkiCompilerIsValidIdentifierCharacter(str[k], k == tokenStart)) {

            // Attempt to parse an identifier.
            while(nkiCompilerIsValidIdentifierCharacter(str[k], k == tokenStart)) {
                k++;
            }
            tokenEnd = k;

        } else if(str[k] >= '0' && str[k] <= '9') {

            // Attempt to parse a number.
            while(str[k] >= '0' && str[k] <= '9') {
                k++;
            }
            tokenEnd = k;


        } else if(str[k] == '"') {

            nkbool skipNext = nkfalse;

            tokenWasQuoted = nktrue;

            // Skip first quote.
            k++;
            tokenStart = k;

            // Attempt to parse a quoted string.
            while(1) {

                if(str[k] == '\\') {

                    skipNext = !skipNext;

                } else if(str[k] == '"') {

                    if(!skipNext) {

                        // Skip final '"'.
                        tokenEnd = k;
                        k++;
                        break;

                    }

                } else {

                    skipNext = nkfalse;

                }

                k++;
            }

        } else if(str[k] == '/' && str[k+1] == '/') {

            // Comment. Skip through to the end of the line.
            while(str[k] && str[k] != '\n') {
                k++;
            }

            tokenStart = k;
            tokenEnd = k;

        } else {

            // Spit out an error, cleanup, and return.
            nkiAddErrorEx(
                vm, lineNumber, fileIndex,
                "Unknown preprocessor token.");

            nkiCompilerFreeStringList(vm, tokens, tokenCount);

            return nkfalse;
        }

        if(str[tokenStart] && str[tokenStart] != '\n') {

            nkuint32_t tokenLength = tokenEnd - tokenStart;
            char *token;

            if(tokenLength == NK_UINT_MAX) {

                nkiAddErrorEx(
                    vm, lineNumber, fileIndex,
                    "Bad preprocessor token.");

                nkiCompilerFreeStringList(vm, tokens, tokenCount);

                return nkfalse;
            }

            // Extract the token substring.
            token = (char*)nkiMalloc(vm, tokenLength + 1);
            nkiMemcpy(token, str + tokenStart, tokenLength);
            token[tokenLength] = 0;

            // Unescape the string if it was a quoted one.
            if(tokenWasQuoted) {
                char *tmp = token;
                token = nkiCompilerTokenizerUnescapeString(vm, token);
                nkiFree(vm, tmp);
            }

            // Add the token to our list.
            tokenCount++;
            tokens = (char**)nkiReallocArray(
                vm, tokens,
                sizeof(char*),
                tokenCount);
            tokens[tokenCount - 1] = token;

        }

        // Skip to the next token.
        nkiCompilerPreprocessorSkipWhitespace(str, &k);

    }

    *tokenListOut = tokens;
    *tokenCountOut = tokenCount;

    return nktrue;
}

nkbool nkiCompilerTokenize(
    struct NKVM *vm,
    const char *str,
    struct NKTokenList *tokenList,
    const char *filename)
{
    nkuint32_t len = nkiStrlen(str);
    nkuint32_t i = 0;
    nkint32_t lineNumber = 1;
    nkuint32_t fileIndex = nkiVmAddSourceFile(vm, filename);
    nkint32_t actualLineNumber = 1;

  #define NK_TOKEN_STATE_PARAMETERS lineNumber, fileIndex, actualLineNumber, tokenList

    while(i < len) {

        // Skip whitespace.
        while(i < len && nkiCompilerIsWhitespace(str[i])) {
            if(str[i] == '\n') {
                lineNumber++;
                actualLineNumber++;
            }
            i++;
        }

        // Handle preprocessor directives.
        if(str[i] == '#') {

            // Preprocessor directive.
            nkuint32_t lineStart = i;
            char **tokenStringList = NULL;
            nkuint32_t tokenCount = 0;

            // Figure out how long the line is, and also advance the
            // tokenization system past it.
            while(str[i] != '\n' && str[i] != '\r' && str[i]) {
                i++;
            }

            // Split line into tokens.
            if(!nkiCompilerGetPreprocessorTokens(
                    vm, str + lineStart,
                    lineNumber, fileIndex,
                    &tokenStringList, &tokenCount))
            {
                nkiAddErrorEx(
                    vm, lineNumber,
                    fileIndex,
                    "Preprocessor directive parse failure.");
                return nkfalse;
            }

            // Actually do something based on the preprocessed stuff.
            if(tokenCount > 0) {

                if(!nkiStrcmp(tokenStringList[0], "line")) {

                    // Line directive from another preprocessor.
                    if(tokenCount != 2 && tokenCount != 3) {
                        nkiAddErrorEx(
                            vm, lineNumber,
                            fileIndex,
                            "Incorrect number of parameters for #line directive.");
                        nkiCompilerFreeStringList(vm, tokenStringList, tokenCount);
                        return nkfalse;
                    }

                    lineNumber = atol(tokenStringList[1]);

                    if(tokenCount > 2) {
                        fileIndex = nkiVmAddSourceFile(vm, tokenStringList[2]);
                    }

                } else if(!nkiStrcmp(tokenStringList[0], "file")) {

                    // File directive from another preprocessor.
                    if(tokenCount != 2) {
                        nkiAddErrorEx(
                            vm, lineNumber,
                            fileIndex,
                            "Incorrect number of parameters for #file directive.");
                        nkiCompilerFreeStringList(vm, tokenStringList, tokenCount);
                        return nkfalse;
                    }

                    fileIndex = nkiVmAddSourceFile(vm, tokenStringList[1]);

                } else {

                    // We don't know what token this is.
                    nkiAddErrorEx(
                        vm, lineNumber,
                        fileIndex,
                        "Unknown preprocessor directive.");
                    nkiCompilerFreeStringList(vm, tokenStringList, tokenCount);
                    return nkfalse;

                }
            }

            // Clean up.
            nkiCompilerFreeStringList(vm, tokenStringList, tokenCount);

        } else if(str[i] == '(') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_PAREN_OPEN, "(", NK_TOKEN_STATE_PARAMETERS);

        } else if(str[i] == ')') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_PAREN_CLOSE, ")", NK_TOKEN_STATE_PARAMETERS);

        } else if(str[i] == '[') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_BRACKET_OPEN, "[", NK_TOKEN_STATE_PARAMETERS);

        } else if(str[i] == ']') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_BRACKET_CLOSE, "]", NK_TOKEN_STATE_PARAMETERS);

        } else if(str[i] == '{') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_CURLYBRACE_OPEN, "{", NK_TOKEN_STATE_PARAMETERS);

        } else if(str[i] == '}') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_CURLYBRACE_CLOSE, "}", NK_TOKEN_STATE_PARAMETERS);

        } else if(str[i] == '+') {

            // Check for "++".
            if(str[i+1] == '+') {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_INCREMENT, "++", NK_TOKEN_STATE_PARAMETERS);
                i++;
            } else {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_PLUS, "+", NK_TOKEN_STATE_PARAMETERS);
            }

        } else if(str[i] == '-') {

            // Check for "--".
            if(str[i+1] == '-') {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_DECREMENT, "--", NK_TOKEN_STATE_PARAMETERS);
                i++;
            } else if(str[i+1] == '>') {
                // C-indirection ("->") operator.
                nkiCompilerAddToken(vm, NK_TOKENTYPE_C_INDIRECTION, "->", NK_TOKEN_STATE_PARAMETERS);
                i++;
            } else {
                // Just a "-".
                nkiCompilerAddToken(vm, NK_TOKENTYPE_MINUS, "-", NK_TOKEN_STATE_PARAMETERS);
            }

        } else if(str[i] == '*') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_MULTIPLY, "*", NK_TOKEN_STATE_PARAMETERS);

        } else if(str[i] == '%') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_MODULO, "%", NK_TOKEN_STATE_PARAMETERS);

        } else if(str[i] == '.') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_DOT, ".", NK_TOKEN_STATE_PARAMETERS);

        } else if(str[i] == '/') {

            if(str[i+1] == '/') {

                // This is a comment. Run to the end of the line and
                // skip everything.
                while(str[i] && str[i] != '\n') {
                    i++;
                }
                i--;

            } else {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_DIVIDE, "/", NK_TOKEN_STATE_PARAMETERS);
            }

        } else if(str[i] == ';') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_SEMICOLON, ";", NK_TOKEN_STATE_PARAMETERS);

        } else if(str[i] == '=') {

            if(str[i+1] == '=') {
                if(str[i+2] == '=') {
                    nkiCompilerAddToken(vm, NK_TOKENTYPE_EQUALWITHSAMETYPE, "===", NK_TOKEN_STATE_PARAMETERS);
                    i += 2;
                } else {
                    nkiCompilerAddToken(vm, NK_TOKENTYPE_EQUAL, "==", NK_TOKEN_STATE_PARAMETERS);
                    i++;
                }
            } else {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_ASSIGNMENT, "=", NK_TOKEN_STATE_PARAMETERS);
            }

        } else if(str[i] == ',') {

            nkiCompilerAddToken(vm, NK_TOKENTYPE_COMMA, ",", NK_TOKEN_STATE_PARAMETERS);

        } else if(str[i] == '>') {

            if(str[i+1] == '=') {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_GREATERTHANOREQUAL, ">=", NK_TOKEN_STATE_PARAMETERS);
                i++;
            } else {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_GREATERTHAN, ">", NK_TOKEN_STATE_PARAMETERS);
            }

        } else if(str[i] == '<') {

            if(str[i+1] == '=') {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_LESSTHANOREQUAL, "<=", NK_TOKEN_STATE_PARAMETERS);
                i++;
            } else {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_LESSTHAN, "<", NK_TOKEN_STATE_PARAMETERS);
            }

        } else if(str[i] == '!') {

            if(str[i+1] == '=') {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_NOTEQUAL, "!=", NK_TOKEN_STATE_PARAMETERS);
                i++;
            } else {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_NOT, "!", NK_TOKEN_STATE_PARAMETERS);
            }

        } else if(str[i] == '&' && str[i+1] == '&') {

            i++;
            nkiCompilerAddToken(vm, NK_TOKENTYPE_AND, "&&", NK_TOKEN_STATE_PARAMETERS);

        } else if(str[i] == '|' && str[i+1] == '|') {

            i++;
            nkiCompilerAddToken(vm, NK_TOKENTYPE_OR, "||", NK_TOKEN_STATE_PARAMETERS);

        } else if(str[i] == '\"') {

            const char *strStart = &str[i+1];
            const char *strEnd   = &str[i+1];
            char *strTmp         = NULL;
            char *strUnescaped   = NULL;
            nkuint32_t quotedLen = 0;
            nkbool skipNextQuote = nkfalse;

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
                    nkiAddErrorEx(vm, lineNumber, fileIndex, "Newline inside of quoted string.");
                    return nkfalse;
                }

                strEnd++;
            }

            // Copy the subsection of the string within the quotes.
            quotedLen = (strEnd - strStart);
            strTmp = (char *)nkiMalloc(vm, quotedLen + 1);
            nkiMemcpy(strTmp, strStart, quotedLen);
            strTmp[quotedLen] = 0;

            strUnescaped = nkiCompilerTokenizerUnescapeString(vm, strTmp);

            nkiCompilerAddToken(vm, NK_TOKENTYPE_STRING, strUnescaped, NK_TOKEN_STATE_PARAMETERS);

            nkiFree(vm, strTmp);
            nkiFree(vm, strUnescaped);

            // Skip the whole string and end quote.
            i += quotedLen + 1;

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
                NK_TOKEN_STATE_PARAMETERS);

        } else if(nkiCompilerIsValidIdentifierCharacter(str[i], nktrue)) {

            nkuint32_t startIndex = i;
            char *tmp;

            i++;
            while(nkiCompilerIsValidIdentifierCharacter(str[i], nkfalse)) {
                i++;
            }

            tmp = (char *)nkiMalloc(vm, (i - startIndex) + 1);
            nkiMemcpy(tmp, str + startIndex, (i - startIndex));
            tmp[(i - startIndex)] = 0;

            if(!nkiStrcmp(tmp, "var")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_VAR, tmp, NK_TOKEN_STATE_PARAMETERS);
            } else if(!nkiStrcmp(tmp, "function")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_FUNCTION, tmp, NK_TOKEN_STATE_PARAMETERS);
            } else if(!nkiStrcmp(tmp, "return")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_RETURN, tmp, NK_TOKEN_STATE_PARAMETERS);
            } else if(!nkiStrcmp(tmp, "if")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_IF, tmp, NK_TOKEN_STATE_PARAMETERS);
            } else if(!nkiStrcmp(tmp, "else")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_ELSE, tmp, NK_TOKEN_STATE_PARAMETERS);
            } else if(!nkiStrcmp(tmp, "while")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_WHILE, tmp, NK_TOKEN_STATE_PARAMETERS);
            } else if(!nkiStrcmp(tmp, "do")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_DO, tmp, NK_TOKEN_STATE_PARAMETERS);
            } else if(!nkiStrcmp(tmp, "for")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_FOR, tmp, NK_TOKEN_STATE_PARAMETERS);
            } else if(!nkiStrcmp(tmp, "newobject")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_NEWOBJECT, tmp, NK_TOKEN_STATE_PARAMETERS);
            } else if(!nkiStrcmp(tmp, "nil")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_NIL, tmp, NK_TOKEN_STATE_PARAMETERS);
            } else if(!nkiStrcmp(tmp, "break")) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_BREAK, tmp, NK_TOKEN_STATE_PARAMETERS);
            } else if(nkiCompilerIsFunctionStyleExpressionName(vm, tmp)) {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_FUNCTIONSTYLEEXPRESSION, tmp, NK_TOKEN_STATE_PARAMETERS);
            } else {
                nkiCompilerAddToken(vm, NK_TOKENTYPE_IDENTIFIER, tmp, NK_TOKEN_STATE_PARAMETERS);
            }

            nkiFree(vm, tmp);

            i--;

        } else if(!str[i]) {

            // Looks like we hit the end of the string after skipping
            // the whitespace. Do nothing.

        } else {

            nkiAddErrorEx(
                vm, lineNumber, fileIndex, "Unknown token.");

            return nkfalse;
        }

        i++;
    }

    nkiCompilerConsolidateStringLiterals(vm, tokenList);

    return nktrue;
}

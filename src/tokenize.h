#ifndef TOKENIZE_H
#define TOKENIZE_H

#include "enums.h"

struct VM;

struct NKToken
{
    enum TokenType type;
    char *str;
    struct NKToken *next;
    int32_t lineNumber;
};

struct NKTokenList
{
    struct NKToken *first;
    struct NKToken *last;
};

void deleteToken(
    struct VM *vm, struct NKToken *token);
void destroyTokenList(struct VM *vm, struct NKTokenList *tokenList);
void addToken(
    struct VM *vm,
    enum TokenType type,
    const char *str,
    int32_t lineNumber,
    struct NKTokenList *tokenList);
bool tokenize(struct VM *vm, const char *str, struct NKTokenList *tokenList);

#endif

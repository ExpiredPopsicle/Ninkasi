#ifndef TOKENIZE_H
#define TOKENIZE_H

#include "enums.h"

struct VM;

struct Token
{
    enum TokenType type;
    char *str;
    struct Token *next;
    int32_t lineNumber;
};

struct TokenList
{
    struct Token *first;
    struct Token *last;
};

void deleteToken(
    struct VM *vm, struct Token *token);
void destroyTokenList(struct VM *vm, struct TokenList *tokenList);
void addToken(
    struct VM *vm,
    enum TokenType type,
    const char *str,
    int32_t lineNumber,
    struct TokenList *tokenList);
bool tokenize(struct VM *vm, const char *str, struct TokenList *tokenList);

#endif

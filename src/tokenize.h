#ifndef TOKENIZE_H
#define TOKENIZE_H

#include "enums.h"

struct Token
{
    enum TokenType type;
    char *str;
    struct Token *next;
};

struct TokenList
{
    struct Token *first;
    struct Token *last;
};

void deleteToken(struct Token *token);
void destroyTokenList(struct TokenList *tokenList);
void addToken(
    enum TokenType type,
    const char *str,
    struct TokenList *tokenList);
bool tokenize(const char *str, struct TokenList *tokenList);

#endif

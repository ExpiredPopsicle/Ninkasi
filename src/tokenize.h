#ifndef NINKASI_TOKENIZE_H
#define NINKASI_TOKENIZE_H

#include "enums.h"

struct NKVM;

struct NKToken
{
    enum NKTokenType type;
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
    struct NKVM *vm, struct NKToken *token);
void destroyTokenList(struct NKVM *vm, struct NKTokenList *tokenList);
void addToken(
    struct NKVM *vm,
    enum NKTokenType type,
    const char *str,
    int32_t lineNumber,
    struct NKTokenList *tokenList);
bool tokenize(struct NKVM *vm, const char *str, struct NKTokenList *tokenList);

#endif // NINKASI_TOKENIZE_H


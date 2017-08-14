#ifndef NINKASI_TOKENIZE_H
#define NINKASI_TOKENIZE_H

#include "nkenums.h"

struct NKVM;

struct NKToken
{
    enum NKTokenType type;
    char *str;
    struct NKToken *next;
    nkint32_t lineNumber;
};

struct NKTokenList
{
    struct NKToken *first;
    struct NKToken *last;
};

void nkiCompilerDeleteToken(
    struct NKVM *vm, struct NKToken *token);
void nkiCompilerDestroyTokenList(struct NKVM *vm, struct NKTokenList *tokenList);
void nkiCompilerAddToken(
    struct NKVM *vm,
    enum NKTokenType type,
    const char *str,
    nkint32_t lineNumber,
    struct NKTokenList *tokenList);
nkbool nkiCompilerTokenize(struct NKVM *vm, const char *str, struct NKTokenList *tokenList);

#endif // NINKASI_TOKENIZE_H


#ifndef TOKENIZE_H
#define TOKENIZE_H

enum TokenType
{
    TOKENTYPE_INTEGER,
    TOKENTYPE_FLOAT,
    TOKENTYPE_PLUS,
    TOKENTYPE_MINUS,
    TOKENTYPE_MULTIPLY,
    TOKENTYPE_DIVIDE,
    TOKENTYPE_INCREMENT,
    TOKENTYPE_PAREN_OPEN,
    TOKENTYPE_PAREN_CLOSE,
    TOKENTYPE_BRACKET_OPEN,
    TOKENTYPE_BRACKET_CLOSE,

    TOKENTYPE_INVALID
};

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

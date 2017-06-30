#ifndef ENUMS_H
#define ENUMS_H

// ISO C doesn't allow forward declarations for enums, so we're just
// jamming all the enums for this system into this one file. It also
// doesn't allow C++ style comments but fuck it, whatever.

enum ValueType
{
    VALUETYPE_INT,
    VALUETYPE_FLOAT,
    VALUETYPE_STRING
};

enum Opcode
{
    OP_NOP = 0, // Leave this at zero so we can memset() sections of
                // code to zero.

    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,

    OP_PUSHLITERAL_INT,
    OP_PUSHLITERAL_FLOAT,
    OP_PUSHLITERAL_STRING,

    OP_POP,

    OP_DUMP,

    OP_STACKPEEK,
    OP_STACKPOKE,

    OPCODE_REALCOUNT,

    // This must be a power of two.
    OPCODE_PADDEDCOUNT = 32
};

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
    TOKENTYPE_CURLYBRACE_OPEN,
    TOKENTYPE_CURLYBRACE_CLOSE,
    TOKENTYPE_STRING,
    TOKENTYPE_IDENTIFIER,
    TOKENTYPE_SEMICOLON,
    TOKENTYPE_ASSIGNMENT,

    TOKENTYPE_VAR,

    TOKENTYPE_INVALID
};

#endif

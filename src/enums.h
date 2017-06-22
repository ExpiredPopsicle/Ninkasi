#ifndef ENUMS_H
#define ENUMS_H

// ISO C doesn't allow forward declarations for enums, so we're just
// jamming all the enums for this system into this one file. It also
// doesn't allow C++ style comments but fuck it, whatever.

enum ValueType
{
    VALUETYPE_INT,
};

enum Opcode
{
    OP_ADD,
    OP_PUSHLITERAL,
    OP_NOP,

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

    TOKENTYPE_INVALID
};

#endif

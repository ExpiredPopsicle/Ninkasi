#ifndef ENUMS_H
#define ENUMS_H

// ISO C doesn't allow forward declarations for enums, so we're just
// jamming all the enums for this system into this one file. It also
// doesn't allow C++ style comments but fuck it, whatever.

enum NKValueType
{
    NK_VALUETYPE_INT,
    NK_VALUETYPE_FLOAT,
    NK_VALUETYPE_STRING,
    NK_VALUETYPE_FUNCTIONID,
    NK_VALUETYPE_OBJECTID,
    NK_VALUETYPE_NIL
};

enum Opcode
{
    NK_OP_NOP = 0, // Leave this at zero so we can memset() sections of
                // code to zero.

    NK_OP_ADD,
    NK_OP_SUBTRACT,
    NK_OP_MULTIPLY,
    NK_OP_DIVIDE,
    NK_OP_NEGATE,
    NK_OP_MODULO,

    NK_OP_PUSHLITERAL_INT,
    NK_OP_PUSHLITERAL_FLOAT,
    NK_OP_PUSHLITERAL_STRING,
    NK_OP_PUSHLITERAL_FUNCTIONID,

    NK_OP_POP,
    NK_OP_POPN,

    NK_OP_DUMP,

    NK_OP_STACKPEEK,
    NK_OP_STACKPOKE,

    NK_OP_JUMP_RELATIVE,

    NK_OP_CALL,
    NK_OP_RETURN,

    NK_OP_END,

    NK_OP_JUMP_IF_ZERO,

    NK_OP_GREATERTHAN,
    NK_OP_LESSTHAN,
    NK_OP_GREATERTHANOREQUAL,
    NK_OP_LESSTHANOREQUAL,
    NK_OP_EQUAL,
    NK_OP_NOTEQUAL,
    NK_OP_EQUALWITHSAMETYPE,
    NK_OP_NOT,

    NK_OP_AND,
    NK_OP_OR,

    NK_OP_CREATEOBJECT,

    NK_OP_OBJECTFIELDGET,
    NK_OP_OBJECTFIELDSET,

    // Stuff specific to foo.bar() syntax.
    NK_OP_OBJECTFIELDGET_NOPOP,
    NK_OP_PREPARESELFCALL,

    NK_OP_PUSHNIL,

    OPCODE_REALCOUNT,

    // This must be a power of two.
    OPCODE_PADDEDCOUNT = 64
};

enum TokenType
{
    TOKENTYPE_INTEGER,
    TOKENTYPE_FLOAT,
    TOKENTYPE_PLUS,
    TOKENTYPE_MINUS,
    TOKENTYPE_MULTIPLY,
    TOKENTYPE_DIVIDE,
    TOKENTYPE_MODULO,
    TOKENTYPE_INCREMENT,
    TOKENTYPE_DECREMENT,
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
    TOKENTYPE_IF,
    TOKENTYPE_ELSE,
    TOKENTYPE_WHILE,
    TOKENTYPE_FOR,
    TOKENTYPE_AND,
    TOKENTYPE_OR,
    TOKENTYPE_DOT,

    TOKENTYPE_GREATERTHAN,
    TOKENTYPE_LESSTHAN,
    TOKENTYPE_GREATERTHANOREQUAL,
    TOKENTYPE_LESSTHANOREQUAL,
    TOKENTYPE_EQUAL,
    TOKENTYPE_NOTEQUAL,
    TOKENTYPE_EQUALWITHSAMETYPE,
    TOKENTYPE_NOT,

    TOKENTYPE_COMMA,

    TOKENTYPE_VAR,
    TOKENTYPE_FUNCTION,
    TOKENTYPE_RETURN,

    TOKENTYPE_NEWOBJECT,

    // These are for the foo.bar() syntax, specfically.
    TOKENTYPE_INDEXINTO_NOPOP,
    TOKENTYPE_FUNCTIONCALL_WITHSELF,

    TOKENTYPE_NIL,

    TOKENTYPE_BREAK,

    TOKENTYPE_INVALID,
};

#endif

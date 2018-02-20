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

#ifndef NINKASI_ENUMS_H
#define NINKASI_ENUMS_H

// Some compilers don't allow forward declarations for enums, so we're
// just jamming all the enums for this system into this one file. The
// older standards also don't allow C++ style comments but fuck it,
// whatever.

enum NKValueType
{
    NK_VALUETYPE_INT,
    NK_VALUETYPE_FLOAT,
    NK_VALUETYPE_STRING,
    NK_VALUETYPE_FUNCTIONID,
    NK_VALUETYPE_OBJECTID,
    NK_VALUETYPE_NIL
};

enum NKOpcode
{
    // Leave this at zero so we can memset() sections of code to zero
    // and have it turn into no-ops.
    NK_OP_NOP = 0,

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

    NK_OP_STACKPEEK,
    NK_OP_STACKPOKE,

    NK_OP_STATICPOKE,
    NK_OP_STATICPEEK,

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

    NK_OPCODE_REALCOUNT,

    // This must be a power of two.
    NK_OPCODE_PADDEDCOUNT = 64
};

enum NKTokenType
{
    NK_TOKENTYPE_INTEGER,
    NK_TOKENTYPE_FLOAT,
    NK_TOKENTYPE_PLUS,
    NK_TOKENTYPE_MINUS,
    NK_TOKENTYPE_MULTIPLY,
    NK_TOKENTYPE_DIVIDE,
    NK_TOKENTYPE_MODULO,
    NK_TOKENTYPE_INCREMENT,
    NK_TOKENTYPE_DECREMENT,
    NK_TOKENTYPE_PAREN_OPEN,
    NK_TOKENTYPE_PAREN_CLOSE,
    NK_TOKENTYPE_BRACKET_OPEN,
    NK_TOKENTYPE_BRACKET_CLOSE,
    NK_TOKENTYPE_CURLYBRACE_OPEN,
    NK_TOKENTYPE_CURLYBRACE_CLOSE,
    NK_TOKENTYPE_STRING,
    NK_TOKENTYPE_IDENTIFIER,
    NK_TOKENTYPE_SEMICOLON,
    NK_TOKENTYPE_ASSIGNMENT,
    NK_TOKENTYPE_IF,
    NK_TOKENTYPE_ELSE,
    NK_TOKENTYPE_WHILE,
    NK_TOKENTYPE_DO,
    NK_TOKENTYPE_FOR,
    NK_TOKENTYPE_AND,
    NK_TOKENTYPE_OR,
    NK_TOKENTYPE_DOT,

    NK_TOKENTYPE_GREATERTHAN,
    NK_TOKENTYPE_LESSTHAN,
    NK_TOKENTYPE_GREATERTHANOREQUAL,
    NK_TOKENTYPE_LESSTHANOREQUAL,
    NK_TOKENTYPE_EQUAL,
    NK_TOKENTYPE_NOTEQUAL,
    NK_TOKENTYPE_EQUALWITHSAMETYPE,
    NK_TOKENTYPE_NOT,

    NK_TOKENTYPE_COMMA,

    NK_TOKENTYPE_VAR,
    NK_TOKENTYPE_FUNCTION,
    NK_TOKENTYPE_RETURN,

    NK_TOKENTYPE_NEWOBJECT,

    // These are for the foo.bar() syntax, specfically.
    NK_TOKENTYPE_INDEXINTO_NOPOP,
    NK_TOKENTYPE_FUNCTIONCALL_WITHSELF,

    NK_TOKENTYPE_NIL,

    NK_TOKENTYPE_BREAK,

    NK_TOKENTYPE_INVALID,
};

#endif // NINKASI_ENUMS_H


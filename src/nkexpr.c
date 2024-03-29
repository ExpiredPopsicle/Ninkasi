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

#include "nkcommon.h"

// TODO: Make a non-recursive version of this.
void nkiCompilerDeleteExpressionNode(struct NKVM *vm, struct NKExpressionAstNode *node)
{
    if(!node) {
        return;
    }

    if(node->ownedToken) {
        nkiCompilerDeleteToken(vm, node->opOrValue);
        node->opOrValue = NULL;
    }

    if(node->children[0]) {
        nkiCompilerDeleteExpressionNode(vm, node->children[0]);
    }
    if(node->children[1]) {
        nkiCompilerDeleteExpressionNode(vm, node->children[1]);
    }

    nkiFree(vm, node);
}

nkbool nkiCompilerIsSubexpressionEndingToken(struct NKToken *token)
{
    return !token ||
        token->type == NK_TOKENTYPE_PAREN_CLOSE ||
        token->type == NK_TOKENTYPE_BRACKET_CLOSE ||
        token->type == NK_TOKENTYPE_COMMA;
}

nkbool nkiCompilerIsExpressionEndingToken(struct NKToken *token)
{
    return !token || nkiCompilerIsSubexpressionEndingToken(token) ||
        token->type == NK_TOKENTYPE_SEMICOLON;
}

nkbool nkiCompilerIsPrefixOperator(struct NKToken *token)
{
    return token && (
        token->type == NK_TOKENTYPE_MINUS ||
        token->type == NK_TOKENTYPE_DECREMENT ||
        token->type == NK_TOKENTYPE_INCREMENT ||
        token->type == NK_TOKENTYPE_NOT);
}

nkbool nkiCompilerIsPostfixOperator(struct NKToken *token)
{
    return token && (
        token->type == NK_TOKENTYPE_BRACKET_OPEN ||
        token->type == NK_TOKENTYPE_PAREN_OPEN ||
        token->type == NK_TOKENTYPE_DOT ||
        token->type == NK_TOKENTYPE_C_INDIRECTION ||
        token->type == NK_TOKENTYPE_INCREMENT ||
        token->type == NK_TOKENTYPE_DECREMENT);
}

nkint32_t nkiCompilerGetPrecedence(enum NKTokenType t)
{
    // Reference:
    // http://en.cppreference.com/w/cpp/language/operator_precedence

    switch(t) {
        case NK_TOKENTYPE_MULTIPLY:
        case NK_TOKENTYPE_DIVIDE:
        case NK_TOKENTYPE_MODULO:
            return 5;
        case NK_TOKENTYPE_PLUS:
        case NK_TOKENTYPE_MINUS:
            return 6;
        case NK_TOKENTYPE_GREATERTHAN:
        case NK_TOKENTYPE_LESSTHAN:
        case NK_TOKENTYPE_GREATERTHANOREQUAL:
        case NK_TOKENTYPE_LESSTHANOREQUAL:
            return 8;
        case NK_TOKENTYPE_EQUAL:
        case NK_TOKENTYPE_NOTEQUAL:
        case NK_TOKENTYPE_EQUALWITHSAMETYPE:
            return 9;
        case NK_TOKENTYPE_AND:
            return 13;
        case NK_TOKENTYPE_OR:
            return 14;
        case NK_TOKENTYPE_ASSIGNMENT:
            return 15;
        default:
            // Error?
            return -1;
    }
}

#define NK_CLEANUP_OUTER()                                                 \
    do {                                                                \
        while(opStack) {                                                \
            struct NKExpressionAstNode *next = opStack->stackNext;      \
            nkiCompilerDeleteExpressionNode(cs->vm, opStack);                      \
            opStack = next;                                             \
        }                                                               \
        while(valueStack) {                                             \
            struct NKExpressionAstNode *next = valueStack->stackNext;   \
            nkiCompilerDeleteExpressionNode(cs->vm, valueStack);                   \
            valueStack = next;                                          \
        }                                                               \
    } while(0)

#define NK_CLEANUP_INLOOP()                                \
    do {                                                \
        NK_CLEANUP_OUTER();                                \
        nkiCompilerDeleteExpressionNode(cs->vm, firstPrefixOp);    \
        nkiCompilerDeleteExpressionNode(cs->vm, lastPostfixOp);    \
        nkiCompilerDeleteExpressionNode(cs->vm, valueNode);        \
    } while(0)

#define NK_EXPECT_AND_SKIP(x)                       \
    do {                                            \
        if(!nkiCompilerExpectAndSkipToken(cs, x)) { \
            NK_CLEANUP_INLOOP();                       \
            nkiCompilerPopRecursion(cs);            \
            return NULL;                            \
        }                                           \
    } while(0)

nkbool nkiCompilerExpressionReduce(
    struct NKExpressionAstNode **opStack,
    struct NKExpressionAstNode **valueStack)
{
    struct NKExpressionAstNode *valNode1 = (*valueStack)->stackNext;
    struct NKExpressionAstNode *valNode2 = (*valueStack);
    struct NKExpressionAstNode *opNode   = (*opStack);

    if(!valNode1 || !valNode2 || !opNode) {
        return nkfalse;
    }

    // Pop off the values.
    (*valueStack) = (*valueStack)->stackNext->stackNext;
    valNode1->stackNext = NULL;
    valNode2->stackNext = NULL;
    (*opStack) = opNode->stackNext;
    opNode->stackNext = NULL;

    // Assign children and push the operation onto the
    // value stack.
    opNode->children[0] = valNode1;
    opNode->children[1] = valNode2;
    opNode->stackNext = (*valueStack);
    (*valueStack) = opNode;

    return nktrue;
}

nkbool nkiCompilerExpressionParseFunctioncall(
    struct NKExpressionAstNode *postfixNode,
    struct NKCompilerState *cs)
{
    if(!nkiCompilerPushRecursion(cs)) {
        return nkfalse;
    }

    postfixNode->isRootFunctionCallNode = nktrue;

    nkiCompilerNextToken(cs);
    {
        struct NKExpressionAstNode *lastParamNode = postfixNode;
        nkuint32_t argCount = 0;

        while(
            nkiCompilerCurrentTokenType(cs) != NK_TOKENTYPE_INVALID &&
            nkiCompilerCurrentTokenType(cs) != NK_TOKENTYPE_PAREN_CLOSE)
        {
            // Make a new node for this parameter.
            struct NKExpressionAstNode *thisParamNode =
                (struct NKExpressionAstNode *)nkiMalloc(
                    cs->vm, sizeof(struct NKExpressionAstNode));
            nkiMemset(thisParamNode, 0, sizeof(*thisParamNode));
            thisParamNode->opOrValue = postfixNode->opOrValue;

            // Parse the expression.
            thisParamNode->children[0] = nkiCompilerParseExpression(cs);

            // Add us to the end of the chain.
            lastParamNode->children[1] = thisParamNode;
            lastParamNode = thisParamNode;

            // Error-check.
            if(!thisParamNode->children[0]) {
                nkiCompilerAddError(cs, "Function parameter subexpression parse failure.");
                nkiCompilerPopRecursion(cs);
                return nkfalse;
            }

            // Skip commas.
            if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_COMMA) {
                nkiCompilerNextToken(cs);
            } else if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_PAREN_CLOSE) {
                // This is okay. It just means we're at the end.
            } else {
                // Anything else is bad.
                nkiCompilerAddError(cs, "Expected ',' or ')' in function parameter parsing.");
                nkiCompilerPopRecursion(cs);
                return nkfalse;
            }

            argCount++;
        }
    }

    if(!nkiCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_PAREN_CLOSE)) {
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }

    nkiCompilerPopRecursion(cs);
    return nktrue;
}

nkuint32_t nkiCompilerCountCallArguments(struct NKExpressionAstNode *node)
{
    nkuint32_t argCount = 0;
    struct NKExpressionAstNode *argumentAstNode = node->children[1];
    while(argumentAstNode) {
        argumentAstNode = argumentAstNode->children[1];
        argCount++;
    }
    return argCount;
}

struct NKExpressionAstNode *nkiCompilerParseExpression(struct NKCompilerState *cs)
{
    struct NKToken **currentToken = &cs->currentToken;
    struct NKExpressionAstNode *opStack = NULL;
    struct NKExpressionAstNode *valueStack = NULL;

    if(!nkiCompilerPushRecursion(cs)) {
        return NULL;
    }

    while(!nkiCompilerIsExpressionEndingToken(*currentToken)) {

        struct NKExpressionAstNode *lastPrefixOp   = NULL;
        struct NKExpressionAstNode *firstPrefixOp  = NULL;
        struct NKExpressionAstNode *lastPostfixOp  = NULL;
        struct NKExpressionAstNode *firstPostfixOp = NULL;
        struct NKExpressionAstNode *valueNode      = NULL;

        // Deal with prefix operators. Build up a list of them.
        while(nkiCompilerIsPrefixOperator(*currentToken)) {

            struct NKExpressionAstNode *prefixNode =
                (struct NKExpressionAstNode *)nkiMalloc(
                    cs->vm, sizeof(struct NKExpressionAstNode));
            nkiMemset(prefixNode, 0, sizeof(*prefixNode));

            // Add it to the end of our list of prefix operations.
            prefixNode->opOrValue = *currentToken;
            if(lastPrefixOp) {
                lastPrefixOp->children[0] = prefixNode;
            }
            lastPrefixOp = prefixNode;

            // Set it as the first if there's nothing else there.
            if(!firstPrefixOp) {
                firstPrefixOp = prefixNode;
            }

            nkiCompilerNextToken(cs);
        }

        // Parse a value or sub-expression.
        if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_PAREN_OPEN) {

            // Parse sub-expression.
            nkiCompilerNextToken(cs);
            valueNode = nkiCompilerParseExpression(cs);

            // Error check.
            if(!valueNode) {
                nkiCompilerAddError(cs, "Subexpression parse failure.");
                NK_CLEANUP_INLOOP();
                nkiCompilerPopRecursion(cs);
                return NULL;
            }

            // Make sure we ended on a closing parenthesis.
            if(!(*currentToken) || !nkiCompilerIsSubexpressionEndingToken(*currentToken)) {
                nkiCompilerAddError(cs, "Bad expression end.");
                NK_CLEANUP_INLOOP();
                nkiCompilerPopRecursion(cs);
                return NULL;
            }

            // Skip expression-ending token.
            nkiCompilerNextToken(cs);

        } else {

            // Parse the actual value.
            if(!nkiCompilerIsExpressionEndingToken(*currentToken)) {

                // Check for anonymous functions before we check for
                // literal values.
                if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_FUNCTION) {

                    char tmp[NK_PRINTF_UINTCHARSNEED + 1];
                    NKVMInternalFunctionID functionId = { 0 };

                    // Parse the function itself.
                    if(!nkiCompilerCompileFunctionDefinition(cs, nktrue, &functionId)) {
                        nkiCompilerAddError(cs, "Anonymous function parse error.");
                        NK_CLEANUP_INLOOP();
                        nkiCompilerPopRecursion(cs);
                        return NULL;
                    }

                    sprintf(tmp, NK_PRINTF_UINT32, functionId.id);

                    valueNode = (struct NKExpressionAstNode*)nkiMalloc(
                        cs->vm, sizeof(struct NKExpressionAstNode));
                    nkiMemset(valueNode, 0, sizeof(*valueNode));

                    valueNode->opOrValue = (struct NKToken *)nkiMalloc(
                        cs->vm, sizeof(struct NKToken));
                    valueNode->opOrValue->type = NK_TOKENTYPE_FUNCTION;
                    valueNode->opOrValue->str = nkiStrdup(cs->vm, tmp);
                    valueNode->opOrValue->next = NULL;
                    valueNode->opOrValue->lineNumber =
                        nkiCompilerCurrentTokenLinenumber(cs);

                    valueNode->ownedToken = nktrue;

                } else {

                    // Function-style expression names get routed
                    // through here as well as variable names and
                    // stuff.
                    valueNode = (struct NKExpressionAstNode*)nkiMalloc(
                        cs->vm, sizeof(struct NKExpressionAstNode));
                    nkiMemset(valueNode, 0, sizeof(*valueNode));
                    valueNode->opOrValue = *currentToken;

                    nkiCompilerNextToken(cs);
                }

            } else {

                // Prefix operator with no value. Why?
                // TODO: Raise error flag.
                nkiCompilerAddError(cs, "Prefix with no value.");
                NK_CLEANUP_INLOOP();
                nkiCompilerPopRecursion(cs);
                return NULL;
            }
        }

        // Deal with postfix operators.
        while(nkiCompilerIsPostfixOperator(*currentToken)) {

            enum NKTokenType currentTokenType;
            struct NKExpressionAstNode *postfixNode =
                (struct NKExpressionAstNode *)nkiMalloc(
                    cs->vm, sizeof(struct NKExpressionAstNode));
            nkiMemset(postfixNode, 0, sizeof(*postfixNode));

            // Add it to the end of our list of postfix operations.
            postfixNode->opOrValue = *currentToken;
            if(lastPostfixOp) {
                postfixNode->children[0] = lastPostfixOp;
            }
            lastPostfixOp = postfixNode;

            // Set it as the first if there's nothing else there.
            if(!firstPostfixOp) {
                firstPostfixOp = postfixNode;
            }

            currentTokenType = nkiCompilerCurrentTokenType(cs);
            if(currentTokenType == NK_TOKENTYPE_DOT ||
                currentTokenType == NK_TOKENTYPE_C_INDIRECTION)
            {
                // Handle index-into, or function call with "self" param.

                nkiCompilerNextToken(cs);

                if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_IDENTIFIER) {

                    struct NKExpressionAstNode *identifierStringNode =
                        (struct NKExpressionAstNode *)nkiMalloc(
                            cs->vm, sizeof(struct NKExpressionAstNode));

                    struct NKExpressionAstNode *indexIntoNode = postfixNode;

                    // Value node for the string.
                    nkiMemset(identifierStringNode, 0, sizeof(*identifierStringNode));
                    identifierStringNode->opOrValue = (struct NKToken *)nkiMalloc(
                        cs->vm, sizeof(struct NKToken));
                    identifierStringNode->opOrValue->type = NK_TOKENTYPE_STRING;
                    identifierStringNode->opOrValue->str = nkiStrdup(cs->vm, cs->currentToken->str);
                    identifierStringNode->opOrValue->next = NULL;
                    identifierStringNode->opOrValue->lineNumber = cs->currentToken->lineNumber;
                    identifierStringNode->ownedToken = nktrue;

                    // Expression node for the index-into operation.
                    indexIntoNode->opOrValue = (struct NKToken *)nkiMalloc(
                        cs->vm, sizeof(struct NKToken));
                    indexIntoNode->opOrValue->type = NK_TOKENTYPE_BRACKET_OPEN;
                    indexIntoNode->opOrValue->str = nkiStrdup(cs->vm, "[");
                    indexIntoNode->opOrValue->next = NULL;
                    indexIntoNode->opOrValue->lineNumber = cs->currentToken->lineNumber;
                    indexIntoNode->ownedToken = nktrue;
                    indexIntoNode->children[1] = identifierStringNode;

                    NK_EXPECT_AND_SKIP(NK_TOKENTYPE_IDENTIFIER);

                    // Now see if this is a function call.
                    if(currentTokenType == NK_TOKENTYPE_C_INDIRECTION) {

                        if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_PAREN_OPEN) {

                            struct NKExpressionAstNode *functionCallNode =
                                (struct NKExpressionAstNode *)nkiMalloc(
                                    cs->vm, sizeof(struct NKExpressionAstNode));

                            functionCallNode->opOrValue = (struct NKToken *)nkiMalloc(
                                cs->vm, sizeof(struct NKToken));
                            functionCallNode->opOrValue->type = NK_TOKENTYPE_PAREN_OPEN;
                            functionCallNode->opOrValue->str = nkiStrdup(cs->vm, "(");
                            functionCallNode->opOrValue->next = NULL;
                            functionCallNode->opOrValue->lineNumber = cs->currentToken->lineNumber;
                            functionCallNode->ownedToken = nktrue;
                            functionCallNode->children[0] = indexIntoNode;
                            functionCallNode->children[1] = NULL;

                            if(!functionCallNode->opOrValue->str ||
                                !nkiCompilerExpressionParseFunctioncall(
                                    functionCallNode, cs))
                            {
                                nkiFree(cs->vm, functionCallNode->opOrValue->str);
                                nkiFree(cs->vm, functionCallNode->opOrValue);
                                functionCallNode->opOrValue = NULL;
                                functionCallNode->children[0] = NULL;
                                nkiCompilerDeleteExpressionNode(cs->vm, functionCallNode);
                                NK_CLEANUP_INLOOP();
                                nkiCompilerPopRecursion(cs);
                                return NULL;
                            }

                            // Make sure the "self" value stays on the stack.
                            indexIntoNode->opOrValue->type =
                                NK_TOKENTYPE_INDEXINTO_NOPOP;

                            // And switch to a version of the function
                            // call that knows that the "self" parameter
                            // is going to be before the function itself
                            // in the stack.
                            functionCallNode->opOrValue->type =
                                NK_TOKENTYPE_FUNCTIONCALL_WITHSELF;

                            if(firstPostfixOp == postfixNode) {
                                firstPostfixOp = indexIntoNode;
                            }
                            if(lastPostfixOp == postfixNode) {
                                lastPostfixOp = functionCallNode;
                            }
                            postfixNode = functionCallNode;

                        } else {
                            nkiCompilerAddError(cs, "Expected function call after '->'.");
                            NK_CLEANUP_INLOOP();
                            nkiCompilerPopRecursion(cs);
                            return NULL;
                        }
                    }

                } else {
                    nkiCompilerAddError(cs, "Expected identifier after '.'.");
                    NK_CLEANUP_INLOOP();
                    nkiCompilerPopRecursion(cs);
                    return NULL;
                }

            } else if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_BRACKET_OPEN) {

                // Handle index-into operator.
                nkiCompilerNextToken(cs);
                postfixNode->children[1] = nkiCompilerParseExpression(cs);

                // Error-check.
                if(!postfixNode->children[1]) {
                    nkiCompilerAddError(cs, "Index subexpression parse failure.");
                    NK_CLEANUP_INLOOP();
                    nkiCompilerPopRecursion(cs);
                    return NULL;
                }

                NK_EXPECT_AND_SKIP(NK_TOKENTYPE_BRACKET_CLOSE);

            } else if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_PAREN_OPEN) {

                // Handle function call "operator".
                if(!nkiCompilerExpressionParseFunctioncall(postfixNode, cs)) {
                    NK_CLEANUP_INLOOP();
                    nkiCompilerPopRecursion(cs);
                    return NULL;
                }

            } else if(
                nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_INCREMENT ||
                nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_DECREMENT)
            {
                struct NKToken *oldToken = postfixNode->opOrValue;
                nkbool ownedOldToken = postfixNode->ownedToken;
                struct NKToken *newToken = (struct NKToken *)nkiMalloc(
                    cs->vm, sizeof(struct NKToken));

                // Set up a new token with the post
                // increment/decrement IDs.
                nkiMemset(newToken, 0, sizeof(*newToken));
                newToken->type =
                    oldToken->type == NK_TOKENTYPE_INCREMENT ?
                    NK_TOKENTYPE_POSTINCREMENT : NK_TOKENTYPE_POSTDECREMENT;
                newToken->str = nkiStrdup(cs->vm,
                    oldToken->type == NK_TOKENTYPE_INCREMENT ? "++" : "--");
                newToken->next = NULL;
                newToken->lineNumber = cs->currentToken->lineNumber;

                postfixNode->opOrValue = newToken;
                postfixNode->ownedToken = nktrue;

                nkiCompilerNextToken(cs);

                // Clean up old token.
                if(ownedOldToken) {
                    nkiCompilerDeleteToken(cs->vm, oldToken);
                    postfixNode->opOrValue = NULL;
                }

            } else {

                struct NKDynString *str =
                    nkiDynStrCreate(cs->vm, "Unknown postfix operator: ");
                nkiDynStrAppend(str, nkiCompilerCurrentTokenString(cs));
                nkiCompilerAddError(cs, str->data);
                nkiDynStrDelete(str);
                NK_CLEANUP_INLOOP();
                nkiCompilerPopRecursion(cs);
                return NULL;
            }
        }

        // Attach prefix and postfix operations.

        if(firstPostfixOp) {
            if(valueNode->opOrValue->type == NK_TOKENTYPE_FUNCTIONSTYLEEXPRESSION &&
                firstPostfixOp->opOrValue->type == NK_TOKENTYPE_PAREN_OPEN)
            {
                assert(!valueNode->children[0]);
                assert(!valueNode->children[1]);

                // Convert function-style expressions from the
                // function call layout to its own node, with the
                // arguments as children.

                // Clean up whatever token was there.
                if(firstPostfixOp->ownedToken) {
                    nkiCompilerDeleteToken(cs->vm, firstPostfixOp->opOrValue);
                }

                // Copy the operator name token into the function call
                // node, replacing the open paren with the operator
                // name.
                firstPostfixOp->opOrValue = (struct NKToken *)nkiMalloc(
                    cs->vm, sizeof(struct NKToken));
                nkiMemset(firstPostfixOp->opOrValue, 0, sizeof(struct NKToken));
                firstPostfixOp->opOrValue->type = valueNode->opOrValue->type;
                firstPostfixOp->opOrValue->str = nkiStrdup(cs->vm, valueNode->opOrValue->str);
                firstPostfixOp->opOrValue->lineNumber = valueNode->opOrValue->lineNumber;
                firstPostfixOp->ownedToken = nktrue;

                nkiCompilerDeleteExpressionNode(cs->vm, valueNode);

            } else {

                // Attach postfix ops.
                assert(firstPostfixOp->children[0] == NULL);
                firstPostfixOp->children[0] = valueNode;
            }

            valueNode = lastPostfixOp;
            lastPostfixOp = NULL;

        }

        if(lastPrefixOp) {

            // Attach prefix ops.
            assert(lastPrefixOp->children[0] == NULL);
            lastPrefixOp->children[0] = valueNode;
            valueNode = firstPrefixOp;
            firstPrefixOp = NULL;
        }

        // Push this value.
        valueNode->stackNext = valueStack;
        valueStack = valueNode;
        valueNode = NULL;

        // Check to see if we're just done or not.
        if(nkiCompilerIsExpressionEndingToken(cs->currentToken)) {
            break;
        }

        // Not done yet. Parse the next operator.

        // Make sure this is even something valid.
        if(nkiCompilerGetPrecedence((*currentToken)->type) == -1) {
            struct NKDynString *str =
                nkiDynStrCreate(cs->vm, "Unknown operator: ");
            nkiDynStrAppend(str, nkiCompilerCurrentTokenString(cs));
            nkiCompilerAddError(cs, str->data);
            nkiDynStrDelete(str);
            NK_CLEANUP_INLOOP();
            nkiCompilerPopRecursion(cs);
            return NULL;
        }

        // Attempt to reduce.
        while(opStack && nkiCompilerGetPrecedence(nkiCompilerCurrentTokenType(cs)) >=
            nkiCompilerGetPrecedence(opStack->opOrValue->type))
        {
            if(!nkiCompilerExpressionReduce(&opStack, &valueStack)) {
                nkiCompilerAddError(cs, "Expression parse failure.");
                NK_CLEANUP_INLOOP();
                nkiCompilerPopRecursion(cs);
                return NULL;
            }
        }

        // Push this operation onto the stack now that we've cleared
        // out everthing of a lower precedence.
        {
            struct NKExpressionAstNode *astNode =
                (struct NKExpressionAstNode *)nkiMalloc(
                    cs->vm, sizeof(struct NKExpressionAstNode));
            nkiMemset(astNode, 0, sizeof(*astNode));
            astNode->stackNext = opStack;
            astNode->opOrValue = cs->currentToken;
            opStack = astNode;
        }

        nkiCompilerNextToken(cs);
    }

    // Reduce all remaining operations.
    while(opStack) {
        if(!nkiCompilerExpressionReduce(&opStack, &valueStack)) {
            nkiCompilerAddError(cs, "Expression parse failure.");
            NK_CLEANUP_OUTER();
            nkiCompilerPopRecursion(cs);
            return NULL;
        }
    }

    if(!valueStack) {
        nkiCompilerAddError(cs, "No value to parse in expression.");
        NK_CLEANUP_OUTER();
        nkiCompilerPopRecursion(cs);
        return NULL;
    }

    // Check that stacks are empty.
    if(opStack || valueStack->stackNext) {
        // TODO: Maybe make this a normal error, but be able to clean
        // up afterwards?
        assert(0);
    }

    nkiCompilerPopRecursion(cs);
    return valueStack;
}

#undef NK_EXPECT_AND_SKIP
#undef NK_CLEANUP_OUTER
#undef NK_CLEANUP_INLOOP

nkbool nkiCompilerEmitFetchVariable(
    struct NKCompilerState *cs,
    const char *name,
    struct NKExpressionAstNode *node)
{
    struct NKCompilerStateContextVariable *var =
        nkiCompilerLookupVariable(cs, name);

    if(!var) {
        return nkfalse;
    }

    if(var->isGlobal) {

        // Positive values for global variables (absolute stack
        // position).
        nkiCompilerEmitPushLiteralInt(cs, var->position, nktrue);
        nkiCompilerAddInstructionSimple(
            cs, NK_OP_STATICPEEK, nkfalse);

    } else {

        // Negative values for local variables (stack position -
        // value).
        nkint32_t fetchStackPos = var->position - cs->context->stackFrameOffset;
        nkiCompilerEmitPushLiteralInt(cs, fetchStackPos, nktrue);
        nkiCompilerAddInstructionSimple(
            cs, NK_OP_STACKPEEK, nkfalse);

    }

    return nktrue;
}

nkbool nkiCompilerExpressionEmitSetVariable(
    struct NKCompilerState *cs,
    const char *name,
    struct NKExpressionAstNode *node)
{
    struct NKCompilerStateContextVariable *var =
        nkiCompilerLookupVariable(cs, name);

    if(!var) {
        return nkfalse;
    }

    if(var->isGlobal) {

        // Positive values for global variables (absolute stack
        // position).
        nkiCompilerEmitPushLiteralInt(cs, var->position, nktrue);
        nkiCompilerAddInstructionSimple(cs, NK_OP_STATICPOKE, nktrue);

    } else {

        // Negative values for local variables (stack position -
        // value).
        nkiCompilerEmitPushLiteralInt(cs,
            var->position - cs->context->stackFrameOffset, nktrue);
        nkiCompilerAddInstructionSimple(cs, NK_OP_STACKPOKE, nktrue);
    }

    return nktrue;
}

nkbool nkiCompilerEmitExpression(struct NKCompilerState *cs, struct NKExpressionAstNode *node);

nkbool nkiCompilerEmitExpressionAssignment(struct NKCompilerState *cs, struct NKExpressionAstNode *node)
{
    if(!nkiCompilerPushRecursion(cs)) {
        return nkfalse;
    }

    if(!node->children[1]) {
        nkiCompilerAddError(
            cs, "No RValue to assign in assignment.");
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }

    if(!node->children[0]) {
        nkiCompilerAddError(
            cs, "No LValue to assign in assignment.");
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }

    // Emit the value we want to assign.
    nkiCompilerEmitExpression(cs, node->children[1]);

    // Emit the assignment itself, which will depend on what we're
    // assigning to.

    switch(node->children[0]->opOrValue->type) {

        // Variable assignment.
        case NK_TOKENTYPE_IDENTIFIER: {
            nkiCompilerExpressionEmitSetVariable(
                cs,
                node->children[0]->opOrValue->str,
                node);
        } break;

        // Object index.
        case NK_TOKENTYPE_BRACKET_OPEN: {

            // Emit the thing we're going to assign to.
            nkiCompilerEmitExpression(cs, node->children[0]->children[0]); // Object id
            nkiCompilerEmitExpression(cs, node->children[0]->children[1]); // Index

            nkiCompilerAddInstructionSimple(cs, NK_OP_OBJECTFIELDSET, nktrue);
        } break;

        default: {
            // FIXME: For post increment/decrement, this spits out the
            // wrong value. Possibly related to the generated nodes we
            // make for it.
            struct NKDynString *dynStr =
                nkiDynStrCreate(cs->vm, "Operator or value cannot be used to generate an LValue: ");
            nkiDynStrAppend(dynStr, node->children[0]->opOrValue->str);
            nkiCompilerAddError(
                cs, dynStr->data);
            nkiDynStrDelete(dynStr);
            nkiCompilerPopRecursion(cs);
            return nkfalse;
        } break;

    }

    nkiCompilerPopRecursion(cs);
    return nktrue;
}

nkbool nkiCompilerEmitExpression(struct NKCompilerState *cs, struct NKExpressionAstNode *node)
{
    struct NKInstruction inst;
    nkuint32_t i;

    if(!nkiCompilerPushRecursion(cs)) {
        return nkfalse;
    }

    if(!node) {
        return nkfalse;
    }

    // Assignments are special, because we need to evaluate the left
    // side as an LValue. One of the reasons we convert inc/dec into
    // normal +/- with assignment nodes is because we can funnel it
    // through this special case.
    if(node->opOrValue->type == NK_TOKENTYPE_ASSIGNMENT) {
        nkiCompilerPopRecursion(cs);
        return nkiCompilerEmitExpressionAssignment(cs, node);
    }

    // Emit children.
    for(i = 0; i < 2; i++) {
        if(node->children[i]) {
            if(!nkiCompilerEmitExpression(cs, node->children[i])) {
                nkiCompilerPopRecursion(cs);
                return nkfalse;
            }
        }
    }

    nkiMemset(&inst, 0, sizeof(inst));

    switch(node->opOrValue->type) {

        case NK_TOKENTYPE_FUNCTION: {
            // Anonymous functions. Function id gets squished into the
            // token string.
            NKVMInternalFunctionID id;
            id.id = atol(node->opOrValue->str);
            nkiCompilerEmitPushLiteralFunctionId(cs, id, nktrue);
        } break;

        case NK_TOKENTYPE_INTEGER:
            nkiCompilerEmitPushLiteralInt(cs, atol(node->opOrValue->str), nktrue);
            break;

        case NK_TOKENTYPE_FLOAT:
            nkiCompilerEmitPushLiteralFloat(cs, atof(node->opOrValue->str), nktrue);
            break;

        case NK_TOKENTYPE_STRING:
            nkiCompilerEmitPushLiteralString(cs, node->opOrValue->str, nktrue);
            break;

        case NK_TOKENTYPE_NEWOBJECT:
            nkiCompilerAddInstructionSimple(cs, NK_OP_CREATEOBJECT, nktrue);
            break;

        case NK_TOKENTYPE_NIL:
            nkiCompilerEmitPushNil(cs, nktrue);
            break;

        case NK_TOKENTYPE_PLUS:
            nkiCompilerAddInstructionSimple(cs, NK_OP_ADD, nktrue);
            break;

        case NK_TOKENTYPE_MINUS:
            if(!node->children[1]) {
                nkiCompilerAddInstructionSimple(cs, NK_OP_NEGATE, nktrue);
            } else {
                nkiCompilerAddInstructionSimple(cs, NK_OP_SUBTRACT, nktrue);
            }
            break;

        case NK_TOKENTYPE_MULTIPLY:
            nkiCompilerAddInstructionSimple(cs, NK_OP_MULTIPLY, nktrue);
            break;

        case NK_TOKENTYPE_DIVIDE:
            nkiCompilerAddInstructionSimple(cs, NK_OP_DIVIDE, nktrue);
            break;

        case NK_TOKENTYPE_MODULO:
            nkiCompilerAddInstructionSimple(cs, NK_OP_MODULO, nktrue);
            break;

        case NK_TOKENTYPE_GREATERTHAN:
            nkiCompilerAddInstructionSimple(cs, NK_OP_GREATERTHAN, nktrue);
            break;

        case NK_TOKENTYPE_LESSTHAN:
            nkiCompilerAddInstructionSimple(cs, NK_OP_LESSTHAN, nktrue);
            break;

        case NK_TOKENTYPE_GREATERTHANOREQUAL:
            nkiCompilerAddInstructionSimple(cs, NK_OP_GREATERTHANOREQUAL, nktrue);
            break;

        case NK_TOKENTYPE_LESSTHANOREQUAL:
            nkiCompilerAddInstructionSimple(cs, NK_OP_LESSTHANOREQUAL, nktrue);
            break;

        case NK_TOKENTYPE_EQUAL:
            nkiCompilerAddInstructionSimple(cs, NK_OP_EQUAL, nktrue);
            break;

        case NK_TOKENTYPE_NOTEQUAL:
            nkiCompilerAddInstructionSimple(cs, NK_OP_NOTEQUAL, nktrue);
            break;

        case NK_TOKENTYPE_EQUALWITHSAMETYPE:
            nkiCompilerAddInstructionSimple(cs, NK_OP_EQUALWITHSAMETYPE, nktrue);
            break;

        case NK_TOKENTYPE_NOT:
            nkiCompilerAddInstructionSimple(cs, NK_OP_NOT, nktrue);
            break;

        case NK_TOKENTYPE_IDENTIFIER:
            nkiCompilerEmitFetchVariable(cs, node->opOrValue->str, node);
            break;

        case NK_TOKENTYPE_AND:
            nkiCompilerAddInstructionSimple(cs, NK_OP_AND, nktrue);
            break;

        case NK_TOKENTYPE_OR:
            nkiCompilerAddInstructionSimple(cs, NK_OP_OR, nktrue);
            break;

        case NK_TOKENTYPE_BRACKET_OPEN:
            nkiCompilerAddInstructionSimple(cs, NK_OP_OBJECTFIELDGET, nktrue);
            break;

        case NK_TOKENTYPE_FUNCTIONSTYLEEXPRESSION:
        {
            // Function-style expressions.
            if(node->isRootFunctionCallNode) {

                // Count up the arguments.
                nkuint32_t argumentCount =
                    nkiCompilerCountCallArguments(node);

                // Run the specific FSE-emitter for this.
                nkiCompilerEmitFunctionStyleExpression(
                    cs, node->opOrValue->str,
                    argumentCount);

            } else {

                nkiCompilerAddError(cs, "Corrupted AST in FSE creation.");
                return nkfalse;
            }

        } break;

        case NK_TOKENTYPE_PAREN_OPEN:
        case NK_TOKENTYPE_FUNCTIONCALL_WITHSELF:
        {
            // Function calls.
            if(node->isRootFunctionCallNode) {

                // Count up the arguments.
                nkuint32_t argumentCount = nkiCompilerCountCallArguments(node);

                // Push argument count.
                nkiCompilerEmitPushLiteralInt(cs, argumentCount, nkfalse);

                // Handle self-calls.
                if(node->opOrValue->type == NK_TOKENTYPE_FUNCTIONCALL_WITHSELF) {
                    nkiCompilerAddInstructionSimple(cs, NK_OP_PREPARESELFCALL, nkfalse);
                    argumentCount++;
                }

                // Emit function call.
                nkiCompilerAddInstructionSimple(cs, NK_OP_CALL, nkfalse);

                cs->context->stackFrameOffset -= argumentCount;

            }

        } break;

        case NK_TOKENTYPE_INDEXINTO_NOPOP:
            nkiCompilerAddInstructionSimple(cs, NK_OP_OBJECTFIELDGET_NOPOP, nktrue);
            break;

        default: {
            struct NKDynString *dynStr =
                nkiDynStrCreate(cs->vm, "Unknown value or operator in nkiCompilerEmitExpression: ");
            nkiDynStrAppend(dynStr, node->opOrValue->str);
            nkiAddErrorEx(
                cs->vm,
                node->opOrValue->lineNumber,
                node->opOrValue->fileIndex,
                dynStr->data);
            nkiDynStrDelete(dynStr);
            nkiCompilerPopRecursion(cs);
            return nkfalse;
        } break;
    }

    nkiCompilerPopRecursion(cs);
    return nktrue;
}

struct NKExpressionAstNode *nkiCompilerCloneExpressionTree(
    struct NKCompilerState *cs,
    struct NKExpressionAstNode *node)
{
    struct NKExpressionAstNode *newNode;

    if(!node) {
        return NULL;
    }

    newNode = (struct NKExpressionAstNode *)nkiMalloc(
        cs->vm, sizeof(struct NKExpressionAstNode));
    nkiMemset(newNode, 0, sizeof(*newNode));

    newNode->ownedToken = node->ownedToken;
    if(node->ownedToken) {
        newNode->opOrValue = (struct NKToken *)nkiMalloc(
            cs->vm, sizeof(struct NKToken));
        nkiMemset(newNode->opOrValue, 0, sizeof(struct NKToken));
        newNode->opOrValue->type = node->opOrValue->type;
        newNode->opOrValue->str = nkiStrdup(cs->vm, node->opOrValue->str);
        newNode->opOrValue->lineNumber = node->opOrValue->lineNumber;
    } else {
        newNode->opOrValue = node->opOrValue;
    }

    newNode->isRootFunctionCallNode = node->isRootFunctionCallNode;

    newNode->children[0] = nkiCompilerCloneExpressionTree(cs, node->children[0]);
    newNode->children[1] = nkiCompilerCloneExpressionTree(cs, node->children[1]);

    return newNode;
}

void nkiCompilerExpressionExpandIncrementsAndDecrements(
    struct NKCompilerState *cs,
    struct NKExpressionAstNode *node,
    nkbool returnValueCanBeDiscarded)
{
    if(!node) {
        return;
    }

    if(!nkiCompilerPushRecursion(cs)) {
        return;
    }

    if(node->opOrValue->type == NK_TOKENTYPE_INCREMENT ||
        node->opOrValue->type == NK_TOKENTYPE_DECREMENT ||
        node->opOrValue->type == NK_TOKENTYPE_POSTINCREMENT ||
        node->opOrValue->type == NK_TOKENTYPE_POSTDECREMENT)
    {
        assert(node->children[0]);
        assert(!node->children[1]);

        {
            struct NKExpressionAstNode *lvalueNode = node->children[0];
            struct NKExpressionAstNode *rvalueNode1 =
                nkiCompilerCloneExpressionTree(cs, node->children[0]);
            struct NKExpressionAstNode *additionNode =
                (struct NKExpressionAstNode *)nkiMalloc(
                    cs->vm, sizeof(struct NKExpressionAstNode));
            struct NKExpressionAstNode *literalOneNode =
                (struct NKExpressionAstNode *)nkiMalloc(
                    cs->vm, sizeof(struct NKExpressionAstNode));
            struct NKToken *oldToken = node->opOrValue;
            nkbool wasOwningToken = node->ownedToken;

            enum NKTokenType simpleType =
                (oldToken->type == NK_TOKENTYPE_INCREMENT ||
                    oldToken->type == NK_TOKENTYPE_POSTINCREMENT) ?
                NK_TOKENTYPE_INCREMENT : NK_TOKENTYPE_DECREMENT;

            struct NKToken *literalOneToken =
                (struct NKToken *)nkiMalloc(
                    cs->vm, sizeof(struct NKToken));
            struct NKToken *additionToken =
                (struct NKToken *)nkiMalloc(
                    cs->vm, sizeof(struct NKToken));
            struct NKToken *assignmentToken =
                (struct NKToken *)nkiMalloc(
                    cs->vm, sizeof(struct NKToken));
            char *oneTokenStr = nkiStrdup(cs->vm, "1");
            char *addTokenStr = nkiStrdup(
                cs->vm,
                simpleType == NK_TOKENTYPE_INCREMENT ?
                "+" : "-");
            char *assignTokenStr = nkiStrdup(cs->vm, "=");

            // Generate a node for the number 1.
            literalOneNode->opOrValue = literalOneToken;
            literalOneNode->opOrValue->type = NK_TOKENTYPE_INTEGER;
            literalOneNode->opOrValue->str = oneTokenStr;
            literalOneNode->opOrValue->lineNumber = node->opOrValue->lineNumber;
            literalOneNode->opOrValue->next = NULL;
            literalOneNode->children[0] = NULL;
            literalOneNode->children[1] = NULL;
            literalOneNode->stackNext = NULL;
            literalOneNode->ownedToken = nktrue;
            literalOneNode->isRootFunctionCallNode = nkfalse;

            // Generate a node that just adds 1 to the value.
            additionNode->opOrValue = additionToken;
            additionNode->opOrValue->type =
                simpleType == NK_TOKENTYPE_INCREMENT ? NK_TOKENTYPE_PLUS : NK_TOKENTYPE_MINUS;
            additionNode->opOrValue->str = addTokenStr;
            additionNode->opOrValue->lineNumber = node->opOrValue->lineNumber;
            additionNode->opOrValue->next = NULL;
            additionNode->children[0] = rvalueNode1;
            additionNode->children[1] = literalOneNode;
            additionNode->stackNext = NULL;
            additionNode->ownedToken = nktrue;
            additionNode->isRootFunctionCallNode = nkfalse;

            // Turn this node into one that assigns the added value
            // back to the original value. This just turns this node
            // into an assignment node.
            node->opOrValue = assignmentToken;
            node->opOrValue->type = NK_TOKENTYPE_ASSIGNMENT;
            node->opOrValue->str = assignTokenStr;
            node->opOrValue->lineNumber = oldToken->lineNumber;
            node->opOrValue->next = NULL;
            node->children[0] = lvalueNode;
            node->children[1] = additionNode;
            node->stackNext = NULL;
            node->ownedToken = nktrue;
            node->isRootFunctionCallNode = nkfalse;

            // For post increment/decrement, we're going to just add a
            // node that adds or subtracts back from the final value
            // to restore it to what it was before the operation,
            // which means inserting a new node into this hierarchy.
            if(oldToken->type == NK_TOKENTYPE_POSTDECREMENT ||
                oldToken->type == NK_TOKENTYPE_POSTINCREMENT)
            {
                // Skip doing this for stuff where we're not going to
                // use the return value.
                if(!returnValueCanBeDiscarded) {

                    // Create a new node and copy the current root
                    // into that.
                    struct NKExpressionAstNode *shiftedRootNode =
                        (struct NKExpressionAstNode *)nkiMalloc(
                            cs->vm, sizeof(struct NKExpressionAstNode));
                    *shiftedRootNode = *node;

                    // Turn the root node into an addition or
                    // subtraction node, with the old root as one of
                    // the children.
                    node->opOrValue =
                        (struct NKToken *)nkiMalloc(
                            cs->vm, sizeof(struct NKToken));
                    node->opOrValue->type =
                        simpleType == NK_TOKENTYPE_INCREMENT
                        ? NK_TOKENTYPE_MINUS
                        : NK_TOKENTYPE_PLUS;
                    node->opOrValue->str = nkiStrdup(
                        cs->vm,
                        simpleType == NK_TOKENTYPE_INCREMENT
                        ? "-"
                        : "+");
                    node->opOrValue->lineNumber = oldToken->lineNumber;
                    node->opOrValue->next = NULL;
                    node->children[0] = shiftedRootNode;
                    node->children[1] =
                        (struct NKExpressionAstNode*)nkiMalloc(
                            cs->vm, sizeof(struct NKExpressionAstNode));
                    node->stackNext = NULL;
                    node->ownedToken = nktrue;
                    node->isRootFunctionCallNode = nkfalse;

                    // Copy the literal one node for the second child
                    // (the number to add or subtract).
                    *node->children[1] = *literalOneNode;
                    node->children[1]->opOrValue = nkiMalloc(
                        cs->vm, sizeof(struct NKToken));
                    *node->children[1]->opOrValue = *literalOneToken;
                    node->children[1]->opOrValue->str = nkiStrdup(
                        cs->vm, literalOneToken->str);

                } else {

                    // If we hit this case, then the return value from
                    // the expression can be ignored, meaning that we
                    // don't have to do the extra addition/subtraction
                    // for a post increment/decrement, so just skip
                    // it.

                }
            }

            if(wasOwningToken) {
                nkiCompilerDeleteToken(cs->vm, oldToken);
            }
        }

    } else {

        if(node->children[0]) {
            nkiCompilerExpressionExpandIncrementsAndDecrements(
                cs, node->children[0], nkfalse);
        }

        if(node->children[1]) {
            nkiCompilerExpressionExpandIncrementsAndDecrements(
                cs, node->children[1], nkfalse);
        }
    }

    nkiCompilerPopRecursion(cs);
}

struct NKExpressionAstNode *nkiCompilerCompileExpressionWithoutEmit(
    struct NKCompilerState *cs,
    nkbool returnValueCanBeDiscarded)
{
    struct NKExpressionAstNode *node;

    if(!nkiCompilerPushRecursion(cs)) {
        return NULL;
    }

    node = nkiCompilerParseExpression(cs);

    if(node) {
        nkiCompilerExpressionExpandIncrementsAndDecrements(
            cs, node, returnValueCanBeDiscarded);
        nkiCompilerOptimizeConstants(cs->vm, &node);
    }

    if(nkiGetErrorCount(cs->vm)) {
        nkiCompilerDeleteExpressionNode(cs->vm, node);
        nkiCompilerPopRecursion(cs);
        return NULL;
    }

    nkiCompilerPopRecursion(cs);
    return node;
}

nkbool nkiCompilerCompileExpression(
    struct NKCompilerState *cs,
    nkbool returnValueCanBeDiscarded)
{
    struct NKExpressionAstNode *node;

    if(!nkiCompilerPushRecursion(cs)) {
        return nkfalse;
    }

    node = nkiCompilerCompileExpressionWithoutEmit(
        cs, returnValueCanBeDiscarded);

    if(node) {
        nkbool ret;
        ret = nkiCompilerEmitExpression(cs, node);
        nkiCompilerDeleteExpressionNode(cs->vm, node);
        nkiCompilerPopRecursion(cs);
        return ret;
    }

    nkiCompilerPopRecursion(cs);
    return nkfalse;
}

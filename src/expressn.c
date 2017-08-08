#include "common.h"

// TODO: Make a non-recursive version of this.
void deleteExpressionNode(struct NKVM *vm, struct NKExpressionAstNode *node)
{
    if(!node) {
        return;
    }

    if(node->ownedToken) {
        deleteToken(vm, node->opOrValue);
        node->opOrValue = NULL;
    }

    if(node->children[0]) {
        deleteExpressionNode(vm, node->children[0]);
    }
    if(node->children[1]) {
        deleteExpressionNode(vm, node->children[1]);
    }

    nkiFree(vm, node);
}

// TODO: Remove this.
void dumpExpressionAstNode(struct NKExpressionAstNode *node)
{
    if(!node) {
        dbgWriteLine("<NULL>");
        return;
    }

    if(isPrefixOperator(node->opOrValue) && !node->children[1]) {

        dbgWriteLine(node->opOrValue->str);
        dbgWriteLine("(");
        dbgPush();
        dumpExpressionAstNode(node->children[0]);
        dbgPop();
        dbgWriteLine(")");

    } else if(node->opOrValue->type == NK_TOKENTYPE_BRACKET_OPEN) {

        dbgWriteLine("(");
        dumpExpressionAstNode(node->children[0]);
        dbgWriteLine(")[");
        dumpExpressionAstNode(node->children[1]);
        dbgWriteLine("]");

    } else {

        dbgWriteLine("(");

        if(node->children[0]) {
            dbgWriteLine("(");
            dbgPush();
            dumpExpressionAstNode(node->children[0]);
            dbgPop();
            dbgWriteLine(")");
        }

        dbgPush();
        dbgWriteLine("%s", node->opOrValue->str);
        dbgPop();

        if(node->children[1]) {
            dbgWriteLine("(");
            dbgPush();
            dumpExpressionAstNode(node->children[1]);
            dbgPop();
            dbgWriteLine(")");
        }

        dbgWriteLine(")");

    }
}

bool isSubexpressionEndingToken(struct NKToken *token)
{
    return !token ||
        token->type == NK_TOKENTYPE_PAREN_CLOSE ||
        token->type == NK_TOKENTYPE_BRACKET_CLOSE ||
        token->type == NK_TOKENTYPE_COMMA;
}

bool isExpressionEndingToken(struct NKToken *token)
{
    return !token || isSubexpressionEndingToken(token) ||
        token->type == NK_TOKENTYPE_SEMICOLON;
}

bool isPrefixOperator(struct NKToken *token)
{
    return token && (
        token->type == NK_TOKENTYPE_MINUS ||
        token->type == NK_TOKENTYPE_DECREMENT ||
        token->type == NK_TOKENTYPE_INCREMENT ||
        token->type == NK_TOKENTYPE_NOT);
}

bool isPostfixOperator(struct NKToken *token)
{
    return token && (
        token->type == NK_TOKENTYPE_BRACKET_OPEN ||
        token->type == NK_TOKENTYPE_PAREN_OPEN ||
        token->type == NK_TOKENTYPE_DOT);
}

int32_t getPrecedence(enum NKTokenType t)
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

#define PARSE_ERROR(x)                          \
    vmCompilerAddError(                         \
        cs,                                     \
        (x))

#define CLEANUP_OUTER()                                                 \
    do {                                                                \
        while(opStack) {                                                \
            struct NKExpressionAstNode *next = opStack->stackNext;      \
            deleteExpressionNode(cs->vm, opStack);                      \
            opStack = next;                                             \
        }                                                               \
        while(valueStack) {                                             \
            struct NKExpressionAstNode *next = valueStack->stackNext;   \
            deleteExpressionNode(cs->vm, valueStack);                   \
            valueStack = next;                                          \
        }                                                               \
    } while(0)

#define CLEANUP_INLOOP()                                \
    do {                                                \
        CLEANUP_OUTER();                                \
        deleteExpressionNode(cs->vm, firstPrefixOp);    \
        deleteExpressionNode(cs->vm, lastPostfixOp);    \
        deleteExpressionNode(cs->vm, valueNode);        \
        } while(0)

#define EXPECT_AND_SKIP(x)                                              \
    do {                                                                \
        if(vmCompilerTokenType(cs) != (x)) {                            \
            struct NKDynString *errStr =                                \
                nkiDynStrCreate(cs->vm, "Unexpected token: ");          \
            nkiDynStrAppend(                                            \
                errStr,                                                 \
                vmCompilerTokenString(cs));                             \
            PARSE_ERROR(errStr->data);                                  \
            nkiDynStrDelete(errStr);                                    \
            CLEANUP_INLOOP();                                           \
            nkiCompilerPopRecursion(cs);                                \
            return NULL;                                                \
        }                                                               \
        vmCompilerNextToken(cs);                                        \
    } while(0)

bool reduce(
    struct NKExpressionAstNode **opStack,
    struct NKExpressionAstNode **valueStack)
{
    struct NKExpressionAstNode *valNode1 = (*valueStack)->stackNext;
    struct NKExpressionAstNode *valNode2 = (*valueStack);
    struct NKExpressionAstNode *opNode   = (*opStack);

    if(!valNode1 || !valNode2 || !opNode) {
        return false;
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

    return true;
}

bool parseFunctioncall(
    struct NKExpressionAstNode *postfixNode,
    struct NKCompilerState *cs)
{
    if(!nkiCompilerPushRecursion(cs)) {
        return false;
    }

    // Handle function call "operator".
    dbgWriteLine("Handling function call operator.");

    // TODO: Keep handling expressions as long as we end
    // up on a comma.

    postfixNode->isRootFunctionCallNode = true;

    dbgPush();
    vmCompilerNextToken(cs);
    {
        struct NKExpressionAstNode *lastParamNode = postfixNode;
        uint32_t argCount = 0;

        while(
            vmCompilerTokenType(cs) != NK_TOKENTYPE_INVALID &&
            vmCompilerTokenType(cs) != NK_TOKENTYPE_PAREN_CLOSE)
        {
            // Make a new node for this parameter.
            struct NKExpressionAstNode *thisParamNode =
                nkiMalloc(cs->vm, sizeof(struct NKExpressionAstNode));
            memset(thisParamNode, 0, sizeof(*thisParamNode));
            thisParamNode->opOrValue = postfixNode->opOrValue;

            // Parse the expression.
            thisParamNode->children[0] = parseExpression(cs);

            // Add us to the end of the chain.
            lastParamNode->children[1] = thisParamNode;
            lastParamNode = thisParamNode;

            // Error-check.
            if(!thisParamNode->children[0]) {
                PARSE_ERROR("Function parameter subexpression parse failure.");
                dbgPop();
                nkiCompilerPopRecursion(cs);
                return false;
            }

            // Skip commas.
            if(vmCompilerTokenType(cs) == NK_TOKENTYPE_COMMA) {
                vmCompilerNextToken(cs);
            } else if(vmCompilerTokenType(cs) == NK_TOKENTYPE_PAREN_CLOSE) {
                // This is okay. It just means we're at the end.
            } else {
                // Anything else is bad.
                PARSE_ERROR("Expected ',' or ')' in function parameter parsing.");
                dbgPop();
                nkiCompilerPopRecursion(cs);
                return false;
            }

            argCount++;
        }
    }
    dbgPop();

    if(!vmCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_PAREN_CLOSE)) {
        nkiCompilerPopRecursion(cs);
        return false;
    }

    dbgWriteLine("Function call operator complete.");

    nkiCompilerPopRecursion(cs);
    return true;
}

struct NKExpressionAstNode *parseExpression(struct NKCompilerState *cs)
{
    struct NKToken **currentToken = &cs->currentToken;
    struct NKExpressionAstNode *opStack = NULL;
    struct NKExpressionAstNode *valueStack = NULL;

    if(!nkiCompilerPushRecursion(cs)) {
        return NULL;
    }

    while(!isExpressionEndingToken(*currentToken)) {

        struct NKExpressionAstNode *lastPrefixOp   = NULL;
        struct NKExpressionAstNode *firstPrefixOp  = NULL;
        struct NKExpressionAstNode *lastPostfixOp  = NULL;
        struct NKExpressionAstNode *firstPostfixOp = NULL;
        struct NKExpressionAstNode *valueNode      = NULL;

        // Deal with prefix operators. Build up a list of them.
        while(isPrefixOperator(*currentToken)) {

            struct NKExpressionAstNode *prefixNode =
                nkiMalloc(cs->vm, sizeof(struct NKExpressionAstNode));
            memset(prefixNode, 0, sizeof(*prefixNode));

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

            dbgWriteLine("Parse prefix operator: %s", (*currentToken)->str);

            vmCompilerNextToken(cs);
        }

        // Parse a value or sub-expression.
        if(vmCompilerTokenType(cs) == NK_TOKENTYPE_PAREN_OPEN) {

            // Parse sub-expression.
            dbgWriteLine("Parse sub-expression.");
            dbgPush();
            vmCompilerNextToken(cs);
            valueNode = parseExpression(cs);
            dbgPop();

            // Error check.
            if(!valueNode) {
                PARSE_ERROR("Subexpression parse failure.");
                CLEANUP_INLOOP();
                nkiCompilerPopRecursion(cs);
                return NULL;
            }

            // Make sure we ended on a closing parenthesis.
            if(!(*currentToken) || !isSubexpressionEndingToken(*currentToken)) {
                PARSE_ERROR("Bad expression end.");
                CLEANUP_INLOOP();
                nkiCompilerPopRecursion(cs);
                return NULL;
            }

            dbgWriteLine("Back from sub-expression. Current token: %s", (*currentToken)->str);

            // Skip expression-ending token.
            vmCompilerNextToken(cs);

        } else {

            // Parse the actual value.
            if(!isExpressionEndingToken(*currentToken)) {

                valueNode = nkiMalloc(cs->vm, sizeof(struct NKExpressionAstNode));
                memset(valueNode, 0, sizeof(*valueNode));
                valueNode->opOrValue = *currentToken;

                dbgWriteLine("Parse value: %s", (*currentToken)->str);
                vmCompilerNextToken(cs);

            } else {

                // Prefix operator with no value. Why?
                // TODO: Raise error flag.
                PARSE_ERROR("Prefix with no value.");
                CLEANUP_INLOOP();
                nkiCompilerPopRecursion(cs);
                return NULL;
            }
        }

        // Deal with postfix operators.
        while(isPostfixOperator(*currentToken)) {

            struct NKExpressionAstNode *postfixNode =
                nkiMalloc(cs->vm, sizeof(struct NKExpressionAstNode));
            memset(postfixNode, 0, sizeof(*postfixNode));

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

            dbgWriteLine("Parse postfix operator: %s", (*currentToken)->str);

            if(vmCompilerTokenType(cs) == NK_TOKENTYPE_DOT) {

                // Handle index-into, or function call with "self" param.

                EXPECT_AND_SKIP(NK_TOKENTYPE_DOT);

                if(vmCompilerTokenType(cs) == NK_TOKENTYPE_IDENTIFIER) {

                    struct NKExpressionAstNode *identifierStringNode =
                        nkiMalloc(cs->vm, sizeof(struct NKExpressionAstNode));

                    struct NKExpressionAstNode *indexIntoNode = postfixNode;

                    memset(identifierStringNode, 0, sizeof(*identifierStringNode));
                    identifierStringNode->opOrValue = nkiMalloc(cs->vm, sizeof(struct NKToken));
                    identifierStringNode->opOrValue->type = NK_TOKENTYPE_STRING;
                    identifierStringNode->opOrValue->str = nkiStrdup(cs->vm, cs->currentToken->str);
                    identifierStringNode->opOrValue->next = NULL;
                    identifierStringNode->opOrValue->lineNumber = cs->currentToken->lineNumber;
                    identifierStringNode->ownedToken = true;

                    indexIntoNode->opOrValue = nkiMalloc(cs->vm, sizeof(struct NKToken));
                    indexIntoNode->opOrValue->type = NK_TOKENTYPE_BRACKET_OPEN;
                    indexIntoNode->opOrValue->str = nkiStrdup(cs->vm, "[");
                    indexIntoNode->opOrValue->next = NULL;
                    indexIntoNode->opOrValue->lineNumber = cs->currentToken->lineNumber;
                    indexIntoNode->ownedToken = true;
                    indexIntoNode->children[1] = identifierStringNode;

                    EXPECT_AND_SKIP(NK_TOKENTYPE_IDENTIFIER);

                    // Now see if this is a function call.
                    if(vmCompilerTokenType(cs) == NK_TOKENTYPE_PAREN_OPEN) {

                        struct NKExpressionAstNode *functionCallNode =
                            nkiMalloc(cs->vm, sizeof(struct NKExpressionAstNode));

                        functionCallNode->opOrValue = nkiMalloc(cs->vm, sizeof(struct NKToken));
                        functionCallNode->opOrValue->type = NK_TOKENTYPE_PAREN_OPEN;
                        functionCallNode->opOrValue->str = nkiStrdup(cs->vm, "(");
                        functionCallNode->opOrValue->next = NULL;
                        functionCallNode->opOrValue->lineNumber = cs->currentToken->lineNumber;
                        functionCallNode->ownedToken = true;
                        functionCallNode->children[0] = indexIntoNode;
                        functionCallNode->children[1] = NULL;

                        if(!functionCallNode->opOrValue->str ||
                            !parseFunctioncall(functionCallNode, cs))
                        {
                            nkiFree(cs->vm, functionCallNode->opOrValue->str);
                            nkiFree(cs->vm, functionCallNode);
                            CLEANUP_INLOOP();
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

                        // assert(postfixNode->children[0]);
                        // assert(firstPostfixOp->children[0]);
                    }

                } else {
                    PARSE_ERROR("Expected identifier after '.'.");
                    CLEANUP_INLOOP();
                    nkiCompilerPopRecursion(cs);
                    return NULL;
                }

            } else if(vmCompilerTokenType(cs) == NK_TOKENTYPE_BRACKET_OPEN) {

                // Handle index-into operator.

                dbgWriteLine("Handling index-into operator.");
                dbgPush();
                vmCompilerNextToken(cs);
                postfixNode->children[1] = parseExpression(cs);
                dbgPop();

                // Error-check.
                if(!postfixNode->children[1]) {
                    PARSE_ERROR("Index subexpression parse failure.");
                    CLEANUP_INLOOP();
                    nkiCompilerPopRecursion(cs);
                    return NULL;
                }

                EXPECT_AND_SKIP(NK_TOKENTYPE_BRACKET_CLOSE);

                dbgWriteLine("Index-into operator complete.");

            } else if(vmCompilerTokenType(cs) == NK_TOKENTYPE_PAREN_OPEN) {

                // Handle function call "operator".
                if(!parseFunctioncall(postfixNode, cs)) {
                    CLEANUP_INLOOP();
                    nkiCompilerPopRecursion(cs);
                    return NULL;
                }

            } else {

                struct NKDynString *str =
                    nkiDynStrCreate(cs->vm, "Unknown postfix operator: ");
                nkiDynStrAppend(str, vmCompilerTokenString(cs));
                PARSE_ERROR(str->data);
                nkiDynStrDelete(str);
                CLEANUP_INLOOP();
                nkiCompilerPopRecursion(cs);
                return NULL;
            }
        }

        // Attach prefix and postfix operations.
        if(firstPostfixOp) {
            assert(firstPostfixOp->children[0] == NULL);
            firstPostfixOp->children[0] = valueNode;
            valueNode = lastPostfixOp;
            lastPostfixOp = NULL;
        }
        if(lastPrefixOp) {
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
        if(isExpressionEndingToken(cs->currentToken)) {
            dbgWriteLine("Done parsing expression and maybe statement.");
            break;
        }

        // Not done yet. Parse the next operator.
        dbgWriteLine("Parse operator: %s", vmCompilerTokenString(cs));

        // Make sure this is even something valid.
        if(getPrecedence((*currentToken)->type) == -1) {
            struct NKDynString *str =
                nkiDynStrCreate(cs->vm, "Unknown operator: ");
            nkiDynStrAppend(str, vmCompilerTokenString(cs));
            PARSE_ERROR(str->data);
            nkiDynStrDelete(str);
            CLEANUP_INLOOP();
            nkiCompilerPopRecursion(cs);
            return NULL;
        }

        // Attempt to reduce.
        if(opStack) {

            if(getPrecedence(vmCompilerTokenType(cs)) >=
                getPrecedence(opStack->opOrValue->type))
            {
                if(!reduce(&opStack, &valueStack)) {
                    PARSE_ERROR("Expression parse failure.");
                    CLEANUP_INLOOP();
                    nkiCompilerPopRecursion(cs);
                    return NULL;
                }
                dbgWriteLine("Reduced!");
            } else {
                dbgWriteLine(
                    "We should NOT reduce! %s <= %s",
                    vmCompilerTokenString(cs),
                    opStack->opOrValue->str);
            }
        }

        // Push this operation onto the stack now that we've cleared
        // out everthing of a lower precedence.
        {
            struct NKExpressionAstNode *astNode =
                nkiMalloc(cs->vm, sizeof(struct NKExpressionAstNode));
            memset(astNode, 0, sizeof(*astNode));
            astNode->stackNext = opStack;
            astNode->opOrValue = cs->currentToken;
            opStack = astNode;
        }

        vmCompilerNextToken(cs);
    }

    // Reduce all remaining operations.
    while(opStack) {
        if(!reduce(&opStack, &valueStack)) {
            PARSE_ERROR("Expression parse failure.");
            CLEANUP_OUTER();
            nkiCompilerPopRecursion(cs);
            return NULL;
        }
        dbgWriteLine("Reduced at end!");
    }

    if(!valueStack) {
        PARSE_ERROR("No value to parse in expression.");
        CLEANUP_OUTER();
        nkiCompilerPopRecursion(cs);
        return NULL;
    }

    // Check that stacks are empty.
    if(opStack || valueStack->stackNext) {
        // TODO: Maybe make this a normal error, but be able to clean
        // up afterwards?
        assert(0);
    }

    dbgWriteLine("remaining values...");
    {
        struct NKExpressionAstNode *n = valueStack;
        while(n) {
            dumpExpressionAstNode(n);
            n = n->stackNext;
        }
    }

    dbgWriteLine("remaining ops...\n");
    {
        struct NKExpressionAstNode *n = opStack;
        while(n) {
            dumpExpressionAstNode(n);
            n = n->stackNext;
        }
    }

    dbgWriteLine("Expression parse success.");
    nkiCompilerPopRecursion(cs);
    return valueStack;
}

bool emitFetchVariable(
    struct NKCompilerState *cs,
    const char *name,
    struct NKExpressionAstNode *node)
{
    struct NKCompilerStateContextVariable *var =
        lookupVariable(cs, name);

    if(!var) {
        return false;
    }

    if(var->isGlobal) {

        // Positive values for global variables (absolute stack
        // position).
        nkiEmitPushLiteralInt(cs, var->stackPos, true);

        dbgWriteLine("Looked up %s: Global at %d", name, var->stackPos);

    } else {

        // Negative values for local variables (stack position -
        // value).
        int32_t fetchStackPos = var->stackPos - cs->context->stackFrameOffset;
        nkiEmitPushLiteralInt(cs, fetchStackPos, true);

        dbgWriteLine("Looked up %s: Local at %d", name, var->stackPos);
    }

    nkiAddInstructionSimple(
        cs, NK_OP_STACKPEEK, false);

    dbgWriteLine("GET VAR: %s", name);

    return true;
}

bool emitSetVariable(
    struct NKCompilerState *cs,
    const char *name,
    struct NKExpressionAstNode *node)
{
    struct NKCompilerStateContextVariable *var =
        lookupVariable(cs, name);

    if(!var) {
        return false;
    }

    if(var->isGlobal) {

        // Positive values for global variables (absolute stack
        // position).
        nkiEmitPushLiteralInt(cs, var->stackPos, true);

    } else {

        // Negative values for local variables (stack position -
        // value).
        nkiEmitPushLiteralInt(cs,
            var->stackPos - cs->context->stackFrameOffset, true);

    }

    nkiAddInstructionSimple(cs, NK_OP_STACKPOKE, true);

    dbgWriteLine("SET VAR: %s", name);

    return true;
}

bool emitExpression(struct NKCompilerState *cs, struct NKExpressionAstNode *node);

bool emitExpressionAssignment(struct NKCompilerState *cs, struct NKExpressionAstNode *node)
{
    if(!nkiCompilerPushRecursion(cs)) {
        return false;
    }

    if(!node->children[1]) {
        vmCompilerAddError(
            cs, "No RValue to assign in assignment.");
        nkiCompilerPopRecursion(cs);
        return false;
    }

    if(!node->children[0]) {
        vmCompilerAddError(
            cs, "No LValue to assign in assignment.");
        nkiCompilerPopRecursion(cs);
        return false;
    }

    // Emit the value we want to assign.
    emitExpression(cs, node->children[1]);

    // Emit the assignment itself, which will depend on what we're
    // assigning to.

    switch(node->children[0]->opOrValue->type) {

        // Variable assignment.
        case NK_TOKENTYPE_IDENTIFIER: {
            emitSetVariable(
                cs,
                node->children[0]->opOrValue->str,
                node);
        } break;

        // Object index.
        case NK_TOKENTYPE_BRACKET_OPEN: {

            // Emit the thing we're going to assign to.
            emitExpression(cs, node->children[0]->children[0]); // Object id
            emitExpression(cs, node->children[0]->children[1]); // Index

            addInstructionSimple(cs, NK_OP_OBJECTFIELDSET);
            cs->context->stackFrameOffset -= 2;
        } break;

        default: {
            struct NKDynString *dynStr =
                nkiDynStrCreate(cs->vm, "Operator or value cannot be used to generate an LValue: ");
            nkiDynStrAppend(dynStr, node->children[0]->opOrValue->str);
            vmCompilerAddError(
                cs, dynStr->data);
            nkiDynStrDelete(dynStr);
            nkiCompilerPopRecursion(cs);
            return false;
        } break;

    }

    nkiCompilerPopRecursion(cs);
    return true;
}

bool emitExpression(struct NKCompilerState *cs, struct NKExpressionAstNode *node)
{
    struct NKInstruction inst;
    uint32_t i;

    if(!nkiCompilerPushRecursion(cs)) {
        return false;
    }

    // Assignments are special, because we need to evaluate the left
    // side as an LValue.
    if(node->opOrValue->type == NK_TOKENTYPE_ASSIGNMENT) {
        nkiCompilerPopRecursion(cs);
        return emitExpressionAssignment(cs, node);
    }

    // Emit children.
    for(i = 0; i < 2; i++) {
        if(node->children[i]) {
            if(!emitExpression(cs, node->children[i])) {
                nkiCompilerPopRecursion(cs);
                return false;
            }
        }
    }

    memset(&inst, 0, sizeof(inst));

    switch(node->opOrValue->type) {

        case NK_TOKENTYPE_INTEGER:
            nkiEmitPushLiteralInt(cs, atoi(node->opOrValue->str), true);
            break;

        case NK_TOKENTYPE_FLOAT:
            nkiEmitPushLiteralFloat(cs, atof(node->opOrValue->str), true);
            break;

        case NK_TOKENTYPE_STRING:
            nkiEmitPushLiteralString(cs, node->opOrValue->str, true);
            break;

        case NK_TOKENTYPE_NEWOBJECT:
            nkiAddInstructionSimple(cs, NK_OP_CREATEOBJECT, true);
            break;

        case NK_TOKENTYPE_NIL:
            nkiEmitPushNil(cs, true);
            break;

        case NK_TOKENTYPE_PLUS:
            nkiAddInstructionSimple(cs, NK_OP_ADD, true);
            break;

        case NK_TOKENTYPE_MINUS:
            if(!node->children[1]) {
                nkiAddInstructionSimple(cs, NK_OP_NEGATE, true);
            } else {
                nkiAddInstructionSimple(cs, NK_OP_SUBTRACT, true);
            }
            break;

        case NK_TOKENTYPE_MULTIPLY:
            nkiAddInstructionSimple(cs, NK_OP_MULTIPLY, true);
            break;

        case NK_TOKENTYPE_DIVIDE:
            nkiAddInstructionSimple(cs, NK_OP_DIVIDE, true);
            break;

        case NK_TOKENTYPE_MODULO:
            nkiAddInstructionSimple(cs, NK_OP_MODULO, true);
            break;

        case NK_TOKENTYPE_GREATERTHAN:
            nkiAddInstructionSimple(cs, NK_OP_GREATERTHAN, true);
            break;

        case NK_TOKENTYPE_LESSTHAN:
            nkiAddInstructionSimple(cs, NK_OP_LESSTHAN, true);
            break;

        case NK_TOKENTYPE_GREATERTHANOREQUAL:
            nkiAddInstructionSimple(cs, NK_OP_GREATERTHANOREQUAL, true);
            break;

        case NK_TOKENTYPE_LESSTHANOREQUAL:
            nkiAddInstructionSimple(cs, NK_OP_LESSTHANOREQUAL, true);
            break;

        case NK_TOKENTYPE_EQUAL:
            nkiAddInstructionSimple(cs, NK_OP_EQUAL, true);
            break;

        case NK_TOKENTYPE_NOTEQUAL:
            nkiAddInstructionSimple(cs, NK_OP_NOTEQUAL, true);
            break;

        case NK_TOKENTYPE_EQUALWITHSAMETYPE:
            nkiAddInstructionSimple(cs, NK_OP_EQUALWITHSAMETYPE, true);
            break;

        case NK_TOKENTYPE_NOT:
            nkiAddInstructionSimple(cs, NK_OP_NOT, true);
            break;

        case NK_TOKENTYPE_IDENTIFIER:
            emitFetchVariable(cs, node->opOrValue->str, node);
            break;

        case NK_TOKENTYPE_AND:
            nkiAddInstructionSimple(cs, NK_OP_AND, true);
            break;

        case NK_TOKENTYPE_OR:
            nkiAddInstructionSimple(cs, NK_OP_OR, true);
            break;

        case NK_TOKENTYPE_BRACKET_OPEN:
            nkiAddInstructionSimple(cs, NK_OP_OBJECTFIELDGET, true);
            break;

        case NK_TOKENTYPE_PAREN_OPEN:
        case NK_TOKENTYPE_FUNCTIONCALL_WITHSELF:
        {
            // Function calls.

            if(node->isRootFunctionCallNode) {

                // Count up the arguments.
                uint32_t argumentCount = 0;
                struct NKExpressionAstNode *argumentAstNode = node->children[1];
                while(argumentAstNode) {
                    argumentAstNode = argumentAstNode->children[1];
                    argumentCount++;
                }

                dbgWriteLine("Emitting function call with arguments: %u", argumentCount);
                nkiEmitPushLiteralInt(cs, argumentCount, false);

                if(node->opOrValue->type == NK_TOKENTYPE_FUNCTIONCALL_WITHSELF) {
                    addInstructionSimple(cs, NK_OP_PREPARESELFCALL);
                    argumentCount++;
                }

                addInstructionSimple(cs, NK_OP_CALL);

                cs->context->stackFrameOffset -= argumentCount;
            }

        } break;

        case NK_TOKENTYPE_INDEXINTO_NOPOP:
            addInstructionSimple(cs, NK_OP_OBJECTFIELDGET_NOPOP);
            break;

        default: {
            struct NKDynString *dynStr =
                nkiDynStrCreate(cs->vm, "Unknown value or operator in emitExpression: ");
            nkiDynStrAppend(dynStr, node->opOrValue->str);
            nkiAddError(
                cs->vm,
                node->opOrValue->lineNumber,
                dynStr->data);
            nkiDynStrDelete(dynStr);
            nkiCompilerPopRecursion(cs);
            return false;
        } break;
    }

    nkiCompilerPopRecursion(cs);
    return true;
}

struct NKExpressionAstNode *cloneExpressionTree(
    struct NKCompilerState *cs,
    struct NKExpressionAstNode *node)
{
    struct NKExpressionAstNode *newNode;

    if(!node) {
        return NULL;
    }

    newNode = nkiMalloc(cs->vm, sizeof(struct NKExpressionAstNode));
    memset(newNode, 0, sizeof(*newNode));

    newNode->ownedToken = node->ownedToken;
    if(node->ownedToken) {
        newNode->opOrValue = nkiMalloc(cs->vm, sizeof(struct NKToken));
        memset(newNode->opOrValue, 0, sizeof(struct NKToken));
        newNode->opOrValue->type = node->opOrValue->type;
        newNode->opOrValue->str = nkiStrdup(cs->vm, node->opOrValue->str);
        newNode->opOrValue->lineNumber = node->opOrValue->lineNumber;
    } else {
        newNode->opOrValue = node->opOrValue;
    }

    newNode->isRootFunctionCallNode = node->isRootFunctionCallNode;

    newNode->children[0] = cloneExpressionTree(cs, node->children[0]);
    newNode->children[1] = cloneExpressionTree(cs, node->children[1]);

    return newNode;
}

void expandIncrementsAndDecrements(
    struct NKCompilerState *cs,
    struct NKExpressionAstNode *node)
{
    if(!node) {
        return;
    }

    if(!nkiCompilerPushRecursion(cs)) {
        return;
    }

    if(node->opOrValue->type == NK_TOKENTYPE_INCREMENT ||
        node->opOrValue->type == NK_TOKENTYPE_DECREMENT)
    {
        assert(node->children[0]);
        assert(!node->children[1]);

        {
            struct NKExpressionAstNode *lvalueNode = node->children[0];
            struct NKExpressionAstNode *rvalueNode1 =
                cloneExpressionTree(cs, node->children[0]);
            struct NKExpressionAstNode *additionNode =
                nkiMalloc(cs->vm, sizeof(struct NKExpressionAstNode));
            struct NKExpressionAstNode *literalOneNode =
                nkiMalloc(cs->vm, sizeof(struct NKExpressionAstNode));
            struct NKToken *oldToken = node->opOrValue;
            bool wasOwningToken = node->ownedToken;

            struct NKToken *literalOneToken = nkiMalloc(cs->vm, sizeof(struct NKToken));
            struct NKToken *additionToken = nkiMalloc(cs->vm, sizeof(struct NKToken));
            struct NKToken *assignmentToken = nkiMalloc(cs->vm, sizeof(struct NKToken));
            char *oneTokenStr = nkiStrdup(cs->vm, "1");
            char *addTokenStr = nkiStrdup(cs->vm, oldToken->type == NK_TOKENTYPE_INCREMENT ? "+" : "-");
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
            literalOneNode->ownedToken = true;
            literalOneNode->isRootFunctionCallNode = false;

            // Generate a node that just adds 1 to the value.
            additionNode->opOrValue = additionToken;
            additionNode->opOrValue->type =
                oldToken->type == NK_TOKENTYPE_INCREMENT ? NK_TOKENTYPE_PLUS : NK_TOKENTYPE_MINUS;
            additionNode->opOrValue->str = addTokenStr;
            additionNode->opOrValue->lineNumber = node->opOrValue->lineNumber;
            additionNode->opOrValue->next = NULL;
            additionNode->children[0] = rvalueNode1;
            additionNode->children[1] = literalOneNode;
            additionNode->stackNext = NULL;
            additionNode->ownedToken = true;
            additionNode->isRootFunctionCallNode = false;

            // Generate a node that assigns the added value back to
            // the original value. This just turns this node into an
            // assignment node.
            node->opOrValue = assignmentToken;
            node->opOrValue->type = NK_TOKENTYPE_ASSIGNMENT;
            node->opOrValue->str = assignTokenStr;
            node->opOrValue->lineNumber = oldToken->lineNumber;
            node->opOrValue->next = NULL;
            node->children[0] = lvalueNode;
            node->children[1] = additionNode;
            node->stackNext = NULL;
            node->ownedToken = true;
            node->isRootFunctionCallNode = false;

            if(wasOwningToken) {
                deleteToken(cs->vm, oldToken);
            }
        }

    } else {

        if(node->children[0]) {
            expandIncrementsAndDecrements(cs, node->children[0]);
        }

        if(node->children[1]) {
            expandIncrementsAndDecrements(cs, node->children[1]);
        }
    }

    nkiCompilerPopRecursion(cs);
}

struct NKExpressionAstNode *compileExpressionWithoutEmit(struct NKCompilerState *cs)
{
    struct NKExpressionAstNode *node;

    if(!nkiCompilerPushRecursion(cs)) {
        return NULL;
    }

    node = parseExpression(cs);

    if(node) {
        expandIncrementsAndDecrements(cs, node);
        optimizeConstants(cs->vm, &node);
    }

    if(vmGetErrorCount(cs->vm)) {
        deleteExpressionNode(cs->vm, node);
        nkiCompilerPopRecursion(cs);
        return NULL;
    }

    nkiCompilerPopRecursion(cs);
    return node;
}

bool compileExpression(struct NKCompilerState *cs)
{
    struct NKExpressionAstNode *node;

    if(!nkiCompilerPushRecursion(cs)) {
        return false;
    }

    node = compileExpressionWithoutEmit(cs);

    if(node) {
        bool ret;
        ret = emitExpression(cs, node);
        deleteExpressionNode(cs->vm, node);
        nkiCompilerPopRecursion(cs);
        return ret;
    }

    nkiCompilerPopRecursion(cs);
    return false;
}


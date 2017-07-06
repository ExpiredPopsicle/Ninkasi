#include "common.h"

void deleteExpressionNode(struct ExpressionAstNode *node)
{
    if(!node) {
        return;
    }

    if(node->ownedToken) {
        deleteToken(node->opOrValue);
        node->opOrValue = NULL;
    }

    if(node->children[0]) {
        deleteExpressionNode(node->children[0]);
    }
    if(node->children[1]) {
        deleteExpressionNode(node->children[1]);
    }

    free(node);
}

void dumpExpressionAstNode(struct ExpressionAstNode *node)
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

    } else if(node->opOrValue->type == TOKENTYPE_BRACKET_OPEN) {

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


bool isSubexpressionEndingToken(struct Token *token)
{
    return !token ||
        token->type == TOKENTYPE_PAREN_CLOSE ||
        token->type == TOKENTYPE_BRACKET_CLOSE ||
        token->type == TOKENTYPE_COMMA;
}

bool isExpressionEndingToken(struct Token *token)
{
    return !token || isSubexpressionEndingToken(token) ||
        token->type == TOKENTYPE_SEMICOLON;
}

bool isPrefixOperator(struct Token *token)
{
    return token && (
        token->type == TOKENTYPE_MINUS ||
        token->type == TOKENTYPE_DECREMENT ||
        token->type == TOKENTYPE_INCREMENT ||
        token->type == TOKENTYPE_NOT);
}

bool isPostfixOperator(struct Token *token)
{
    return token && (
        token->type == TOKENTYPE_BRACKET_OPEN ||
        token->type == TOKENTYPE_PAREN_OPEN);
}

int32_t getPrecedence(enum TokenType t)
{
    // Reference:
    // http://en.cppreference.com/w/cpp/language/operator_precedence

    switch(t) {
        case TOKENTYPE_MULTIPLY:
        case TOKENTYPE_DIVIDE:
        case TOKENTYPE_MODULO:
            return 5;
        case TOKENTYPE_PLUS:
        case TOKENTYPE_MINUS:
            return 6;
        case TOKENTYPE_GREATERTHAN:
        case TOKENTYPE_LESSTHAN:
        case TOKENTYPE_GREATERTHANOREQUAL:
        case TOKENTYPE_LESSTHANOREQUAL:
            return 8;
        case TOKENTYPE_EQUAL:
        case TOKENTYPE_NOTEQUAL:
        case TOKENTYPE_EQUALWITHSAMETYPE:
            return 9;
        case TOKENTYPE_AND:
            return 13;
        case TOKENTYPE_OR:
            return 14;
        case TOKENTYPE_ASSIGNMENT:
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

#define CLEANUP_OUTER()                                             \
    do {                                                            \
        while(opStack) {                                            \
            struct ExpressionAstNode *next = opStack->stackNext;    \
            deleteExpressionNode(opStack);                          \
            opStack = next;                                         \
        }                                                           \
        while(valueStack) {                                         \
            struct ExpressionAstNode *next = valueStack->stackNext; \
            deleteExpressionNode(valueStack);                       \
            valueStack = next;                                      \
        }                                                           \
    } while(0)

#define CLEANUP_INLOOP()                        \
    do {                                        \
        CLEANUP_OUTER();                        \
        deleteExpressionNode(firstPrefixOp);    \
        deleteExpressionNode(lastPostfixOp);    \
        deleteExpressionNode(valueNode);        \
    } while(0)

#define EXPECT_AND_SKIP(x)                          \
    do {                                            \
        if(vmCompilerTokenType(cs) != (x)) {        \
            struct DynString *errStr =              \
                dynStrCreate("Unexpected token: "); \
            dynStrAppend(                           \
                errStr,                             \
                vmCompilerTokenString(cs));         \
            PARSE_ERROR(errStr->data);              \
            dynStrDelete(errStr);                   \
            CLEANUP_INLOOP();                       \
            return NULL;                            \
        }                                           \
        vmCompilerNextToken(cs);                    \
    } while(0)

#define MAKE_OP(x)                                  \
    struct ExpressionAstNode *astNode =             \
        malloc(sizeof(struct ExpressionAstNode));   \
    memset(astNode, 0, sizeof(*astNode));           \
    astNode->stackNext = opStack;                   \
    astNode->opOrValue = x;

#define PUSH_OP(x)                              \
    do {                                        \
        MAKE_OP(x);                             \
        opStack = astNode;                      \
    } while(0)

bool reduce(
    struct ExpressionAstNode **opStack,
    struct ExpressionAstNode **valueStack)
{
    struct ExpressionAstNode *valNode1 = (*valueStack)->stackNext;
    struct ExpressionAstNode *valNode2 = (*valueStack);
    struct ExpressionAstNode *opNode   = (*opStack);

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

struct ExpressionAstNode *parseExpression(struct CompilerState *cs)
{
    struct Token **currentToken = &cs->currentToken;
    struct ExpressionAstNode *opStack = NULL;
    struct ExpressionAstNode *valueStack = NULL;

    while(!isExpressionEndingToken(*currentToken)) {

        struct ExpressionAstNode *lastPrefixOp   = NULL;
        struct ExpressionAstNode *firstPrefixOp  = NULL;
        struct ExpressionAstNode *lastPostfixOp  = NULL;
        struct ExpressionAstNode *firstPostfixOp = NULL;
        struct ExpressionAstNode *valueNode      = NULL;

        // Deal with prefix operators. Build up a list of them.
        while(isPrefixOperator(*currentToken)) {

            struct ExpressionAstNode *prefixNode = malloc(sizeof(struct ExpressionAstNode));
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
        if(vmCompilerTokenType(cs) == TOKENTYPE_PAREN_OPEN) {

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
                return NULL;
            }

            // Make sure we ended on a closing parenthesis.
            if(!(*currentToken) || !isSubexpressionEndingToken(*currentToken)) {
                PARSE_ERROR("Bad expression end.");
                CLEANUP_INLOOP();
                return NULL;
            }

            dbgWriteLine("Back from sub-expression. Current token: %s", (*currentToken)->str);

            // Skip expression-ending token.
            vmCompilerNextToken(cs);

        } else {

            // Parse the actual value.
            if(!isExpressionEndingToken(*currentToken)) {

                valueNode = malloc(sizeof(struct ExpressionAstNode));
                memset(valueNode, 0, sizeof(*valueNode));
                valueNode->opOrValue = *currentToken;

                dbgWriteLine("Parse value: %s", (*currentToken)->str);
                vmCompilerNextToken(cs);

            } else {

                // Prefix operator with no value. Why?
                // TODO: Raise error flag.
                PARSE_ERROR("Prefix with no value.");
                CLEANUP_INLOOP();
                return NULL;
            }
        }

        // Deal with postfix operators.
        while(isPostfixOperator(*currentToken)) {

            struct ExpressionAstNode *postfixNode =
                malloc(sizeof(struct ExpressionAstNode));
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

            if(vmCompilerTokenType(cs) == TOKENTYPE_BRACKET_OPEN) {

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
                    return NULL;
                }

                EXPECT_AND_SKIP(TOKENTYPE_BRACKET_CLOSE);

                dbgWriteLine("Index-into operator complete.");

            } else if(vmCompilerTokenType(cs) == TOKENTYPE_PAREN_OPEN) {

                // Handle function call "operator".
                dbgWriteLine("Handling function call operator.");

                // TODO: Keep handling expressions as long as we end
                // up on a comma.

                postfixNode->isRootFunctionCallNode = true;

                dbgPush();
                vmCompilerNextToken(cs);
                {
                    struct ExpressionAstNode *lastParamNode = postfixNode;
                    uint32_t argCount = 0;

                    while(
                        vmCompilerTokenType(cs) != TOKENTYPE_INVALID &&
                        vmCompilerTokenType(cs) != TOKENTYPE_PAREN_CLOSE)
                    {
                        // Make a new node for this parameter.
                        struct ExpressionAstNode *thisParamNode =
                            malloc(sizeof(struct ExpressionAstNode));
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
                            CLEANUP_INLOOP();
                            dbgPop();
                            return NULL;
                        }

                        // Skip commas.
                        if(vmCompilerTokenType(cs) == TOKENTYPE_COMMA) {
                            vmCompilerNextToken(cs);
                        } else if(vmCompilerTokenType(cs) == TOKENTYPE_PAREN_CLOSE) {
                            // This is okay. It just means we're at the end.
                        } else {
                            // Anything else is bad.
                            PARSE_ERROR("Expected ',' or ')' in function parameter parsing.");
                            CLEANUP_INLOOP();
                            dbgPop();
                            return NULL;
                        }

                        argCount++;
                    }
                }
                dbgPop();

                EXPECT_AND_SKIP(TOKENTYPE_PAREN_CLOSE);

                dbgWriteLine("Function call operator complete.");

            } else {

                struct DynString *str =
                    dynStrCreate("Unknown postfix operator: ");
                dynStrAppend(str, vmCompilerTokenString(cs));
                PARSE_ERROR(str->data);
                dynStrDelete(str);
                CLEANUP_INLOOP();
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
            struct DynString *str =
                dynStrCreate("Unknown operator: ");
            dynStrAppend(str, vmCompilerTokenString(cs));
            PARSE_ERROR(str->data);
            dynStrDelete(str);
            CLEANUP_INLOOP();
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

        PUSH_OP(cs->currentToken);

        vmCompilerNextToken(cs);
    }

    // Reduce all remaining operations.
    while(opStack) {
        if(!reduce(&opStack, &valueStack)) {
            PARSE_ERROR("Expression parse failure.");
            CLEANUP_OUTER();
            return NULL;
        }
        dbgWriteLine("Reduced at end!");
    }

    if(!valueStack) {
        PARSE_ERROR("No value to parse in expression.");
        CLEANUP_OUTER();
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
        struct ExpressionAstNode *n = valueStack;
        while(n) {
            dumpExpressionAstNode(n);
            n = n->stackNext;
        }
    }

    dbgWriteLine("remaining ops...\n");
    {
        struct ExpressionAstNode *n = opStack;
        while(n) {
            dumpExpressionAstNode(n);
            n = n->stackNext;
        }
    }

    dbgWriteLine("Expression parse success.");
    return valueStack;
}

bool emitFetchVariable(
    struct CompilerState *cs,
    const char *name,
    struct ExpressionAstNode *node)
{
    struct CompilerStateContextVariable *var =
        lookupVariable(cs, name);

    if(!var) {
        return false;
    }

    if(var->isGlobal) {

        // Positive values for global variables (absolute stack
        // position).
        emitPushLiteralInt(cs, var->stackPos);

        dbgWriteLine("Looked up %s: Global at %d", name, var->stackPos);

    } else {

        // Negative values for local variables (stack position -
        // value).
        emitPushLiteralInt(cs,
            var->stackPos - cs->context->stackFrameOffset);

        dbgWriteLine("Looked up %s: Local at %d", name, var->stackPos);
    }

    addInstructionSimple(cs, OP_STACKPEEK);
    cs->context->stackFrameOffset++;

    dbgWriteLine("GET VAR: %s", name);

    return true;
}

bool emitSetVariable(
    struct CompilerState *cs,
    const char *name,
    struct ExpressionAstNode *node)
{
    struct CompilerStateContextVariable *var =
        lookupVariable(cs, name);

    if(!var) {
        return false;
    }

    if(var->isGlobal) {

        // Positive values for global variables (absolute stack
        // position).
        emitPushLiteralInt(cs, var->stackPos);

    } else {

        // Negative values for local variables (stack position -
        // value).
        emitPushLiteralInt(cs,
            var->stackPos - cs->context->stackFrameOffset);

    }

    addInstructionSimple(cs, OP_STACKPOKE);

    dbgWriteLine("SET VAR: %s", name);

    return true;
}

bool emitExpression(struct CompilerState *cs, struct ExpressionAstNode *node);

bool emitExpressionAssignment(struct CompilerState *cs, struct ExpressionAstNode *node)
{
    if(!node->children[1]) {
        vmCompilerAddError(
            cs, "No RValue to assign in assignment.");
        return false;
    }

    if(!node->children[0]) {
        vmCompilerAddError(
            cs, "No LValue to assign in assignment.");
        return false;
    }

    // Emit the value we want to assign.
    emitExpression(cs, node->children[1]);

    // Emit the assignment itself, which will depend on what we're
    // assigning to.

    switch(node->children[0]->opOrValue->type) {

        // Variable assignment.
        case TOKENTYPE_IDENTIFIER: {
            emitSetVariable(
                cs,
                node->children[0]->opOrValue->str,
                node);
        } break;

        // TODO: Array index.

        default: {
            struct DynString *dynStr =
                dynStrCreate("Operator or value cannot be used to generate an LValue: ");
            dynStrAppend(dynStr, node->children[0]->opOrValue->str);
            vmCompilerAddError(
                cs, dynStr->data);
            dynStrDelete(dynStr);
            return false;
        } break;

    }

    return true;
}

bool emitExpression(struct CompilerState *cs, struct ExpressionAstNode *node)
{
    struct Instruction inst;
    uint32_t i;

    // Assignments are special, because we need to evaluate the left
    // side as an LValue.
    if(node->opOrValue->type == TOKENTYPE_ASSIGNMENT) {
        return emitExpressionAssignment(cs, node);
    }

    // Emit children.
    for(i = 0; i < 2; i++) {
        if(node->children[i]) {
            if(!emitExpression(cs, node->children[i])) {
                return false;
            }
        }
    }

    memset(&inst, 0, sizeof(inst));

    switch(node->opOrValue->type) {

        case TOKENTYPE_INTEGER: {
            emitPushLiteralInt(cs, atoi(node->opOrValue->str));
            cs->context->stackFrameOffset++;

            dbgWriteLine("PUSH INTEGER: %s", node->opOrValue->str);

        } break;

        case TOKENTYPE_FLOAT: {
            emitPushLiteralFloat(cs, atof(node->opOrValue->str));
            cs->context->stackFrameOffset++;

            dbgWriteLine("PUSH FLOAT: %s", node->opOrValue->str);

        } break;

        case TOKENTYPE_STRING: {
            emitPushLiteralString(cs, node->opOrValue->str);
            cs->context->stackFrameOffset++;

            dbgWriteLine("PUSH STRING: %s", node->opOrValue->str);

        } break;

        case TOKENTYPE_PLUS: {
            addInstructionSimple(cs, OP_ADD);
            cs->context->stackFrameOffset--;

            dbgWriteLine("ADD");

        } break;

        case TOKENTYPE_MINUS:
            if(!node->children[1]) {
                addInstructionSimple(cs, OP_NEGATE);
                dbgWriteLine("NEGATE");
            } else {
                addInstructionSimple(cs, OP_SUBTRACT);
                cs->context->stackFrameOffset--;
                dbgWriteLine("SUBTRACT");
            }
            break;

        case TOKENTYPE_MULTIPLY:
            addInstructionSimple(cs, OP_MULTIPLY);
            cs->context->stackFrameOffset--;
            dbgWriteLine("MULTIPLY");
            break;

        case TOKENTYPE_DIVIDE:
            addInstructionSimple(cs, OP_DIVIDE);
            cs->context->stackFrameOffset--;
            dbgWriteLine("DIVIDE");
            break;

        case TOKENTYPE_MODULO:
            addInstructionSimple(cs, OP_MODULO);
            cs->context->stackFrameOffset--;
            break;

        case TOKENTYPE_GREATERTHAN:
            addInstructionSimple(cs, OP_GREATERTHAN);
            cs->context->stackFrameOffset--;
            break;

        case TOKENTYPE_LESSTHAN:
            addInstructionSimple(cs, OP_LESSTHAN);
            cs->context->stackFrameOffset--;
            break;

        case TOKENTYPE_GREATERTHANOREQUAL:
            addInstructionSimple(cs, OP_GREATERTHANOREQUAL);
            cs->context->stackFrameOffset--;
            break;

        case TOKENTYPE_LESSTHANOREQUAL:
            addInstructionSimple(cs, OP_LESSTHANOREQUAL);
            cs->context->stackFrameOffset--;
            break;

        case TOKENTYPE_EQUAL:
            addInstructionSimple(cs, OP_EQUAL);
            cs->context->stackFrameOffset--;
            break;

        case TOKENTYPE_NOTEQUAL:
            addInstructionSimple(cs, OP_NOTEQUAL);
            cs->context->stackFrameOffset--;
            break;

        case TOKENTYPE_EQUALWITHSAMETYPE:
            addInstructionSimple(cs, OP_EQUALWITHSAMETYPE);
            cs->context->stackFrameOffset--;
            break;

        case TOKENTYPE_NOT:
            // No stack size change here.
            addInstructionSimple(cs, OP_NOT);
            break;

        case TOKENTYPE_IDENTIFIER:
            emitFetchVariable(cs, node->opOrValue->str, node);
            break;

        case TOKENTYPE_AND:
            addInstructionSimple(cs, OP_AND);
            cs->context->stackFrameOffset--;
            break;

        case TOKENTYPE_OR:
            addInstructionSimple(cs, OP_OR);
            cs->context->stackFrameOffset--;
            break;

            // TODO: Array index.

        case TOKENTYPE_PAREN_OPEN: {
            // Function calls.

            if(node->isRootFunctionCallNode) {

                // Count up the arguments.
                uint32_t argumentCount = 0;
                struct ExpressionAstNode *argumentAstNode = node->children[1];
                while(argumentAstNode) {
                    argumentAstNode = argumentAstNode->children[1];
                    argumentCount++;
                }

                dbgWriteLine("Emitting function call with arguments: %u", argumentCount);
                emitPushLiteralInt(cs, argumentCount);
                addInstructionSimple(cs, OP_CALL);

                cs->context->stackFrameOffset -= argumentCount;
            }

        } break;

        default: {
            struct DynString *dynStr =
                dynStrCreate("Unknown value or operator in emitExpression: ");
            dynStrAppend(dynStr, node->opOrValue->str);
            errorStateAddError(
                &cs->vm->errorState,
                node->opOrValue->lineNumber,
                dynStr->data);
            dynStrDelete(dynStr);
            return false;
        } break;
    }


    return true;
}

struct ExpressionAstNode *cloneExpressionTree(struct ExpressionAstNode *node)
{
    struct ExpressionAstNode *newNode;

    if(!node) {
        return NULL;
    }

    newNode = malloc(sizeof(struct ExpressionAstNode));
    memset(newNode, 0, sizeof(*newNode));

    newNode->ownedToken = node->ownedToken;
    if(node->ownedToken) {
        newNode->opOrValue = malloc(sizeof(struct Token));
        memset(newNode->opOrValue, 0, sizeof(struct Token));
        newNode->opOrValue->type = node->opOrValue->type;
        newNode->opOrValue->str = strdup(node->opOrValue->str);
        newNode->opOrValue->lineNumber = node->opOrValue->lineNumber;
    } else {
        newNode->opOrValue = node->opOrValue;
    }

    newNode->isRootFunctionCallNode = node->isRootFunctionCallNode;

    newNode->children[0] = cloneExpressionTree(node->children[0]);
    newNode->children[1] = cloneExpressionTree(node->children[1]);

    return newNode;
}

void expandIncrementsAndDecrements(
    struct CompilerState *cs,
    struct ExpressionAstNode *node)
{
    if(!node) {
        return;
    }

    if(node->opOrValue->type == TOKENTYPE_INCREMENT ||
        node->opOrValue->type == TOKENTYPE_DECREMENT)
    {
        assert(node->children[0]);
        assert(!node->children[1]);

        {
            struct ExpressionAstNode *lvalueNode = node->children[0];
            struct ExpressionAstNode *rvalueNode1 = cloneExpressionTree(node->children[0]);
            struct ExpressionAstNode *additionNode = malloc(sizeof(struct ExpressionAstNode));
            struct ExpressionAstNode *literalOneNode = malloc(sizeof(struct ExpressionAstNode));
            struct Token *oldToken = node->opOrValue;
            bool wasOwningToken = node->ownedToken;

            // Generate a node for the number 1.
            literalOneNode->opOrValue = malloc(sizeof(struct Token));
            literalOneNode->opOrValue->type = TOKENTYPE_INTEGER;
            literalOneNode->opOrValue->str = strdup("1");
            literalOneNode->opOrValue->lineNumber = node->opOrValue->lineNumber;
            literalOneNode->opOrValue->next = NULL;
            literalOneNode->children[0] = NULL;
            literalOneNode->children[1] = NULL;
            literalOneNode->stackNext = NULL;
            literalOneNode->ownedToken = true;
            literalOneNode->isRootFunctionCallNode = false;

            // Generate a node that just adds 1 to the value.
            additionNode->opOrValue = malloc(sizeof(struct Token));
            additionNode->opOrValue->type =
                oldToken->type == TOKENTYPE_INCREMENT ? TOKENTYPE_PLUS : TOKENTYPE_MINUS;
            additionNode->opOrValue->str = strdup(
                oldToken->type == TOKENTYPE_INCREMENT ? "+" : "-");
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
            node->opOrValue = malloc(sizeof(struct Token));
            node->opOrValue->type = TOKENTYPE_ASSIGNMENT;
            node->opOrValue->str = strdup("=");
            node->opOrValue->lineNumber = oldToken->lineNumber;
            node->opOrValue->next = NULL;
            node->children[0] = lvalueNode;
            node->children[1] = additionNode;
            node->stackNext = NULL;
            node->ownedToken = true;
            node->isRootFunctionCallNode = false;

            if(wasOwningToken) {
                deleteToken(oldToken);
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
}

struct ExpressionAstNode *compileExpressionWithoutEmit(struct CompilerState *cs)
{
    struct ExpressionAstNode *node =
        parseExpression(cs);

    if(node) {
        expandIncrementsAndDecrements(cs, node);
        optimizeConstants(&node);
    }

    return node;
}

bool compileExpression(struct CompilerState *cs)
{
    struct ExpressionAstNode *node =
        compileExpressionWithoutEmit(cs);

    if(node) {
        bool ret;
        ret = emitExpression(cs, node);
        deleteExpressionNode(node);
        return ret;
    }

    return false;
}


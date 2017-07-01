#include "common.h"

void addInstruction(struct CompilerState *cs, struct Instruction *inst)
{
    // if(inst->opcode == OP_NOP && cs->instructionWriteIndex == 6) {
    //     assert(0);
    //     printf("Writing NOP at index %d\n", cs->instructionWriteIndex);
    // }

    if(cs->instructionWriteIndex >= cs->vm->instructionAddressMask) {

        // TODO: Remove this.
        printf("Expanding VM instruction space\n");

        // FIXME: Add a dynamic or settable memory limit.
        if(cs->vm->instructionAddressMask >= 0xfffff) {
            errorStateAddError(&cs->vm->errorState, -1, "Too many instructions.");
        }

        {
            uint32_t oldSize = cs->vm->instructionAddressMask + 1;
            uint32_t newSize = oldSize << 1;

            cs->vm->instructionAddressMask <<= 1;
            cs->vm->instructionAddressMask |= 1;
            cs->vm->instructions = realloc(
                cs->vm->instructions,
                sizeof(struct Instruction) *
                newSize);

            // Clear the new area to NOPs.
            memset(
                cs->vm->instructions + oldSize, 0,
                (newSize - oldSize) * sizeof(struct Instruction));
        }
    }

    cs->vm->instructions[cs->instructionWriteIndex] = *inst;
    cs->instructionWriteIndex++;
}

void addInstructionSimple(struct CompilerState *cs, enum Opcode opcode)
{
    struct Instruction inst;
    memset(&inst, 0, sizeof(inst));
    inst.opcode = opcode;
    addInstruction(cs, &inst);
}

void pushContext(struct CompilerState *cs)
{
    struct CompilerStateContext *newContext =
        malloc(sizeof(struct CompilerStateContext));
    memset(newContext, 0, sizeof(*newContext));
    newContext->currentFunctionId = ~(uint32_t)0;
    newContext->parent = cs->context;

    // Set stack frame offset.
    if(newContext->parent) {
        newContext->stackFrameOffset = newContext->parent->stackFrameOffset;
    }

    cs->context = newContext;

    dbgWriteLine("Pushed context");
    dbgPush();
}

void popContext(struct CompilerState *cs)
{
    struct CompilerStateContext *oldContext =
        cs->context;
    cs->context = cs->context->parent;

    // Free variable data.
    {
        struct CompilerStateContextVariable *var =
            oldContext->variables;

        while(var) {
            struct CompilerStateContextVariable *next =
                var->next;

            // Add instructions to pop variables off the stack that
            // are no longer in scope.
            if(!var->doNotPopWhenOutOfScope) {
                addInstructionSimple(cs, OP_POP);
                oldContext->stackFrameOffset--;
            }

            // Free it.
            free(var->name);
            free(var);
            var = next;

            dbgWriteLine("Variable removed.");
        }
    }

    // Free the context itself.
    free(oldContext);

    dbgPop();
    dbgWriteLine("Popped context");
}


void emitPushLiteralInt(struct CompilerState *cs, int32_t value)
{
    struct Instruction inst;

    // Add instruction.
    memset(&inst, 0, sizeof(inst));
    inst.opcode = OP_PUSHLITERAL_INT;
    addInstruction(cs, &inst);

    // Add parameter.
    memset(&inst, 0, sizeof(inst));
    inst.opData_int = value;
    addInstruction(cs, &inst);
}

void emitPushLiteralFunctionId(struct CompilerState *cs, uint32_t functionId)
{
    struct Instruction inst;

    // Add instruction.
    memset(&inst, 0, sizeof(inst));
    inst.opcode = OP_PUSHLITERAL_FUNCTIONID;
    addInstruction(cs, &inst);

    // Add parameter.
    memset(&inst, 0, sizeof(inst));
    inst.opData_functionId = functionId;
    addInstruction(cs, &inst);
}

void emitPushLiteralFloat(struct CompilerState *cs, float value)
{
    struct Instruction inst;

    // Add instruction.
    memset(&inst, 0, sizeof(inst));
    inst.opcode = OP_PUSHLITERAL_FLOAT;
    addInstruction(cs, &inst);

    // Add parameter.
    memset(&inst, 0, sizeof(inst));
    inst.opData_float = value;
    addInstruction(cs, &inst);
}

void emitPushLiteralString(struct CompilerState *cs, const char *str)
{
    struct Instruction inst;

    // Add instruction.
    memset(&inst, 0, sizeof(inst));
    inst.opcode = OP_PUSHLITERAL_STRING;
    addInstruction(cs, &inst);

    // Add string table entry data as op parameter.
    memset(&inst, 0, sizeof(inst));
    inst.opData_string =
        vmStringTableFindOrAddString(
            &cs->vm->stringTable,
            str);
    addInstruction(cs, &inst);

    // Mark this string as not garbage-collected.
    {
        struct VMString *entry = vmStringTableGetEntryById(
            &cs->vm->stringTable, inst.opData_string);
        if(entry) {
            entry->dontGC = true;
        }
    }
}

struct CompilerStateContextVariable *addVariableWithoutStackAllocation(
    struct CompilerState *cs, const char *name)
{
    struct CompilerStateContextVariable *var =
        malloc(sizeof(struct CompilerStateContextVariable));
    memset(var, 0, sizeof(*var));

    var->next = cs->context->variables;
    var->isGlobal = !cs->context->parent;
    var->name = strdup(name);
    var->stackPos = cs->context->stackFrameOffset - 1;

    cs->context->variables = var;

    dbgWriteLine("Variable added.");

    return var;
}

void addVariable(struct CompilerState *cs, const char *name)
{
    // Add an instruction to make some stack space for this variable.
    emitPushLiteralInt(cs, 0);

    cs->context->stackFrameOffset++;

    addVariableWithoutStackAllocation(cs, name);
}

#define EXPECT_AND_SKIP_STATEMENT(x)                                \
    do {                                                            \
        if(!(*currentToken) || (*currentToken)->type != (x)) {      \
            struct DynString *errStr =                              \
                dynStrCreate("Unexpected token: ");                 \
            dynStrAppend(                                           \
                errStr,                                             \
                *currentToken ? (*currentToken)->str : "<none>");   \
            errorStateAddError(                                     \
                &cs->vm->errorState,                                \
                *currentToken ? (*currentToken)->lineNumber : -1,   \
                errStr->data);                                      \
            dynStrDelete(errStr);                                   \
            return false;                                           \
        } else {                                                    \
            *currentToken = (*currentToken)->next;                  \
        }                                                           \
    } while(0)

bool compileStatement(struct CompilerState *cs, struct Token **currentToken)
{
    if(!*currentToken) {
        errorStateAddError(
            &cs->vm->errorState, -1, "Ran out of tokens to parse.");
        return false;
    }

    switch((*currentToken)->type) {

        case TOKENTYPE_VAR:
            // "var" = Variable declaration.
            return compileVariableDeclaration(cs, currentToken);

        case TOKENTYPE_FUNCTION:
            // "function" = Function definition.
            return compileFunctionDefinition(cs, currentToken);

        case TOKENTYPE_RETURN:
            // "return" = Return statement.
            return compileReturnStatement(cs, currentToken);

        case TOKENTYPE_CURLYBRACE_OPEN:
            // Curly braces mean we need to parse a block.
            return compileBlock(cs, currentToken, false);

        default:
            // Fall back to just parsing an expression.
            if(!compileExpression(cs, currentToken)) {
                return false;
            }

            EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_SEMICOLON);

            // TODO: Remove this. (Debugging output.) Replace it with
            // a OP_POP.
            {
                addInstructionSimple(cs, OP_DUMP);
                cs->context->stackFrameOffset--;
            }

            break;
    }


    return true;
}

struct CompilerStateContextVariable *lookupVariable(
    struct CompilerState *cs,
    const char *name,
    uint32_t lineNumber)
{
    struct CompilerStateContext *ctx = cs->context;
    struct CompilerStateContextVariable *var = NULL;

    while(ctx) {
        var = ctx->variables;
        while(var) {
            if(!strcmp(var->name, name)) {
                // Found it.
                ctx = NULL;
                break;
            }
            var = var->next;
        }
        if(!var) {
            ctx = ctx->parent;
        }
    }

    if(!var) {
        struct DynString *dynStr =
            dynStrCreate("Cannot find variable: ");
        dynStrAppend(dynStr, name);
        errorStateAddError(
            &cs->vm->errorState,
            lineNumber,
            dynStr->data);
        dynStrDelete(dynStr);
        return NULL;
    }

    return var;
}

bool compileBlock(struct CompilerState *cs, struct Token **currentToken, bool noBracesOrContext)
{
    if(!noBracesOrContext) {
        EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_CURLYBRACE_OPEN);
        pushContext(cs);
    }

    while(*currentToken && (*currentToken)->type != TOKENTYPE_CURLYBRACE_CLOSE) {
        if(!compileStatement(cs, currentToken)) {
            return false;
        }
    }

    if(!noBracesOrContext) {
        popContext(cs);
        EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_CURLYBRACE_CLOSE);
    }

    return true;
}

bool compileVariableDeclaration(struct CompilerState *cs, struct Token **currentToken)
{
    EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_VAR);

    if(!*currentToken || (*currentToken)->type != TOKENTYPE_IDENTIFIER) {
        errorStateAddError(
            &cs->vm->errorState,
            *currentToken ? (*currentToken)->lineNumber : -1,
            "Expected identifier in variable declaration.");
        return false;
    }

    {
        const char *variableName = (*currentToken)->str;

        EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_IDENTIFIER);

        if(*currentToken && (*currentToken)->type == TOKENTYPE_ASSIGNMENT) {

            // Something in the form of "var foo = expression;" We'll just
            // treat the "foo = expression;" part as a separate thing.

            EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_ASSIGNMENT);

            if(!compileExpression(cs, currentToken)) {
                return false;
            }

            addVariableWithoutStackAllocation(cs, variableName);

        } else {

            // Something in the form of "var foo;"

            addVariable(cs, variableName);
        }
    }

    EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_SEMICOLON);

    return true;
}

void emitReturn(struct CompilerState *cs)
{
    // Find the function we're in.
    struct CompilerStateContext *ctx = cs->context;
    struct VMFunction *func;
    while(ctx && ctx->currentFunctionId == ~(uint32_t)0) {
        ctx = ctx->parent;
    }

    if(!ctx) {
        errorStateAddError(
            &cs->vm->errorState,
            -1,
            "return statement outside of function.");
        return;
    }

    if(ctx->currentFunctionId >= cs->vm->functionCount) {
        errorStateAddError(
            &cs->vm->errorState,
            -1,
            "Bad function id when attempting to emit return.");
        return;
    }

    func = &cs->vm->functionTable[ctx->currentFunctionId];

    {
        // We want to throw away the context except for...
        //   The return value.   (-1)
        //   The return pointer. (-1)
        //   The argument list.  (-func->argumentCount)
        //   This thing we're about pushing right now. (-1)
        uint32_t throwAwayContext =
            cs->context->stackFrameOffset - func->argumentCount - 3;
        printf("stackFrameOffset: %d\n", cs->context->stackFrameOffset);
        printf("argumentCount:    %d\n", func->argumentCount);
        printf("throwAwayContext: %d\n", throwAwayContext);
        emitPushLiteralInt(cs, throwAwayContext);
        addInstructionSimple(cs, OP_RETURN);
    }
}

bool compileReturnStatement(struct CompilerState *cs, struct Token **currentToken)
{
    EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_RETURN);
    if(!compileExpression(cs, currentToken)) {
        return false;
    }
    emitReturn(cs);
    EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_SEMICOLON);
    return true;
}

bool compileFunctionDefinition(struct CompilerState *cs, struct Token **currentToken)
{
    const char *functionName = NULL;
    uint32_t skipOffset;
    struct CompilerStateContext *functionLocalContext;
    struct CompilerStateContext *savedContext;
    struct CompilerStateContext *searchContext;
    struct CompilerStateContextVariable *varTmp;
    uint32_t functionArgumentCount = 0;

    uint32_t functionId = 0;
    struct VMFunction *functionObject = vmCreateFunction(
        cs->vm, &functionId);

    EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_FUNCTION);

    // Read the function name.
    if(*currentToken && (*currentToken)->type == TOKENTYPE_IDENTIFIER) {
        functionName = (*currentToken)->str;
        *currentToken = (*currentToken)->next;
    } else {
        errorStateAddError(
            &cs->vm->errorState,
            *currentToken ? (*currentToken)->lineNumber : -1,
            "Expected identifier for function name.");
        return false;
    }

    // At the parent scope, create a variable with the name of the
    // function and give it an immediate value for the function.
    // Recursive function calls will not be possible if the function
    // cannot refer to itself before it's finished being fully
    // created.
    emitPushLiteralFunctionId(cs, functionId);
    addVariableWithoutStackAllocation(cs, functionName);

    // Add some instructions to skip around this function. This is
    // kind of a weird way to do it, but it means that we can just
    // start programs at instruction 0 and not worry about
    // functions in the middle. If declared at global scope, this
    // will only happen once per function so whatever.
    emitPushLiteralInt(cs, 0);
    skipOffset = cs->instructionWriteIndex - 1;
    addInstructionSimple(cs, OP_JUMP_RELATIVE);

    // This context is different from the ones we'd normally push/pop,
    // because it's parented to the global context. So we're going to
    // set it and parent it directly.
    functionLocalContext = malloc(sizeof(struct CompilerStateContext));
    memset(functionLocalContext, 0, sizeof(*functionLocalContext));
    savedContext = cs->context;
    // Find the global context and set it as our parent.
    searchContext = savedContext;
    while(searchContext->parent) {
        searchContext = searchContext->parent;
    }
    functionLocalContext->parent = searchContext;
    functionLocalContext->currentFunctionId = functionId;
    // Switch the new context in.
    cs->context = functionLocalContext;

    // Skip '('.
    EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_PAREN_OPEN);

    // Read variable names and skip commas until we get to a closing
    // parenthesis.
    while(*currentToken) {

        if((*currentToken)->type == TOKENTYPE_IDENTIFIER) {

            const char *argumentName = (*currentToken)->str;

            varTmp = addVariableWithoutStackAllocation(cs, argumentName);
            varTmp->doNotPopWhenOutOfScope = true;
            cs->context->stackFrameOffset++;

            functionArgumentCount++;

            *currentToken = (*currentToken)->next;

        } else {

            errorStateAddError(
                &cs->vm->errorState,
                *currentToken ? (*currentToken)->lineNumber : -1,
                "Expected identifier for function argument name.");
        }

        if(*currentToken && (*currentToken)->type == TOKENTYPE_PAREN_CLOSE) {
            break;
        }

        EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_COMMA);
    }

    // Store the functionArgumentCount on the function object.
    functionObject->argumentCount = functionArgumentCount;
    printf("Function argument count is: %d\n", functionArgumentCount);

    // Store the function start address on the function object.
    functionObject->firstInstructionIndex = cs->instructionWriteIndex;

    // We'll check this against the stored function argument count as
    // part of the CALL instruction, and throw an error if it doesn't
    // match. This will be pushed before the CALL.
    varTmp = addVariableWithoutStackAllocation(cs, "_argumentCount");
    varTmp->doNotPopWhenOutOfScope = true;
    cs->context->stackFrameOffset++;

    // // The CALL instruction will push this onto the stack
    // // automatically.
    // varTmp = addVariableWithoutStackAllocation(cs, "_returnValue");
    // varTmp->doNotPopWhenOutOfScope = true;
    // cs->context->stackFrameOffset++;

    // The CALL instruction will push this onto the stack
    // automatically.
    varTmp = addVariableWithoutStackAllocation(cs, "_returnPointer");
    varTmp->doNotPopWhenOutOfScope = true;
    cs->context->stackFrameOffset++;

    // Skip ')'.
    EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_PAREN_CLOSE);

    // Parse and emit actual function code.
    compileStatement(cs, currentToken);

    // functionObject pointer may have been invalidated in the
    // recursive code because of a reallocation (from a function added
    // inside the function), so let's update it just in case.
    functionObject = &cs->vm->functionTable[functionId];

    // Add a RETURN instruction just in case the function reaches the
    // end without returning.
    emitPushLiteralInt(cs, 0); // Return value (zero is probably fine
                               // as a default).
    cs->context->stackFrameOffset++;
    emitReturn(cs);

    // Go back and fix up our relative jump that skips this
    // function now that we know how long it is.
    cs->vm->instructions[skipOffset].opData_int =
        cs->instructionWriteIndex - skipOffset - 2;

    // Restore the "real" context back to the parent.
    popContext(cs);
    cs->context = savedContext;

    return true;
}


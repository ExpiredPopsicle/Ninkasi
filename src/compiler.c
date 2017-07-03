#include "common.h"

void addInstruction(struct CompilerState *cs, struct Instruction *inst)
{
    if(cs->instructionWriteIndex >= cs->vm->instructionAddressMask) {

        // TODO: Remove this.
        dbgWriteLine("Expanding VM instruction space.");

        // FIXME: Add a dynamic or settable memory limit.
        if(cs->vm->instructionAddressMask >= 0xfffff) {
            vmCompilerAddError(cs, "Too many instructions.");
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

  #if VM_DEBUG
    cs->vm->instructions[cs->instructionWriteIndex].lineNumber =
        vmCompilerGetLinenumber(cs);
  #endif

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

    dbgWriteLine("Pushed context: %p", cs->context);
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

        uint32_t popCount = 0;

        while(var) {
            struct CompilerStateContextVariable *next =
                var->next;

            // Count up the amount of stuff in this stack frame to get
            // rid of.
            if(!var->doNotPopWhenOutOfScope) {
                popCount++;
            }

            // Free it.
            free(var->name);
            free(var);
            var = next;

            dbgWriteLine("Variable removed.");
        }

        {
            // Add instructions to pop variables off the stack that
            // are no longer in scope. First attempt to add onto
            // existing instructions that do this for the context.

            uint32_t maybePushIntAddr =
                (cs->instructionWriteIndex - 3) & cs->vm->instructionAddressMask;
            uint32_t maybeIntAddr =
                (cs->instructionWriteIndex - 2) & cs->vm->instructionAddressMask;
            uint32_t maybePopNAddr =
                (cs->instructionWriteIndex - 1) & cs->vm->instructionAddressMask;

            if(cs->vm->instructions[maybePushIntAddr].opcode == OP_PUSHLITERAL_INT &&
                cs->vm->instructions[maybePopNAddr].opcode == OP_POPN)
            {
                // Looks like we just came out of a context, so we can
                // add onto that.
                cs->vm->instructions[maybeIntAddr].opData_int += popCount;

            } else {

                // Last thing was not exiting a context, so we need a
                // new set of instructions here. (Assuming there's
                // anything to pop.)
                if(popCount) {
                    emitPushLiteralInt(cs, popCount);
                    addInstructionSimple(cs, OP_POPN);
                }
            }
        }

    }

    // Free the context itself.
    free(oldContext);

    dbgPop();
    dbgWriteLine("Popped context: %p (now %p)", oldContext, cs->context);
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

#define EXPECT_AND_SKIP_STATEMENT(x)                \
    do {                                            \
        if(vmCompilerTokenType(cs) != (x)) {        \
            struct DynString *errStr =              \
                dynStrCreate("Unexpected token: "); \
            dynStrAppend(                           \
                errStr,                             \
                vmCompilerTokenString(cs));         \
            vmCompilerAddError(cs, errStr->data);   \
            dynStrDelete(errStr);                   \
            return false;                           \
        } else {                                    \
            vmCompilerNextToken(cs);                \
        }                                           \
    } while(0)

bool compileStatement(struct CompilerState *cs)
{
    // FIXME: Remove this.
    dbgWriteLine(
        "Entering compileStatement with stackFrameOffset: %u",
        cs->context->stackFrameOffset);

    if(vmCompilerTokenType(cs) == TOKENTYPE_INVALID) {
        vmCompilerAddError(cs, "Ran out of tokens to parse.");
        return false;
    }

    switch(vmCompilerTokenType(cs)) {

        case TOKENTYPE_VAR:
            // "var" = Variable declaration.
            return compileVariableDeclaration(cs);

        case TOKENTYPE_FUNCTION:
            // "function" = Function definition.
            return compileFunctionDefinition(cs);

        case TOKENTYPE_RETURN:
            // "return" = Return statement.
            return compileReturnStatement(cs);

        case TOKENTYPE_CURLYBRACE_OPEN:
            // Curly braces mean we need to parse a block.
            return compileBlock(cs, false);

        case TOKENTYPE_IF:
            // Curly braces mean we need to parse a block.
            return compileIfStatement(cs);

        default:
            // Fall back to just parsing an expression.
            if(!compileExpression(cs)) {
                return false;
            }

            if(vmCompilerTokenType(cs) == TOKENTYPE_SEMICOLON) {
                // TODO: Remove this. (Debugging output.) Replace it with
                // a OP_POP.
                addInstructionSimple(cs, OP_DUMP);
                cs->context->stackFrameOffset--;
            }

            EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_SEMICOLON);

            break;
    }

    return true;
}

struct CompilerStateContextVariable *lookupVariable(
    struct CompilerState *cs,
    const char *name)
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
        vmCompilerAddError(cs, dynStr->data);
        dynStrDelete(dynStr);
        return NULL;
    }

    return var;
}

bool compileBlock(struct CompilerState *cs, bool noBracesOrContext)
{
    if(!noBracesOrContext) {
        EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_CURLYBRACE_OPEN);
        pushContext(cs);
    }

    // FIXME: Remove this.
    dbgWriteLine("Compiling block");
    dbgPush();

    while(
        vmCompilerTokenType(cs) != TOKENTYPE_CURLYBRACE_CLOSE &&
        vmCompilerTokenType(cs) != TOKENTYPE_INVALID)
    {
        if(!compileStatement(cs)) {

            // FIXME: Remove this.
            dbgPop();

            return false;
        }
    }

    // FIXME: Remove this.
    dbgPop();

    if(!noBracesOrContext) {
        popContext(cs);
        EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_CURLYBRACE_CLOSE);
    }

    return true;
}

bool compileVariableDeclaration(struct CompilerState *cs)
{
    EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_VAR);

    if(vmCompilerTokenType(cs) != TOKENTYPE_IDENTIFIER) {
        vmCompilerAddError(cs, "Expected identifier in variable declaration.");
        return false;
    }

    {
        const char *variableName = vmCompilerTokenString(cs);

        EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_IDENTIFIER);

        if(vmCompilerTokenType(cs) == TOKENTYPE_ASSIGNMENT) {

            // Something in the form of "var foo = expression;" We'll just
            // treat the "foo = expression;" part as a separate thing.

            EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_ASSIGNMENT);

            if(!compileExpression(cs)) {
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
        vmCompilerAddError(
            cs, "return statement outside of function.");
        return;
    }

    if(ctx->currentFunctionId >= cs->vm->functionCount) {
        vmCompilerAddError(
            cs, "Bad function id when attempting to emit return.");
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

        // FIXME: Remove these.
        dbgWriteLine("stackFrameOffset: %d", cs->context->stackFrameOffset);
        dbgWriteLine("argumentCount:    %d", func->argumentCount);
        dbgWriteLine("throwAwayContext: %d", throwAwayContext);

        emitPushLiteralInt(cs, throwAwayContext);
        addInstructionSimple(cs, OP_RETURN);
    }
}

bool compileReturnStatement(struct CompilerState *cs)
{
    EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_RETURN);
    if(!compileExpression(cs)) {
        return false;
    }
    emitReturn(cs);
    EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_SEMICOLON);
    return true;
}

bool compileFunctionDefinition(struct CompilerState *cs)
{
    bool ret = true;
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
    if(vmCompilerTokenType(cs) == TOKENTYPE_IDENTIFIER) {
        functionName = vmCompilerTokenString(cs);
        vmCompilerNextToken(cs);
    } else {
        errorStateAddError(
            &cs->vm->errorState,
            vmCompilerGetLinenumber(cs),
            "Expected identifier for function name.");
        return false;
    }

    // At the parent scope, create a variable with the name of the
    // function and give it an immediate value for the function.
    // Recursive function calls will not be possible if the function
    // cannot refer to itself before it's finished being fully
    // created.
    emitPushLiteralFunctionId(cs, functionId);
    cs->context->stackFrameOffset++;
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
    if(vmCompilerTokenType(cs) == TOKENTYPE_PAREN_OPEN) {
        vmCompilerNextToken(cs);
    } else {
        vmCompilerAddError(cs, "Expected '('.");
    }

    // Read variable names and skip commas until we get to a closing
    // parenthesis.
    while(vmCompilerTokenType(cs) != TOKENTYPE_INVALID) {

        // Add each of them as a local variable to the new context.
        if(vmCompilerTokenType(cs) == TOKENTYPE_IDENTIFIER) {

            const char *argumentName = vmCompilerTokenString(cs);

            // FIXME: Some day, figure out why the heck the variables
            // are offset +1 in the stack.
            cs->context->stackFrameOffset++;
            varTmp = addVariableWithoutStackAllocation(cs, argumentName);

            varTmp->doNotPopWhenOutOfScope = true;

            functionArgumentCount++;

            vmCompilerNextToken(cs);

        } else if(vmCompilerTokenType(cs) != TOKENTYPE_PAREN_CLOSE) {

            vmCompilerAddError(
                cs, "Expected identifier for function argument name.");
            ret = false;
            break;
        }

        if(vmCompilerTokenType(cs) == TOKENTYPE_PAREN_CLOSE) {
            break;
        }

        if(vmCompilerTokenType(cs) == TOKENTYPE_COMMA) {
            vmCompilerNextToken(cs);
        } else {
            vmCompilerAddError(cs, "Expected ')' or ','.");
        }
    }

    // Store the functionArgumentCount on the function object.
    functionObject->argumentCount = functionArgumentCount;

    // FIXME: Remove this.
    dbgWriteLine("Function argument count is: %d", functionArgumentCount);

    // Store the function start address on the function object.
    functionObject->firstInstructionIndex = cs->instructionWriteIndex;

    // Add the function id itself as a variable inside the function
    // (because it's on the stack anyway).
    varTmp = addVariableWithoutStackAllocation(cs, "_functionId");
    varTmp->doNotPopWhenOutOfScope = true;
    cs->context->stackFrameOffset++;

    // We'll check this against the stored function argument count as
    // part of the CALL instruction, and throw an error if it doesn't
    // match. This will be pushed before the CALL.
    varTmp = addVariableWithoutStackAllocation(cs, "_argumentCount");
    varTmp->doNotPopWhenOutOfScope = true;
    cs->context->stackFrameOffset++;

    // The CALL instruction will push this onto the stack
    // automatically.
    varTmp = addVariableWithoutStackAllocation(cs, "_returnPointer");
    varTmp->doNotPopWhenOutOfScope = true;

    // Skip ')'.
    if(vmCompilerTokenType(cs) == TOKENTYPE_PAREN_CLOSE) {
        vmCompilerNextToken(cs);
    } else {
        vmCompilerAddError(cs, "Expected ')'.");
    }

    // Parse and emit actual function code.
    ret = ret && compileStatement(cs);

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
    dbgPush(); // FIXME: To counter the push inside popContext.
    popContext(cs);
    cs->context = savedContext;

    return ret;
}

struct Token *vmCompilerNextToken(struct CompilerState *cs)
{
    if(cs->currentToken) {
        cs->currentToken = cs->currentToken->next;
        if(cs->currentToken) {
            cs->currentLineNumber = cs->currentToken->lineNumber;
        }
    }
    return cs->currentToken;
}

enum TokenType vmCompilerTokenType(struct CompilerState *cs)
{
    if(cs->currentToken) {
        return cs->currentToken->type;
    }
    return TOKENTYPE_INVALID;
}

uint32_t vmCompilerGetLinenumber(struct CompilerState *cs)
{
    if(cs->currentToken) {
        return cs->currentToken->lineNumber;
    }
    return cs->currentLineNumber;
}

const char *vmCompilerTokenString(struct CompilerState *cs)
{
    if(cs->currentToken) {
        return cs->currentToken->str;
    }
    return "<invalid token>";
}

void vmCompilerAddError(struct CompilerState *cs, const char *error)
{
    errorStateAddError(
        &cs->vm->errorState,
        vmCompilerGetLinenumber(cs),
        error);
}

void vmCompilerCreateCFunctionVariable(
    struct CompilerState *cs,
    const char *name,
    VMFunctionCallback func)
{
    // TODO: Lookup function first, to make sure we aren't making
    // duplicate functions.

    uint32_t functionId = 0;
    struct VMFunction *vmfunc =
        vmCreateFunction(cs->vm, &functionId);
    emitPushLiteralFunctionId(cs, functionId);
    cs->context->stackFrameOffset++;
    addVariableWithoutStackAllocation(cs, name);

    vmfunc->argumentCount = ~(uint32_t)0;
    vmfunc->isCFunction = true;
    vmfunc->CFunctionCallback = func;
}

struct CompilerState *vmCompilerCreate(
    struct VM *vm)
{
    struct CompilerState *cs;

    cs = malloc(sizeof(struct CompilerState));
    memset(cs, 0, sizeof(*cs));

    cs->instructionWriteIndex = 0;
    cs->vm = vm;
    cs->context = NULL;

    // cs->currentToken = tokenList.first;
    // cs->currentLineNumber =
    //     cs->currentToken ? cs->currentToken->lineNumber : 0;

    pushContext(cs);

    return cs;
}

void vmCompilerFinalize(
    struct CompilerState *cs)
{
    // Pop everything up to the global context.
    while(cs->context && cs->context->parent) {
        popContext(cs);
    }

    // Create the global variable table.
    {
        // Run through the variable list once and count up how much
        // memory we need.
        uint32_t count = 0;
        uint32_t nameStorageNeeded = 0;
        struct CompilerStateContextVariable *var =
            cs->context->variables;
        while(var) {
            count++;
            nameStorageNeeded += strlen(var->name) + 1;
            var = var->next;
        }

        // Allocate that.
        cs->vm->globalVariableNameStorage = malloc(nameStorageNeeded);
        cs->vm->globalVariables =
            malloc(sizeof(struct GlobalVariableRecord) * count);
        cs->vm->globalVariableCount = count;

        // Now run through it all again and actually assign data.
        var = cs->context->variables;
        {
            char *nameWritePtr = cs->vm->globalVariableNameStorage;
            count = 0;

            while(var) {

                cs->vm->globalVariables[count].stackPosition = var->stackPos;
                cs->vm->globalVariables[count].name = nameWritePtr;
                strcpy(nameWritePtr, var->name);
                nameWritePtr += strlen(var->name) + 1;

                count++;
                var = var->next;
            }
        }
    }

    // Pop the global context.
    while(cs->context) {
        popContext(cs);
    }

    // Add a single end instruction.
    addInstructionSimple(cs, OP_END);

    // TODO: Setup the global variable table in the VM, so we can
    // access stuff by name later.

    free(cs);
}

bool vmCompilerCompileScript(
    struct CompilerState *cs,
    const char *script)
{
    struct TokenList tokenList;
    bool success;

    // Tokenize.

    memset(&tokenList, 0, sizeof(tokenList));
    tokenList.first = NULL;
    tokenList.last = NULL;

    success = tokenize(cs->vm, script, &tokenList);
    if(!success) {
        destroyTokenList(&tokenList);
        return false;
    }

    // Compile.

    cs->currentToken = tokenList.first;
    cs->currentLineNumber =
        cs->currentToken ? cs->currentToken->lineNumber : 0;

    success = compileBlock(cs, true);

    cs->currentToken = NULL;
    cs->currentLineNumber = 0;
    destroyTokenList(&tokenList);
    return success;
}

bool vmCompilerCompileScriptFile(
    struct CompilerState *cs,
    const char *scriptFilename)
{
    FILE *in = fopen(scriptFilename, "rb");
    uint32_t len;
    char *buf;
    bool success;

    if(!in) {
        struct DynString *errStr =
            dynStrCreate("Cannot open script file: ");
        dynStrAppend(errStr, scriptFilename);
        vmCompilerAddError(cs, errStr->data);
        dynStrDelete(errStr);
        return false;
    }

    fseek(in, 0, SEEK_END);
    len = ftell(in);
    fseek(in, 0, SEEK_SET);

    buf = malloc(len + 1);
    fread(buf, len, 1, in);
    buf[len] = 0;

    fclose(in);

    success = vmCompilerCompileScript(cs, buf);

    free(buf);

    return success;
}

bool compileIfStatement(struct CompilerState *cs)
{
    uint32_t skipAddressWritePtr = 0;

    // Skip "if("
    EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_IF);
    EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_PAREN_OPEN);

    // Generate the expression code.
    compileExpression(cs);

    // Add the OP_JUMP_IF_ZERO, and save the literal address so we can
    // fill it in after we know how much we're going to have to skip.
    emitPushLiteralInt(cs, 0);
    skipAddressWritePtr = cs->instructionWriteIndex - 1;
    addInstructionSimple(cs, OP_JUMP_IF_ZERO);

    // Skip ")"
    EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_PAREN_CLOSE);

    // Generate code to execute if test passes.
    compileStatement(cs);

    // Fixup skip offset.
    cs->vm->instructions[skipAddressWritePtr & cs->vm->instructionAddressMask].opData_int =
        (cs->instructionWriteIndex - skipAddressWritePtr) - 2;

    if(vmCompilerTokenType(cs) == TOKENTYPE_ELSE) {

        EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_ELSE);

        // Increase our skip amount by enough to skip past the
        // OP_PUSHLITERAL_INT and OP_JUMP_RELATIVE we're going to
        // insert to skip past the "else" clause.
        cs->vm->instructions[
            skipAddressWritePtr &
            cs->vm->instructionAddressMask].opData_int
            += 3;

        // Emit instructions to skip past the contents of the "else"
        // block. Keep the index of the relative offset here so we can
        // go back and modify it after the inner code is complete.
        emitPushLiteralInt(cs, 0);
        skipAddressWritePtr = cs->instructionWriteIndex - 1;
        addInstructionSimple(cs, OP_JUMP_RELATIVE);

        // Generate code to execute if test fails.
        compileStatement(cs);

        // Fixup "else" skip offset.
        cs->vm->instructions[skipAddressWritePtr & cs->vm->instructionAddressMask].opData_int =
            (cs->instructionWriteIndex - skipAddressWritePtr) - 2;

    }

    return true;
}



#include "common.h"

void nkiCompilerAddInstruction(
    struct NKCompilerState *cs, struct NKInstruction *inst,
    bool adjustStackFrame)
{
    if(cs->instructionWriteIndex >= cs->vm->instructionAddressMask) {

        // TODO: Remove this.
        dbgWriteLine("Expanding VM instruction space.");

        // FIXME: Add a dynamic or settable memory limit.
        if(cs->vm->instructionAddressMask >= 0xfffff) {
            nkiCompilerAddError(cs, "Too many instructions.");
        }

        {
            uint32_t oldSize = cs->vm->instructionAddressMask + 1;
            uint32_t newSize = oldSize << 1;

            cs->vm->instructionAddressMask <<= 1;
            cs->vm->instructionAddressMask |= 1;
            cs->vm->instructions = nkiRealloc(
                cs->vm,
                cs->vm->instructions,
                sizeof(struct NKInstruction) *
                newSize);

            // Clear the new area to NOPs.
            memset(
                cs->vm->instructions + oldSize, 0,
                (newSize - oldSize) * sizeof(struct NKInstruction));
        }
    }

    cs->vm->instructions[cs->instructionWriteIndex] = *inst;

    // Adjust stack frame offset, if necessary.
    if(adjustStackFrame && cs->context) {
        cs->context->stackFrameOffset +=
            nkiCompilerStackOffsetTable[inst->opcode & (NK_OPCODE_PADDEDCOUNT - 1)];
    }

  #if VM_DEBUG
    cs->vm->instructions[cs->instructionWriteIndex].lineNumber =
        nkiCompilerCurrentTokenLinenumber(cs);
  #endif

    cs->instructionWriteIndex++;
}

// FIXME: "nkiCompilerAdd..."
void nkiCompilerAddInstructionSimple(
    struct NKCompilerState *cs, enum NKOpcode opcode,
    bool adjustStackFrame)
{
    struct NKInstruction inst;
    memset(&inst, 0, sizeof(inst));
    inst.opcode = opcode;
    nkiCompilerAddInstruction(cs, &inst, adjustStackFrame);
}

void nkiCompilerPushContext(struct NKCompilerState *cs)
{
    struct NKCompilerStateContext *newContext =
        nkiMalloc(cs->vm, sizeof(struct NKCompilerStateContext));
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

void nkiCompilerPopContext(struct NKCompilerState *cs)
{
    struct NKCompilerStateContext *oldContext =
        cs->context;
    cs->context = cs->context->parent;

    // Free variable data.
    {
        struct NKCompilerStateContextVariable *var =
            oldContext->variables;

        uint32_t popCount = 0;

        while(var) {
            struct NKCompilerStateContextVariable *next =
                var->next;

            // Count up the amount of stuff in this stack frame to get
            // rid of.
            if(!var->doNotPopWhenOutOfScope) {
                popCount++;
            }

            // Free it.
            nkiFree(cs->vm, var->name);
            nkiFree(cs->vm, var);
            var = next;

            dbgWriteLine("Variable removed.");
        }

        {

            // FIXME !!!!!!!! This idea of consolidating the POPN
            // instructions failed because of one very important
            // thing! Sometimes we need two adjacent blocks of
            // PUSHLITERAL/POPNs, because a JUMP might target the
            // second one. This manifested in an "if" statement's
            // "else" block at the end of a "for" loop incorrectly
            // getting consolidated with the "for" loop context.
            // Yikes!

            // Add instructions to pop variables off the stack that
            // are no longer in scope. First attempt to add onto
            // existing instructions that do this for the context.

            // uint32_t maybePushIntAddr =
            //     (cs->instructionWriteIndex - 3) & cs->vm->instructionAddressMask;
            // uint32_t maybeIntAddr =
            //     (cs->instructionWriteIndex - 2) & cs->vm->instructionAddressMask;
            // uint32_t maybePopNAddr =
            //     (cs->instructionWriteIndex - 1) & cs->vm->instructionAddressMask;

            // if(cs->vm->instructions[maybePushIntAddr].opcode == NK_OP_PUSHLITERAL_INT &&
            //     cs->vm->instructions[maybePopNAddr].opcode == NK_OP_POPN)
            // {
            //     // Looks like we just came out of a context, so we can
            //     // add onto that.
            //     cs->vm->instructions[maybeIntAddr].opData_int += popCount;

            // } else {

                // Last thing was not exiting a context, so we need a
                // new set of instructions here. (Assuming there's
                // anything to pop.)
                if(popCount) {
                    nkiCompilerEmitPushLiteralInt(cs, popCount, false);
                    nkiCompilerAddInstructionSimple(cs, NK_OP_POPN, false);
                }

            // }
        }

    }

    // Free loop context jump fixups for break statements.
    nkiFree(cs->vm, oldContext->loopContextFixups);

    // Free the context itself.
    nkiFree(cs->vm, oldContext);

    dbgPop();
    dbgWriteLine("Popped context: %p (now %p)", oldContext, cs->context);
}

void nkiCompilerEmitPushLiteralInt(struct NKCompilerState *cs, int32_t value, bool adjustStackFrame)
{
    struct NKInstruction inst;

    // Add instruction.
    memset(&inst, 0, sizeof(inst));
    inst.opcode = NK_OP_PUSHLITERAL_INT;
    nkiCompilerAddInstruction(cs, &inst, adjustStackFrame);

    // Add parameter.
    memset(&inst, 0, sizeof(inst));
    inst.opData_int = value;
    nkiCompilerAddInstruction(cs, &inst, false);
}

void nkiCompilerEmitPushLiteralFunctionId(struct NKCompilerState *cs, uint32_t functionId, bool adjustStackFrame)
{
    struct NKInstruction inst;

    // Add instruction.
    memset(&inst, 0, sizeof(inst));
    inst.opcode = NK_OP_PUSHLITERAL_FUNCTIONID;
    nkiCompilerAddInstruction(cs, &inst, adjustStackFrame);

    // Add parameter.
    memset(&inst, 0, sizeof(inst));
    inst.opData_functionId = functionId;
    nkiCompilerAddInstruction(cs, &inst, false);
}

void nkiCompilerEmitPushLiteralFloat(struct NKCompilerState *cs, float value, bool adjustStackFrame)
{
    struct NKInstruction inst;

    // Add instruction.
    memset(&inst, 0, sizeof(inst));
    inst.opcode = NK_OP_PUSHLITERAL_FLOAT;
    nkiCompilerAddInstruction(cs, &inst, adjustStackFrame);

    // Add parameter.
    memset(&inst, 0, sizeof(inst));
    inst.opData_float = value;
    nkiCompilerAddInstruction(cs, &inst, false);
}

void nkiCompilerEmitPushLiteralString(struct NKCompilerState *cs, const char *str, bool adjustStackFrame)
{
    struct NKInstruction inst;

    // Add instruction.
    memset(&inst, 0, sizeof(inst));
    inst.opcode = NK_OP_PUSHLITERAL_STRING;
    nkiCompilerAddInstruction(cs, &inst, adjustStackFrame);

    // Add string table entry data as op parameter.
    memset(&inst, 0, sizeof(inst));
    inst.opData_string =
        nkiVmStringTableFindOrAddString(
            cs->vm,
            str);
    nkiCompilerAddInstruction(cs, &inst, false);

    // Mark this string as not garbage-collected.
    {
        struct NKVMString *entry = nkiVmStringTableGetEntryById(
            &cs->vm->stringTable, inst.opData_string);
        if(entry) {
            entry->dontGC = true;
        }
    }
}

void nkiCompilerEmitPushNil(struct NKCompilerState *cs, bool adjustStackFrame)
{
    nkiCompilerAddInstructionSimple(cs, NK_OP_PUSHNIL, adjustStackFrame);
}

struct NKCompilerStateContextVariable *nkiCompilerAddVariable(
    struct NKCompilerState *cs, const char *name, bool allocateStackSpace)
{
    struct NKCompilerStateContextVariable *var =
        nkiMalloc(cs->vm, sizeof(struct NKCompilerStateContextVariable));
    memset(var, 0, sizeof(*var));

    // Add an instruction to make some stack space for this variable,
    // if needed.
    if(allocateStackSpace) {
        nkiCompilerEmitPushLiteralInt(cs, 0, true);
    }

    var->next = cs->context->variables;
    var->isGlobal = !cs->context->parent;
    var->name = nkiStrdup(cs->vm, name);
    var->stackPos = cs->context->stackFrameOffset - 1;

    cs->context->variables = var;

    dbgWriteLine("Variable added.");

    return var;
}

bool nkiCompilerExpectAndSkipToken(
    struct NKCompilerState *cs, enum NKTokenType t)
{
    if(nkiCompilerCurrentTokenType(cs) != t) {
        struct NKDynString *errStr =
            nkiDynStrCreate(cs->vm, "Unexpected token: ");
        nkiDynStrAppend(
            errStr,
            nkiCompilerCurrentTokenString(cs));
        nkiCompilerAddError(cs, errStr->data);
        nkiDynStrDelete(errStr);
        return false;
    } else {
        nkiCompilerNextToken(cs);
    }
    return true;
}

// TODO: Remove instances of this and replace them with
// nkiCompilerExpectAndSkipToken().
#define EXPECT_AND_SKIP_STATEMENT(x)                \
    do {                                            \
        if(!nkiCompilerExpectAndSkipToken(cs, x)) { \
            nkiCompilerPopRecursion(cs);            \
            return false;                           \
        }                                           \
    } while(0)


bool nkiCompilerCompileStatement(struct NKCompilerState *cs)
{
    struct NKCompilerStateContext *startContext = cs->context;

    if(!nkiCompilerPushRecursion(cs)) {
        return false;
    }

    // FIXME: Remove this.
    dbgWriteLine(
        "Entering nkiCompilerCompileStatement with stackFrameOffset: %u",
        cs->context->stackFrameOffset);

    if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_INVALID) {
        nkiCompilerAddError(cs, "Ran out of tokens to parse.");
        return false;
    }

    switch(nkiCompilerCurrentTokenType(cs)) {

        case NK_TOKENTYPE_VAR:
            // "var" = Variable declaration.
            return nkiCompilerCompileVariableDeclaration(cs);

        case NK_TOKENTYPE_FUNCTION:
            // "function" = Function definition.
            if(!nkiCompilerCompileFunctionDefinition(cs)) {
                return false;
            }
            break;

        case NK_TOKENTYPE_RETURN:
            // "return" = Return statement.
            return nkiCompilerCompileReturnStatement(cs);

        case NK_TOKENTYPE_CURLYBRACE_OPEN:
            // Curly braces mean we need to parse a block.
            if(!nkiCompilerCompileBlock(cs, false)) {
                return false;
            }
            break;

        case NK_TOKENTYPE_IF:
            // "if" statements.
            if(!nkiCompilerCompileIfStatement(cs)) {
                return false;
            }
            break;

        case NK_TOKENTYPE_WHILE:
            // "while" statements.
            if(!nkiCompilerCompileWhileStatement(cs)) {
                return false;
            }
            break;

        case NK_TOKENTYPE_FOR:
            // "for" statements.
            if(!nkiCompilerCompileForStatement(cs)) {
                return false;
            }
            break;

        case NK_TOKENTYPE_BREAK:
            // "break" statements.
            if(!nkiCompilerCompileBreakStatement(cs)) {
                return false;
            }
            break;

        default:
            // Fall back to just parsing an expression.
            if(!nkiCompilerCompileExpression(cs)) {
                return false;
            }

            if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_SEMICOLON) {
                // TODO: Remove this. (Debugging output.) Replace it with
                // a NK_OP_POP.
                nkiCompilerAddInstructionSimple(cs, NK_OP_DUMP, true);
            }

            EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_SEMICOLON);

            break;
    }

    assert(cs->context == startContext);
    nkiCompilerPopRecursion(cs);
    return true;
}

struct NKCompilerStateContextVariable *nkiCompilerLookupVariable(
    struct NKCompilerState *cs,
    const char *name)
{
    struct NKCompilerStateContext *ctx = cs->context;
    struct NKCompilerStateContextVariable *var = NULL;

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
        struct NKDynString *dynStr =
            nkiDynStrCreate(cs->vm, "Cannot find variable: ");
        nkiDynStrAppend(dynStr, name);
        nkiCompilerAddError(cs, dynStr->data);
        nkiDynStrDelete(dynStr);
        return NULL;
    }

    return var;
}

bool nkiCompilerCompileBlock(struct NKCompilerState *cs, bool noBracesOrContext)
{
    struct NKCompilerStateContext *startContext = cs->context;

    if(!nkiCompilerPushRecursion(cs)) {
        return false;
    }

    if(!noBracesOrContext) {
        EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_CURLYBRACE_OPEN);
        nkiCompilerPushContext(cs);
    }

    while(
        nkiCompilerCurrentTokenType(cs) != NK_TOKENTYPE_CURLYBRACE_CLOSE &&
        nkiCompilerCurrentTokenType(cs) != NK_TOKENTYPE_INVALID)
    {
        if(!nkiCompilerCompileStatement(cs)) {

            if(!noBracesOrContext) {
                nkiCompilerPopContext(cs);
            }

            // assert(startContext == cs->context);
            nkiCompilerPopRecursion(cs);
            return false;
        }
    }

    if(!noBracesOrContext) {
        nkiCompilerPopContext(cs);
        EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_CURLYBRACE_CLOSE);
    }

    assert(startContext == cs->context);
    nkiCompilerPopRecursion(cs);
    return true;
}

bool nkiCompilerCompileVariableDeclaration(struct NKCompilerState *cs)
{
    if(!nkiCompilerPushRecursion(cs)) {
        return false;
    }

    EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_VAR);

    if(nkiCompilerCurrentTokenType(cs) != NK_TOKENTYPE_IDENTIFIER) {
        nkiCompilerAddError(cs, "Expected identifier in variable declaration.");
        nkiCompilerPopRecursion(cs);
        return false;
    }

    {
        const char *variableName = nkiCompilerCurrentTokenString(cs);

        EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_IDENTIFIER);

        if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_ASSIGNMENT) {

            // Something in the form of "var foo = expression;" We'll just
            // treat the "foo = expression;" part as a separate thing.

            EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_ASSIGNMENT);

            if(!nkiCompilerCompileExpression(cs)) {
                nkiCompilerPopRecursion(cs);
                return false;
            }

            nkiCompilerAddVariable(cs, variableName, false);

        } else {

            // Something in the form of "var foo;"

            nkiCompilerAddVariable(cs, variableName, true);
        }
    }

    EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_SEMICOLON);

    nkiCompilerPopRecursion(cs);
    return true;
}

void emitReturn(struct NKCompilerState *cs)
{
    // Find the function we're in.
    struct NKCompilerStateContext *ctx = cs->context;
    struct NKVMFunction *func;
    while(ctx && ctx->currentFunctionId == ~(uint32_t)0) {
        ctx = ctx->parent;
    }

    if(!ctx) {
        nkiCompilerAddError(
            cs, "return statement outside of function.");
        return;
    }

    if(ctx->currentFunctionId >= cs->vm->functionCount) {
        nkiCompilerAddError(
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

        nkiCompilerEmitPushLiteralInt(cs, throwAwayContext, false);
        nkiCompilerAddInstructionSimple(cs, NK_OP_RETURN, false);
    }
}

bool nkiCompilerCompileReturnStatement(struct NKCompilerState *cs)
{
    if(!nkiCompilerPushRecursion(cs)) {
        return false;
    }

    EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_RETURN);

    if(!nkiCompilerCompileExpression(cs)) {
        nkiCompilerPopRecursion(cs);
        return false;
    }

    emitReturn(cs);

    EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_SEMICOLON);

    nkiCompilerPopRecursion(cs);

    return true;
}

bool nkiCompilerCompileFunctionDefinition(struct NKCompilerState *cs)
{
    bool ret = true;
    const char *functionName = NULL;
    uint32_t skipOffset;
    struct NKCompilerStateContext *functionLocalContext;
    struct NKCompilerStateContext *savedContext;
    struct NKCompilerStateContext *searchContext;
    struct NKCompilerStateContextVariable *varTmp;
    uint32_t functionArgumentCount = 0;

    uint32_t functionId = 0;
    struct NKVMFunction *functionObject;

    if(!nkiCompilerPushRecursion(cs)) {
        return false;
    }

    functionObject = vmCreateFunction(
        cs->vm, &functionId);

    EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_FUNCTION);

    // Read the function name.
    if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_IDENTIFIER) {
        functionName = nkiCompilerCurrentTokenString(cs);
        nkiCompilerNextToken(cs);
    } else {
        nkiAddError(
            cs->vm,
            nkiCompilerCurrentTokenLinenumber(cs),
            "Expected identifier for function name.");
        nkiCompilerPopRecursion(cs);
        return false;
    }

    // At the parent scope, create a variable with the name of the
    // function and give it an immediate value for the function.
    // Recursive function calls will not be possible if the function
    // cannot refer to itself before it's finished being fully
    // created.
    nkiCompilerEmitPushLiteralFunctionId(cs, functionId, true);
    nkiCompilerAddVariable(cs, functionName, false);

    // Add some instructions to skip around this function. This is
    // kind of a weird way to do it, but it means that we can just
    // start programs at instruction 0 and not worry about
    // functions in the middle. If declared at global scope, this
    // will only happen once per function so whatever.
    nkiCompilerEmitPushLiteralInt(cs, 0, false);
    skipOffset = cs->instructionWriteIndex - 1;
    nkiCompilerAddInstructionSimple(cs, NK_OP_JUMP_RELATIVE, false);

    // This context is different from the ones we'd normally push/pop,
    // because it's parented to the global context. So we're going to
    // set it and parent it directly.
    functionLocalContext = nkiMalloc(
        cs->vm, sizeof(struct NKCompilerStateContext));
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
    if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_PAREN_OPEN) {
        nkiCompilerNextToken(cs);
    } else {
        nkiCompilerAddError(cs, "Expected '('.");
    }

    // Read variable names and skip commas until we get to a closing
    // parenthesis.
    while(nkiCompilerCurrentTokenType(cs) != NK_TOKENTYPE_INVALID) {

        // Add each of them as a local variable to the new context.
        if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_IDENTIFIER) {

            const char *argumentName = nkiCompilerCurrentTokenString(cs);

            // FIXME: Some day, figure out why the heck the variables
            // are offset +1 in the stack.
            cs->context->stackFrameOffset++;
            varTmp = nkiCompilerAddVariable(cs, argumentName, false);

            varTmp->doNotPopWhenOutOfScope = true;

            functionArgumentCount++;

            nkiCompilerNextToken(cs);

        } else if(nkiCompilerCurrentTokenType(cs) != NK_TOKENTYPE_PAREN_CLOSE) {

            nkiCompilerAddError(
                cs, "Expected identifier for function argument name.");
            ret = false;
            break;
        }

        if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_PAREN_CLOSE) {
            break;
        }

        if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_COMMA) {
            nkiCompilerNextToken(cs);
        } else {
            nkiCompilerAddError(cs, "Expected ')' or ','.");
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
    varTmp = nkiCompilerAddVariable(cs, "_functionId", false);
    varTmp->doNotPopWhenOutOfScope = true;
    cs->context->stackFrameOffset++;

    // We'll check this against the stored function argument count as
    // part of the CALL instruction, and throw an error if it doesn't
    // match. This will be pushed before the CALL.
    varTmp = nkiCompilerAddVariable(cs, "_argumentCount", false);
    varTmp->doNotPopWhenOutOfScope = true;
    cs->context->stackFrameOffset++;

    // The CALL instruction will push this onto the stack
    // automatically.
    varTmp = nkiCompilerAddVariable(cs, "_returnPointer", false);
    varTmp->doNotPopWhenOutOfScope = true;

    // Skip ')'.
    if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_PAREN_CLOSE) {
        nkiCompilerNextToken(cs);
    } else {
        nkiCompilerAddError(cs, "Expected ')'.");
    }

    // Parse and emit actual function code.
    if(!nkiCompilerCompileStatement(cs)) {
        ret = false;
    }

    // functionObject pointer may have been invalidated in the
    // recursive code because of a reallocation (from a function added
    // inside the function), so let's update it just in case.
    functionObject = &cs->vm->functionTable[functionId];

    // Add a RETURN instruction just in case the function reaches the
    // end without returning. (Zero is probably fine as a default
    // return value).
    nkiCompilerEmitPushLiteralInt(cs, 0, false);
    if(cs->context) {
        cs->context->stackFrameOffset++;
    }
    emitReturn(cs);

    // Go back and fix up our relative jump that skips this
    // function now that we know how long it is.
    if(cs->vm->instructions) {
        cs->vm->instructions[skipOffset].opData_int =
            cs->instructionWriteIndex - skipOffset - 2;
    }

    // Restore the "real" context back to the parent.
    dbgPush(); // FIXME: To counter the push inside nkiCompilerPopContext.

    // assert(cs->context == functionLocalContext);

    nkiCompilerPopContext(cs);
    cs->context = savedContext;

    nkiCompilerPopRecursion(cs);
    return ret;
}

struct NKToken *nkiCompilerNextToken(struct NKCompilerState *cs)
{
    if(cs->currentToken) {
        cs->currentToken = cs->currentToken->next;
        if(cs->currentToken) {
            cs->currentLineNumber = cs->currentToken->lineNumber;
        }
    }
    return cs->currentToken;
}

enum NKTokenType nkiCompilerCurrentTokenType(struct NKCompilerState *cs)
{
    if(cs->currentToken) {
        return cs->currentToken->type;
    }
    return NK_TOKENTYPE_INVALID;
}

uint32_t nkiCompilerCurrentTokenLinenumber(struct NKCompilerState *cs)
{
    if(cs->currentToken) {
        return cs->currentToken->lineNumber;
    }
    return cs->currentLineNumber;
}

const char *nkiCompilerCurrentTokenString(struct NKCompilerState *cs)
{
    if(cs->currentToken) {
        return cs->currentToken->str;
    }
    return "<invalid token>";
}

void nkiCompilerAddError(struct NKCompilerState *cs, const char *error)
{
    nkiAddError(
        cs->vm,
        nkiCompilerCurrentTokenLinenumber(cs),
        error);
}

void vmCompilerCreateCFunctionVariable(
    struct NKCompilerState *cs,
    const char *name,
    VMFunctionCallback func,
    void *userData)
{
    uint32_t functionId = 0;
    struct NKVM *vm = cs->vm;

    // Lookup function first, to make sure we aren't making duplicate
    // functions.
    for(functionId = 0; functionId < vm->functionCount; functionId++) {
        if(vm->functionTable[functionId].CFunctionCallback == func &&
           vm->functionTable[functionId].CFunctionCallbackUserdata == userData)
        {
            break;
        }
    }

    // Add a new one if we haven't found an existing one.
    if(functionId == cs->vm->functionCount) {
        struct NKVMFunction *vmfunc =
            vmCreateFunction(cs->vm, &functionId);
        vmfunc->argumentCount = ~(uint32_t)0;
        vmfunc->isCFunction = true;
        vmfunc->CFunctionCallback = func;
        vmfunc->CFunctionCallbackUserdata = userData;
    }

    // Add the variable.
    nkiCompilerEmitPushLiteralFunctionId(cs, functionId, true);
    nkiCompilerAddVariable(cs, name, false);
}

struct NKCompilerState *vmCompilerCreate(
    struct NKVM *vm)
{
    NK_FAILURE_RECOVERY_DECL();

    struct NKCompilerState *cs;

    NK_SET_FAILURE_RECOVERY(NULL);

    cs = nkiMalloc(vm, sizeof(struct NKCompilerState));
    memset(cs, 0, sizeof(*cs));

    cs->instructionWriteIndex = 0;
    cs->vm = vm;
    cs->context = NULL;

    cs->currentToken = NULL;
    cs->currentLineNumber = 0;

    nkiCompilerPushContext(cs);

    NK_CLEAR_FAILURE_RECOVERY();

    return cs;
}

void vmCompilerFinalize(
    struct NKCompilerState *cs)
{
    // We MUST end up on the global context at the end.
    if(!cs->context || cs->context->parent) {
        nkiCompilerAddError(cs, "Context mismatch at compilation end!");
    }

    // Pop everything up to the global context.
    while(cs->context && cs->context->parent) {
        nkiCompilerPopContext(cs);
    }

    // Create the global variable table.
    {
        // Run through the variable list once and count up how much
        // memory we need.
        uint32_t count = 0;
        uint32_t nameStorageNeeded = 0;
        struct NKCompilerStateContextVariable *var =
            cs->context->variables;
        while(var) {
            count++;
            nameStorageNeeded += strlen(var->name) + 1;
            var = var->next;
        }

        // Allocate that.
        cs->vm->globalVariableNameStorage = nkiMalloc(
            cs->vm, nameStorageNeeded);
        cs->vm->globalVariables =
            nkiMalloc(cs->vm, sizeof(struct GlobalVariableRecord) * count);
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
        nkiCompilerPopContext(cs);
    }

    // Add a single end instruction.
    nkiCompilerAddInstructionSimple(cs, NK_OP_END, true);

    nkiFree(cs->vm, cs);
}

bool vmCompilerCompileScript(
    struct NKCompilerState *cs,
    const char *script)
{
    struct NKTokenList tokenList;
    bool success;

    if(!nkiCompilerPushRecursion(cs)) {
        return false;
    }

    // Tokenize.

    memset(&tokenList, 0, sizeof(tokenList));
    tokenList.first = NULL;
    tokenList.last = NULL;

    success = tokenize(cs->vm, script, &tokenList);
    if(!success) {
        destroyTokenList(cs->vm, &tokenList);
        nkiCompilerPopRecursion(cs);
        return false;
    }

    // Compile.

    cs->currentToken = tokenList.first;
    cs->currentLineNumber =
        cs->currentToken ? cs->currentToken->lineNumber : 0;

    success = nkiCompilerCompileBlock(cs, true);

    cs->currentToken = NULL;
    cs->currentLineNumber = 0;
    destroyTokenList(cs->vm, &tokenList);

    nkiCompilerPopRecursion(cs);
    return success;
}

// bool vmCompilerCompileScriptFile(
//     struct NKCompilerState *cs,
//     const char *scriptFilename)
// {
//     NK_FAILURE_RECOVERY_DECL();
//     struct NKVM *vm = cs->vm;
//     FILE *in = fopen(scriptFilename, "rb");
//     uint32_t len;
//     char *buf;
//     bool success;

//     NK_SET_FAILURE_RECOVERY(false);

//     if(!in) {
//         struct NKDynString *errStr =
//             nkiDynStrCreate(cs->vm, "Cannot open script file: ");
//         nkiDynStrAppend(errStr, scriptFilename);
//         nkiCompilerAddError(cs, errStr->data);
//         nkiDynStrDelete(errStr);
//         return false;
//     }

//     fseek(in, 0, SEEK_END);
//     len = ftell(in);
//     fseek(in, 0, SEEK_SET);

//     buf = malloc(len + 1);
//     if(!buf) {
//         fclose(in);
//         nkiErrorStateSetAllocationFailFlag(vm);
//         NK_CLEAR_FAILURE_RECOVERY();
//         return false;
//     }
//     fread(buf, len, 1, in);
//     buf[len] = 0;

//     fclose(in);

//     success = vmCompilerCompileScript(cs, buf);

//     free(buf);

//     NK_CLEAR_FAILURE_RECOVERY();

//     return success;
// }

uint32_t emitJump(struct NKCompilerState *cs, uint32_t target)
{
    uint32_t instructionWriteIndex = cs->instructionWriteIndex;
    nkiCompilerEmitPushLiteralInt(cs, (target - instructionWriteIndex) - 3, true);
    nkiCompilerAddInstructionSimple(cs, NK_OP_JUMP_RELATIVE, true);
    return instructionWriteIndex;
}

// Note: Pops +1 item off the stack.
uint32_t emitJumpIfZero(struct NKCompilerState *cs, uint32_t target)
{
    uint32_t instructionWriteIndex = cs->instructionWriteIndex;
    nkiCompilerEmitPushLiteralInt(cs, (target - instructionWriteIndex) - 3, true);
    nkiCompilerAddInstructionSimple(cs, NK_OP_JUMP_IF_ZERO, true);
    return instructionWriteIndex;
}

struct NKInstruction *vmCompilerGetInstruction(struct NKCompilerState *cs, uint32_t address)
{
    return &cs->vm->instructions[cs->vm->instructionAddressMask & address];
}

void modifyJump(
    struct NKCompilerState *cs,
    uint32_t pushLiteralBeforeJumpAddress,
    uint32_t target)
{
    struct NKInstruction *pushLitInst =
        vmCompilerGetInstruction(cs, pushLiteralBeforeJumpAddress);
    struct NKInstruction *jumpInst =
        vmCompilerGetInstruction(cs, pushLiteralBeforeJumpAddress + 2);

    assert(pushLitInst->opcode == NK_OP_PUSHLITERAL_INT);
    assert(
        jumpInst->opcode == NK_OP_JUMP_RELATIVE ||
        jumpInst->opcode == NK_OP_JUMP_IF_ZERO);

    vmCompilerGetInstruction(cs, pushLiteralBeforeJumpAddress + 1)->opData_int =
        (target - pushLiteralBeforeJumpAddress) - 3;
}

bool nkiCompilerCompileIfStatement(struct NKCompilerState *cs)
{
    uint32_t skipAddressWritePtr = 0;

    if(!nkiCompilerPushRecursion(cs)) {
        return false;
    }

    // Skip "if("
    EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_IF);
    EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_PAREN_OPEN);

    // Generate the expression code.
    if(!nkiCompilerCompileExpression(cs)) {
        nkiCompilerPopRecursion(cs);
        return false;
    }

    // Add the NK_OP_JUMP_IF_ZERO, and save the literal address so we can
    // fill it in after we know how much we're going to have to skip.
    skipAddressWritePtr = emitJumpIfZero(cs, 0);

    // Skip ")"
    EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_PAREN_CLOSE);

    // Generate code to execute if test passes.
    nkiCompilerPushContext(cs);
    if(!nkiCompilerCompileStatement(cs)) {
        nkiCompilerPopContext(cs);
        nkiCompilerPopRecursion(cs);
        return false;
    }
    nkiCompilerPopContext(cs);

    // Fixup skip offset.
    modifyJump(cs, skipAddressWritePtr, cs->instructionWriteIndex);

    if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_ELSE) {

        uint32_t skipAddressWritePtrElse = 0;

        EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_ELSE);

        // Emit instructions to skip past the contents of the "else"
        // block. Keep the index of the relative offset here so we can
        // go back and modify it after the inner code is complete.
        skipAddressWritePtrElse = emitJump(cs, 0);

        // Increase our skip amount to account for the
        // NK_OP_PUSHLITERAL_INT and NK_OP_JUMP_RELATIVE we added to skip
        // past the "else" clause.
        modifyJump(cs, skipAddressWritePtr, cs->instructionWriteIndex);

        // Generate code to execute if test fails.
        nkiCompilerPushContext(cs);
        if(!nkiCompilerCompileStatement(cs)) {
            nkiCompilerPopContext(cs);
            nkiCompilerPopRecursion(cs);
            return false;
        }
        nkiCompilerPopContext(cs);

        // Fixup "else" skip offset.
        modifyJump(cs, skipAddressWritePtrElse, cs->instructionWriteIndex);
    }

    nkiCompilerPopRecursion(cs);
    return true;
}

void fixupBreakJumpForContext(struct NKCompilerState *cs)
{
    while(cs->context->loopContextFixupCount) {
        uint32_t fixupLocation =
            cs->context->loopContextFixups[--cs->context->loopContextFixupCount];
        modifyJump(cs, fixupLocation, cs->instructionWriteIndex);
    }
    nkiFree(cs->vm, cs->context->loopContextFixups);
    cs->context->loopContextFixups = NULL;
}


void nkiCompilerPopContextCount(struct NKCompilerState *cs, uint32_t n)
{
    uint32_t k;
    for(k = 0; k < n; k++) {
        nkiCompilerPopContext(cs);
    }
}

bool nkiCompilerCompileWhileStatement(struct NKCompilerState *cs)
{
    uint32_t skipAddressWritePtr = 0;
    uint32_t startAddress = cs->instructionWriteIndex;

    if(!nkiCompilerPushRecursion(cs)) {
        return false;
    }

    nkiCompilerPushContext(cs);
    cs->context->isLoopContext = true;
    nkiCompilerPushContext(cs);

    // Skip "while("
    if(!nkiCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_WHILE) ||
        !nkiCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_PAREN_OPEN))
    {
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return false;
    }

    // Generate the expression code.
    if(!nkiCompilerCompileExpression(cs)) {
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return false;
    }

    // Add the NK_OP_JUMP_IF_ZERO, and save the literal address so we can
    // fill it in after we know how much we're going to have to skip.
    skipAddressWritePtr = emitJumpIfZero(cs, 0);

    // Skip ")"
    if(!nkiCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_PAREN_CLOSE)) {
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return false;
    }

    // Generate code to execute if test passes.
    nkiCompilerPushContext(cs);
    if(!nkiCompilerCompileStatement(cs)) {
        nkiCompilerPopContextCount(cs, 3);
        nkiCompilerPopRecursion(cs);
        return false;
    }
    nkiCompilerPopContext(cs);

    // Emit jump back to start.
    emitJump(cs, startAddress);

    // Fixup skip offset.
    modifyJump(cs, skipAddressWritePtr, cs->instructionWriteIndex);

    nkiCompilerPopContext(cs);
    fixupBreakJumpForContext(cs);
    nkiCompilerPopContext(cs);

    nkiCompilerPopRecursion(cs);
    return true;
}

bool nkiCompilerCompileForStatement(struct NKCompilerState *cs)
{
    uint32_t skipAddressWritePtr = 0;
    uint32_t loopStartAddress = 0;
    struct NKExpressionAstNode *incrementExpression = NULL;

    if(!nkiCompilerPushRecursion(cs)) {
        return false;
    }

    nkiCompilerPushContext(cs);
    cs->context->isLoopContext = true;

    // Just in case we want to declare a variable inline in the init
    // code.
    nkiCompilerPushContext(cs);

    // Skip "for("
    if(!nkiCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_FOR) ||
        !nkiCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_PAREN_OPEN))
    {
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return false;
    }

    // Generate the init expression code. Technically the first thing
    // can be a statement (so we can support variable declarations). I
    // really hope I don't regret that, because it'll mean some stupid
    // shit can go into that area.
    if(!nkiCompilerCompileStatement(cs)) {
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return false;
    }
    // If we ever go back to using expressions for the init thing,
    // we'll have to remember to pop the value it leaves behind,
    // decrement the stack frame offset, and then
    // EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_SEMICOLON).

    // Save the address we want to jump back to for the loop.
    loopStartAddress = cs->instructionWriteIndex;

    // Generate test expression code.
    if(!nkiCompilerCompileExpression(cs)) {
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return false;
    }
    skipAddressWritePtr = emitJumpIfZero(cs, 0);

    if(!nkiCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_SEMICOLON)) {
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return false;
    }

    // Parse the increment expression, but don't emit yet (we'll do
    // this at the end, before the jump back).
    incrementExpression = nkiCompilerCompileExpressionWithoutEmit(cs); // FIXME: Check return value.

    // Skip ")"
    if(!incrementExpression || !nkiCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_PAREN_CLOSE)) {
        deleteExpressionNode(cs->vm, incrementExpression);
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return false;
    }

    // Generate code to execute if test passes.
    nkiCompilerPushContext(cs);
    if(!nkiCompilerCompileStatement(cs)) {
        deleteExpressionNode(cs->vm, incrementExpression);
        nkiCompilerPopContextCount(cs, 3);
        nkiCompilerPopRecursion(cs);
        return false;
    }
    nkiCompilerPopContext(cs);

    // Emit the increment expression.
    if(!emitExpression(cs, incrementExpression)) {
        deleteExpressionNode(cs->vm, incrementExpression);
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return false;
    }
    nkiCompilerAddInstructionSimple(cs, NK_OP_POP, true);

    // Emit jump back to start.
    emitJump(cs, loopStartAddress);

    // Fixup skip offset.
    modifyJump(cs, skipAddressWritePtr, cs->instructionWriteIndex);

    deleteExpressionNode(cs->vm, incrementExpression);

    nkiCompilerPopContext(cs);
    fixupBreakJumpForContext(cs);
    nkiCompilerPopContext(cs);
    nkiCompilerPopRecursion(cs);
    return true;
}

bool nkiCompilerCompileBreakStatement(struct NKCompilerState *cs)
{
    uint32_t contextLevel = cs->context->stackFrameOffset;
    uint32_t loopContextLevel = 0;

    if(!nkiCompilerPushRecursion(cs)) {
        return false;
    }

    // Find a loop context we can break out of.
    struct NKCompilerStateContext *searchContext = cs->context;
    while(searchContext) {
        if(searchContext->isLoopContext) {
            break;
        }
        searchContext = searchContext->parent;
    }

    if(!searchContext) {
        nkiCompilerAddError(cs, "Cannot break outside of a loop.");
        nkiCompilerPopRecursion(cs);
        return false;
    }

    if(!searchContext->parent) {
        nkiCompilerAddError(cs, "Cannot break out of the root context.");
        nkiCompilerPopRecursion(cs);
        return false;
    }

    loopContextLevel = searchContext->stackFrameOffset;

    printf("Stack frame difference: %u\n", contextLevel - loopContextLevel);
    // assert(0);

    // Pop off every context's data between the break statement and
    // the loop context.
    nkiCompilerEmitPushLiteralInt(cs, contextLevel - loopContextLevel, false);
    nkiCompilerAddInstructionSimple(cs, NK_OP_POPN, false);

    {
        uint32_t jumpFixup = emitJump(cs, 0);
        searchContext->loopContextFixupCount++;
        searchContext->loopContextFixups =
            nkiRealloc(cs->vm, searchContext->loopContextFixups,
                sizeof(*searchContext->loopContextFixups) *
                searchContext->loopContextFixupCount);
        searchContext->loopContextFixups[
            searchContext->loopContextFixupCount - 1] =
            jumpFixup;
    }

    EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_BREAK);
    EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_SEMICOLON);

    nkiCompilerPopRecursion(cs);
    return true;
}

bool nkiCompilerPushRecursion(struct NKCompilerState *cs)
{
    // TODO: Make this limit configurable.
    if(cs->recursionCount > 64) {
        nkiCompilerAddError(cs, "Reached recursion limit during compilation.");
        return false;
    }
    cs->recursionCount++;
    return true;
}

void nkiCompilerPopRecursion(struct NKCompilerState *cs)
{
    assert(cs->recursionCount != 0);
    cs->recursionCount--;
}

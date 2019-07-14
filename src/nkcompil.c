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

void nkiCompilerAddInstruction(
    struct NKCompilerState *cs, struct NKInstruction *inst,
    nkbool adjustStackFrame)
{
    if(cs->instructionWriteIndex >= cs->vm->instructionAddressMask) {

        nkuint32_t oldSize = cs->vm->instructionAddressMask + 1;
        nkuint32_t newSize = oldSize << 1;

        cs->vm->instructionAddressMask <<= 1;
        cs->vm->instructionAddressMask |= 1;

        // This is a HARD limit because of the implementation and
        // must be spearate from the settable limit.
        if(cs->vm->instructionAddressMask >= ~(nkuint32_t)0) {
            nkiCompilerAddError(cs, "Too many instructions. Hit address space limit.");
            cs->vm->instructionAddressMask >>= 1;
        }

        cs->vm->instructions =
            (struct NKInstruction *)nkiReallocArray(
                cs->vm,
                cs->vm->instructions,
                sizeof(struct NKInstruction),
                newSize);

        // Clear the new area to NOPs.
        nkiMemset(
            cs->vm->instructions + oldSize, 0,
            (newSize - oldSize) * sizeof(struct NKInstruction));
    }

    cs->vm->instructions[cs->instructionWriteIndex] = *inst;

    // Adjust stack frame offset, if necessary.
    if(adjustStackFrame && cs->context) {
        cs->context->stackFrameOffset +=
            nkiCompilerStackOffsetTable[inst->opcode & (NK_OPCODE_PADDEDCOUNT - 1)];
    }

  #if NK_VM_DEBUG
    // FIXME: Swap this out for line/file markers.
    cs->vm->instructions[cs->instructionWriteIndex].lineNumber =
        nkiCompilerCurrentTokenLinenumber(cs);
    cs->vm->instructions[cs->instructionWriteIndex].fileIndex =
        nkiCompilerCurrentTokenFileIndex(cs);
  #endif

    cs->instructionWriteIndex++;
}

void nkiCompilerAddInstructionSimple(
    struct NKCompilerState *cs, enum NKOpcode opcode,
    nkbool adjustStackFrame)
{
    struct NKInstruction inst;
    nkiMemset(&inst, 0, sizeof(inst));
    inst.opcode = opcode;
    nkiCompilerAddInstruction(cs, &inst, adjustStackFrame);
}

void nkiCompilerPushContext(struct NKCompilerState *cs)
{
    struct NKCompilerStateContext *newContext =
        (struct NKCompilerStateContext *)nkiMalloc(
            cs->vm, sizeof(struct NKCompilerStateContext));
    nkiMemset(newContext, 0, sizeof(*newContext));
    newContext->currentFunctionId.id = NK_INVALID_VALUE;
    newContext->parent = cs->context;

    // Set stack frame offset.
    if(newContext->parent) {
        newContext->stackFrameOffset = newContext->parent->stackFrameOffset;
    }

    cs->context = newContext;
}

void nkiCompilerPopContext(struct NKCompilerState *cs)
{
    struct NKCompilerStateContext *oldContext =
        cs->context;

    assert(cs->context);

    cs->context = cs->context->parent;

    // Free variable data.
    {
        struct NKCompilerStateContextVariable *var =
            oldContext->variables;

        nkuint32_t popCount = 0;

        while(var) {
            struct NKCompilerStateContextVariable *next =
                var->next;

            // Count up the amount of stuff in this stack frame to get
            // rid of.
            if(!var->doNotPopWhenOutOfScope && !var->isGlobal) {
                popCount++;
            }

            // Free it.
            nkiFree(cs->vm, var->name);
            nkiFree(cs->vm, var);
            var = next;
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

            // nkuint32_t maybePushIntAddr =
            //     (cs->instructionWriteIndex - 3) & cs->vm->instructionAddressMask;
            // nkuint32_t maybeIntAddr =
            //     (cs->instructionWriteIndex - 2) & cs->vm->instructionAddressMask;
            // nkuint32_t maybePopNAddr =
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
                    nkiCompilerEmitPushLiteralInt(cs, popCount, nkfalse);
                    nkiCompilerAddInstructionSimple(cs, NK_OP_POPN, nkfalse);
                }

            // }
        }

    }

    // Free loop context jump fixups for break statements.
    nkiFree(cs->vm, oldContext->loopContextFixups);

    // Free the context itself.
    nkiFree(cs->vm, oldContext);
}

void nkiCompilerEmitPushLiteralInt(struct NKCompilerState *cs, nkint32_t value, nkbool adjustStackFrame)
{
    struct NKInstruction inst;

    // Add instruction.
    nkiMemset(&inst, 0, sizeof(inst));
    inst.opcode = NK_OP_PUSHLITERAL_INT;
    nkiCompilerAddInstruction(cs, &inst, adjustStackFrame);

    // Add parameter.
    nkiMemset(&inst, 0, sizeof(inst));
    inst.opData_int = value;
    nkiCompilerAddInstruction(cs, &inst, nkfalse);
}

void nkiCompilerEmitPushLiteralFunctionId(
    struct NKCompilerState *cs, NKVMInternalFunctionID functionId, nkbool adjustStackFrame)
{
    struct NKInstruction inst;

    // Add instruction.
    nkiMemset(&inst, 0, sizeof(inst));
    inst.opcode = NK_OP_PUSHLITERAL_FUNCTIONID;
    nkiCompilerAddInstruction(cs, &inst, adjustStackFrame);

    // Add parameter.
    nkiMemset(&inst, 0, sizeof(inst));
    inst.opData_functionId = functionId;
    nkiCompilerAddInstruction(cs, &inst, nkfalse);
}

void nkiCompilerEmitPushLiteralFloat(struct NKCompilerState *cs, float value, nkbool adjustStackFrame)
{
    struct NKInstruction inst;

    // Add instruction.
    nkiMemset(&inst, 0, sizeof(inst));
    inst.opcode = NK_OP_PUSHLITERAL_FLOAT;
    nkiCompilerAddInstruction(cs, &inst, adjustStackFrame);

    // Add parameter.
    nkiMemset(&inst, 0, sizeof(inst));
    inst.opData_float = value;
    nkiCompilerAddInstruction(cs, &inst, nkfalse);
}

void nkiCompilerEmitPushLiteralString(struct NKCompilerState *cs, const char *str, nkbool adjustStackFrame)
{
    struct NKInstruction inst;

    // Add instruction.
    nkiMemset(&inst, 0, sizeof(inst));
    inst.opcode = NK_OP_PUSHLITERAL_STRING;
    nkiCompilerAddInstruction(cs, &inst, adjustStackFrame);

    // Add string table entry data as op parameter.
    nkiMemset(&inst, 0, sizeof(inst));
    inst.opData_string =
        nkiVmStringTableFindOrAddString(
            cs->vm,
            str);
    nkiCompilerAddInstruction(cs, &inst, nkfalse);

    // Mark this string as not garbage-collected.
    {
        struct NKVMString *entry = nkiVmStringTableGetEntryById(
            &cs->vm->stringTable, inst.opData_string);
        if(entry) {
            entry->dontGC = nktrue;
        }
    }
}

void nkiCompilerEmitPushNil(struct NKCompilerState *cs, nkbool adjustStackFrame)
{
    nkiCompilerAddInstructionSimple(cs, NK_OP_PUSHNIL, adjustStackFrame);
}

nkuint32_t nkiCompilerAllocateStaticSpace(
    struct NKCompilerState *cs)
{
    struct NKVM *vm = cs->vm;

    // Expand our static address space if necessary.
    if((vm->staticAddressMask + 1) == cs->staticVariableCount) {

        vm->staticAddressMask <<= 1;
        vm->staticAddressMask |= 1;

        // Note: This limits the address space to 0xefffffff, because
        // we can't use that last bit. (The space allocation would
        // overflow to zero.)
        if(vm->staticAddressMask == ~(nkuint32_t)0) {
            nkiCompilerAddError(cs,
                "Address space exhaustion trying to add static variable.");
            return 0;
        }

        // Reallocate and clear out the new space.
        vm->staticSpace = (struct NKValue *)nkiReallocArray(
            vm, vm->staticSpace,
            sizeof(struct NKValue), (vm->staticAddressMask + 1));
        nkiMemset(
            &vm->staticSpace[cs->staticVariableCount], 0,
            (cs->staticVariableCount) * sizeof(struct NKValue));
    }

    // Add the variable.
    cs->staticVariableCount++;
    if(cs->staticVariableCount == 0) {
        nkiCompilerAddError(cs,
            "Address space exhaustion trying to add static variable.");
        return 0;
    }

    return cs->staticVariableCount - 1;
}

struct NKCompilerStateContextVariable *nkiCompilerAddVariable(
    struct NKCompilerState *cs, const char *name,
    nkbool useValueAtStackTop, nkbool emitInitCode)
{
    // This is a global variable if we're at a root-level context with
    // no parent.
    nkbool isGlobal = !cs->context->parent;
    struct NKCompilerStateContextVariable *var;
    struct NKCompilerStateContextVariable *checkVar;

    for(checkVar = cs->context->variables; checkVar; checkVar = checkVar->next) {
        if(!nkiStrcmp(checkVar->name, name)) {
            nkiCompilerAddError(cs,
                "Redundant variable declaration.");
            return NULL;
        }
    }

    var = (struct NKCompilerStateContextVariable *)nkiMalloc(
        cs->vm, sizeof(struct NKCompilerStateContextVariable));
    nkiMemset(var, 0, sizeof(*var));

    if(isGlobal) {

        // Add an instruction to make some stack space for this variable,
        // if needed.
        if(emitInitCode) {
            if(!useValueAtStackTop) {
                nkiCompilerEmitPushLiteralInt(cs, 0, nktrue);
            }
        }

        var->next = cs->context->variables;
        var->isGlobal = isGlobal;
        var->name = nkiStrdup(cs->vm, name);

        var->position = nkiCompilerAllocateStaticSpace(cs);

        // Set the global variable's initial value.
        if(emitInitCode) {
            nkiCompilerEmitPushLiteralInt(cs, var->position, nktrue);
            nkiCompilerAddInstructionSimple(cs, NK_OP_STATICPOKE, nktrue);
            nkiCompilerAddInstructionSimple(cs, NK_OP_POP, nktrue);
        }

    } else {

        // Add an instruction to make some stack space for this variable,
        // if needed.
        if(!useValueAtStackTop) {
            nkiCompilerEmitPushLiteralInt(cs, 0, nktrue);
        }

        var->next = cs->context->variables;
        var->isGlobal = isGlobal;
        var->name = nkiStrdup(cs->vm, name);
        var->position = cs->context->stackFrameOffset - 1;
    }

    cs->context->variables = var;

    return var;
}

nkbool nkiCompilerExpectAndSkipToken(
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
        return nkfalse;
    } else {
        nkiCompilerNextToken(cs);
    }
    return nktrue;
}

// TODO: Remove instances of this and replace them with
// nkiCompilerExpectAndSkipToken().
#define NK_EXPECT_AND_SKIP_STATEMENT(x)             \
    do {                                            \
        if(!nkiCompilerExpectAndSkipToken(cs, x)) { \
            nkiCompilerPopRecursion(cs);            \
            return nkfalse;                           \
        }                                           \
    } while(0)


nkbool nkiCompilerCompileStatement(struct NKCompilerState *cs)
{
    struct NKCompilerStateContext *startContext = cs->context;

    if(!nkiCompilerPushRecursion(cs)) {
        return nkfalse;
    }

    if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_INVALID) {
        nkiCompilerAddError(cs, "Ran out of tokens to parse.");
        return nkfalse;
    }

    switch(nkiCompilerCurrentTokenType(cs)) {

        case NK_TOKENTYPE_VAR:
            // "var" = Variable declaration.
            return nkiCompilerCompileVariableDeclaration(cs);

        case NK_TOKENTYPE_RETURN:
            // "return" = Return statement.
            return nkiCompilerCompileReturnStatement(cs);

        case NK_TOKENTYPE_CURLYBRACE_OPEN:
            // Curly braces mean we need to parse a block.
            if(!nkiCompilerCompileBlock(cs, nkfalse)) {
                return nkfalse;
            }
            break;

        case NK_TOKENTYPE_IF:
            // "if" statements.
            if(!nkiCompilerCompileIfStatement(cs)) {
                return nkfalse;
            }
            break;

        case NK_TOKENTYPE_WHILE:
            // "while" statements.
            if(!nkiCompilerCompileWhileStatement(cs)) {
                return nkfalse;
            }
            break;

        case NK_TOKENTYPE_DO:
            // "do/while" statements.
            if(!nkiCompilerCompileDoWhileStatement(cs)) {
                return nkfalse;
            }
            break;

        case NK_TOKENTYPE_FOR:
            // "for" statements.
            if(!nkiCompilerCompileForStatement(cs)) {
                return nkfalse;
            }
            break;

        case NK_TOKENTYPE_BREAK:
            // "break" statements.
            if(!nkiCompilerCompileBreakStatement(cs)) {
                return nkfalse;
            }
            break;

        case NK_TOKENTYPE_FUNCTION:
            // "function" = Function definition OR anonymous function.
        {
            struct NKToken *nextToken = nkiCompilerPeekToken(cs);
            if(nextToken && nextToken->type == NK_TOKENTYPE_IDENTIFIER) {
                if(!nkiCompilerCompileFunctionDefinition(cs, nkfalse, NULL)) {
                    return nkfalse;
                }
                break;
            }
        }
            // Intentional fall through to expression parsing.

        default:
            // Fall back to just parsing an expression.
            if(!nkiCompilerCompileExpression(cs)) {
                return nkfalse;
            }

            // Pop the expression result off the stack.
            if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_SEMICOLON) {
                nkiCompilerAddInstructionSimple(cs, NK_OP_POP, nktrue);
            }

            NK_EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_SEMICOLON);

            break;
    }

    assert(cs->context == startContext);
    nkiCompilerPopRecursion(cs);
    return nktrue;
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
            if(!nkiStrcmp(var->name, name)) {
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

nkbool nkiCompilerCompileBlock(struct NKCompilerState *cs, nkbool noBracesOrContext)
{
    struct NKCompilerStateContext *startContext = cs->context;

    if(!nkiCompilerPushRecursion(cs)) {
        return nkfalse;
    }

    if(!noBracesOrContext) {
        NK_EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_CURLYBRACE_OPEN);
        nkiCompilerPushContext(cs);
    }

    // Keep compiling statements until we see a curly brace or have an
    // error.
    while(
        nkiCompilerCurrentTokenType(cs) != NK_TOKENTYPE_CURLYBRACE_CLOSE &&
        nkiCompilerCurrentTokenType(cs) != NK_TOKENTYPE_INVALID)
    {
        if(!nkiCompilerCompileStatement(cs)) {

            if(!noBracesOrContext) {
                nkiCompilerPopContext(cs);
            }

            nkiCompilerPopRecursion(cs);
            return nkfalse;
        }
    }

    if(!noBracesOrContext) {
        nkiCompilerPopContext(cs);
        NK_EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_CURLYBRACE_CLOSE);
    }

    assert(startContext == cs->context);
    nkiCompilerPopRecursion(cs);

    return nktrue;
}

nkbool nkiCompilerCompileVariableDeclaration(struct NKCompilerState *cs)
{
    if(!nkiCompilerPushRecursion(cs)) {
        return nkfalse;
    }

    NK_EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_VAR);

    if(nkiCompilerCurrentTokenType(cs) != NK_TOKENTYPE_IDENTIFIER) {
        nkiCompilerAddError(cs, "Expected identifier in variable declaration.");
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }

    {
        const char *variableName = nkiCompilerCurrentTokenString(cs);

        NK_EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_IDENTIFIER);

        if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_ASSIGNMENT) {

            // Something in the form of "var foo = expression;" We'll just
            // treat the "foo = expression;" part as a separate thing.

            NK_EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_ASSIGNMENT);

            if(!nkiCompilerCompileExpression(cs)) {
                nkiCompilerPopRecursion(cs);
                return nkfalse;
            }

            nkiCompilerAddVariable(cs, variableName, nktrue, nktrue);

        } else {

            // Something in the form of "var foo;"

            nkiCompilerAddVariable(cs, variableName, nkfalse, nktrue);
        }
    }

    NK_EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_SEMICOLON);

    nkiCompilerPopRecursion(cs);
    return nktrue;
}

void nkiCompilerEmitReturn(struct NKCompilerState *cs)
{
    // Find the function we're in.
    struct NKCompilerStateContext *ctx = cs->context;
    struct NKVMFunction *func;
    while(ctx && ctx->currentFunctionId.id == NK_INVALID_VALUE) {
        ctx = ctx->parent;
    }

    if(!ctx) {
        nkiCompilerAddError(
            cs, "return statement outside of function.");
        return;
    }

    if(ctx->currentFunctionId.id >= cs->vm->functionCount) {
        nkiCompilerAddError(
            cs, "Bad function id when attempting to emit return.");
        return;
    }

    func = &cs->vm->functionTable[ctx->currentFunctionId.id];

    {
        // We want to throw away the context except for...
        //   The return value.   (-1)
        //   The return pointer. (-1)
        //   The argument list.  (-func->argumentCount)
        //   This thing we're about pushing right now. (-1)
        nkuint32_t throwAwayContext =
            cs->context->stackFrameOffset - func->argumentCount - 3;

        nkiCompilerEmitPushLiteralInt(cs, throwAwayContext, nkfalse);
        nkiCompilerAddInstructionSimple(cs, NK_OP_RETURN, nkfalse);
    }
}

nkbool nkiCompilerCompileReturnStatement(struct NKCompilerState *cs)
{
    if(!nkiCompilerPushRecursion(cs)) {
        return nkfalse;
    }

    NK_EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_RETURN);

    if(!nkiCompilerCompileExpression(cs)) {
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }

    nkiCompilerEmitReturn(cs);

    NK_EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_SEMICOLON);

    nkiCompilerPopRecursion(cs);

    return nktrue;
}

nkbool nkiCompilerCompileFunctionDefinition(
    struct NKCompilerState *cs,
    nkbool anonymousFunction,
    NKVMInternalFunctionID *outputId)
{
    nkbool ret = nktrue;
    const char *functionName = NULL;
    nkuint32_t skipOffset;
    struct NKCompilerStateContext *functionLocalContext;
    struct NKCompilerStateContext *savedContext;
    struct NKCompilerStateContext *searchContext;
    struct NKCompilerStateContextVariable *varTmp;
    nkuint32_t functionArgumentCount = 0;

    NKVMInternalFunctionID functionId;
    struct NKVMFunction *functionObject;

    functionId.id = NK_INVALID_VALUE;
    if(outputId) {
        *outputId = functionId;
    }

    if(!nkiCompilerPushRecursion(cs)) {
        return nkfalse;
    }

    functionObject = nkiVmCreateFunction(
        cs->vm, &functionId);

    NK_EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_FUNCTION);

    if(!anonymousFunction) {

        // Read the function name.
        if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_IDENTIFIER) {
            functionName = nkiCompilerCurrentTokenString(cs);
            nkiCompilerNextToken(cs);
        } else {
            nkiAddErrorEx(
                cs->vm,
                nkiCompilerCurrentTokenLinenumber(cs),
                nkiCompilerCurrentTokenFileIndex(cs),
                "Expected identifier for function name.");
            nkiCompilerPopRecursion(cs);
            return nkfalse;
        }

        // At the parent scope, create a variable with the name of the
        // function and give it an immediate value for the function.
        // Recursive function calls will not be possible if the
        // function cannot refer to itself before it's finished being
        // fully created.
        nkiCompilerEmitPushLiteralFunctionId(cs, functionId, nktrue);
        nkiCompilerAddVariable(cs, functionName, nktrue, nktrue);
    }

    // Add some instructions to skip around this function. This is
    // kind of a weird way to do it, but it means that we can just
    // start programs at instruction 0 and not worry about
    // functions in the middle. If declared at global scope, this
    // will only happen once per function so whatever.
    nkiCompilerEmitPushLiteralInt(cs, 0, nkfalse);
    skipOffset = cs->instructionWriteIndex - 1;
    nkiCompilerAddInstructionSimple(cs, NK_OP_JUMP_RELATIVE, nkfalse);

    // This context is different from the ones we'd normally push/pop,
    // because it's parented to the global context. So we're going to
    // set it and parent it directly.
    functionLocalContext = (struct NKCompilerStateContext *)nkiMalloc(
        cs->vm, sizeof(struct NKCompilerStateContext));
    nkiMemset(functionLocalContext, 0, sizeof(*functionLocalContext));
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
            varTmp = nkiCompilerAddVariable(cs, argumentName, nktrue, nktrue);

            // Thanks AFL!
            if(varTmp) {
                varTmp->doNotPopWhenOutOfScope = nktrue;
            }

            functionArgumentCount++;

            nkiCompilerNextToken(cs);

        } else if(nkiCompilerCurrentTokenType(cs) != NK_TOKENTYPE_PAREN_CLOSE) {

            nkiCompilerAddError(
                cs, "Expected identifier for function argument name.");
            ret = nkfalse;
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

    // Store the function start address on the function object.
    functionObject->firstInstructionIndex = cs->instructionWriteIndex;

    // Add the function id itself as a variable inside the function
    // (because it's on the stack anyway).
    varTmp = nkiCompilerAddVariable(cs, "_functionId", nktrue, nktrue);
    // Thanks AFL!
    if(varTmp) {
        varTmp->doNotPopWhenOutOfScope = nktrue;
    }
    cs->context->stackFrameOffset++;

    // We'll check this against the stored function argument count as
    // part of the CALL instruction, and throw an error if it doesn't
    // match. This will be pushed before the CALL.
    varTmp = nkiCompilerAddVariable(cs, "_argumentCount", nktrue, nktrue);
    // Thanks AFL!
    if(varTmp) {
        varTmp->doNotPopWhenOutOfScope = nktrue;
    }
    cs->context->stackFrameOffset++;

    // The CALL instruction will push this onto the stack
    // automatically.
    varTmp = nkiCompilerAddVariable(cs, "_returnPointer", nktrue, nktrue);
    // Thanks AFL!
    if(varTmp) {
        varTmp->doNotPopWhenOutOfScope = nktrue;
    }

    // Skip ')'.
    if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_PAREN_CLOSE) {
        nkiCompilerNextToken(cs);
    } else {
        nkiCompilerAddError(cs, "Expected ')'.");
    }

    // Parse and emit actual function code.
    if(!nkiCompilerCompileStatement(cs)) {
        ret = nkfalse;
    }

    // functionObject pointer may have been invalidated in the
    // recursive code because of a reallocation (from a function added
    // inside the function), so let's update it just in case.
    functionObject = &cs->vm->functionTable[functionId.id];

    // Add a RETURN instruction just in case the function reaches the
    // end without returning. (Zero is probably fine as a default
    // return value).
    nkiCompilerEmitPushLiteralInt(cs, 0, nkfalse);
    if(cs->context) {
        cs->context->stackFrameOffset++;
    }
    nkiCompilerEmitReturn(cs);

    // Go back and fix up our relative jump that skips this
    // function now that we know how long it is.
    if(cs->vm->instructions) {
        cs->vm->instructions[skipOffset].opData_int =
            cs->instructionWriteIndex - skipOffset - 2;
    }

    // Restore the "real" context back to the parent.
    nkiCompilerPopContext(cs);
    cs->context = savedContext;

    nkiCompilerPopRecursion(cs);

    if(outputId) {
        *outputId = functionId;
    }
    return ret;
}

struct NKToken *nkiCompilerNextToken(struct NKCompilerState *cs)
{
    if(cs->currentToken) {
        cs->currentToken = cs->currentToken->next;
        if(cs->currentToken) {

            if(cs->currentLineNumber != cs->currentToken->lineNumber ||
                cs->currentFileIndex != cs->currentToken->fileIndex)
            {
                cs->currentLineNumber = cs->currentToken->lineNumber;
                cs->currentFileIndex = cs->currentToken->fileIndex;

                printf(
                    "ASDF: " NK_PRINTF_UINT32 " - " NK_PRINTF_UINT32 " - " NK_PRINTF_UINT32 "\n",
                    cs->instructionWriteIndex, cs->currentLineNumber, cs->currentFileIndex);

                // TODO: Add a file/line marker to a list of markers, if
                // we decide to move that information out of the
                // instruction type (we do want to).
            }
        }
    }
    return cs->currentToken;
}

struct NKToken *nkiCompilerPeekToken(struct NKCompilerState *cs)
{
    if(cs->currentToken) {
        if(cs->currentToken->next) {
            return cs->currentToken->next;
        }
    }
    return NULL;
}

enum NKTokenType nkiCompilerCurrentTokenType(struct NKCompilerState *cs)
{
    if(cs->currentToken) {
        return cs->currentToken->type;
    }
    return NK_TOKENTYPE_INVALID;
}

nkuint32_t nkiCompilerCurrentTokenLinenumber(struct NKCompilerState *cs)
{
    if(cs->currentToken) {
        return cs->currentToken->lineNumber;
    }
    return cs->currentLineNumber;
}

nkuint32_t nkiCompilerCurrentTokenFileIndex(struct NKCompilerState *cs)
{
    if(cs->currentToken) {
        return cs->currentToken->fileIndex;
    }
    return cs->currentFileIndex;
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
    nkiAddErrorEx(
        cs->vm,
        nkiCompilerCurrentTokenLinenumber(cs),
        nkiCompilerCurrentTokenFileIndex(cs),
        error);
}

struct NKValue *nkiCompilerCreateGlobalVariable(
    struct NKCompilerState *cs,
    const char *name)
{
    struct NKVM *vm = cs->vm;

    // This is only something that should be used in the root context.
    // To do otherwise is a programming error by the hosting
    // application!
    assert(!cs->context->parent);

    {
        struct NKCompilerStateContextVariable *var =
            nkiCompilerAddVariable(cs, name, nkfalse, nkfalse);

        if(var && var->position != NK_INVALID_VALUE) {
            nkuint32_t position = nkiCompilerAllocateStaticSpace(cs);
            struct NKValue *val = &vm->staticSpace[position & vm->staticAddressMask];
            var->position = position;
            return val;
        }
    }

    return NULL;
}

void nkiCompilerCreateCFunctionVariable(
    struct NKCompilerState *cs,
    const char *name,
    NKVMFunctionCallback func)
{
    NKVMInternalFunctionID functionId = { NK_INVALID_VALUE };
    NKVMExternalFunctionID externalFunctionId = { NK_INVALID_VALUE };
    struct NKVM *vm = cs->vm;

    // Create an external and internal function ID.
    externalFunctionId = nkiVmRegisterExternalFunction(cs->vm, name, func);
    functionId = nkiVmGetOrCreateInternalFunctionForExternalFunction(vm, externalFunctionId);

    // Make a global variable for it.
    {
        struct NKValue funcVal;
        struct NKValue *staticVal;
        nkiMemset(&funcVal, 0, sizeof(funcVal));
        funcVal.type = NK_VALUETYPE_FUNCTIONID;
        funcVal.functionId = functionId;
        staticVal = nkiCompilerCreateGlobalVariable(cs, name);
        if(staticVal) {
            *staticVal = funcVal;
        }
    }
}

struct NKCompilerState *nkiCompilerCreate(
    struct NKVM *vm)
{
    NK_FAILURE_RECOVERY_DECL();

    struct NKCompilerState *cs;

    NK_SET_FAILURE_RECOVERY(NULL);

    cs = (struct NKCompilerState *)nkiMalloc(
        vm, sizeof(struct NKCompilerState));
    nkiMemset(cs, 0, sizeof(*cs));

    cs->instructionWriteIndex = 0;
    cs->vm = vm;
    cs->context = NULL;

    cs->currentToken = NULL;
    cs->currentLineNumber = 0;
    cs->currentFileIndex = NK_INVALID_VALUE;

    cs->staticVariableCount = 0;

    nkiCompilerPushContext(cs);

    NK_CLEAR_FAILURE_RECOVERY();

    return cs;
}

void nkiCompilerFinalize(
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
        nkuint32_t count = 0;
        struct NKCompilerStateContextVariable *var =
            cs->context->variables;
        while(var) {
            count++;
            var = var->next;
        }

        // Allocate that.
        cs->vm->globalVariables =
            (struct NKGlobalVariableRecord *)nkiMallocArray(
                cs->vm, sizeof(struct NKGlobalVariableRecord), count);
        cs->vm->globalVariableCount = count;

        // Now run through it all again and actually assign data.
        var = cs->context->variables;
        count = 0;
        while(var) {
            cs->vm->globalVariables[count].staticPosition = var->position;
            cs->vm->globalVariables[count].name = nkiStrdup(cs->vm, var->name);
            count++;
            var = var->next;
        }
    }

    // Pop the global context.
    while(cs->context) {
        nkiCompilerPopContext(cs);
    }

    // Add a final "END" instruction.
    nkiCompilerAddInstructionSimple(cs, NK_OP_END, nktrue);

    nkiFree(cs->vm, cs);
}

nkbool nkiCompilerCompileScript(
    struct NKCompilerState *cs,
    const char *script,
    const char *filename)
{
    struct NKTokenList tokenList;
    nkbool success;

    if(!nkiCompilerPushRecursion(cs)) {
        return nkfalse;
    }

    // Tokenize.

    nkiMemset(&tokenList, 0, sizeof(tokenList));
    tokenList.first = NULL;
    tokenList.last = NULL;

    success = nkiCompilerTokenize(cs->vm, script, &tokenList, filename);
    if(!success) {
        nkiCompilerDestroyTokenList(cs->vm, &tokenList);
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }

    // Compile.

    cs->currentToken = tokenList.first;
    cs->currentLineNumber =
        cs->currentToken ? cs->currentToken->lineNumber : 0;
    cs->currentFileIndex =
        cs->currentToken ? cs->currentToken->fileIndex : NK_INVALID_VALUE;

    success = nkiCompilerCompileBlock(cs, nktrue);

    cs->currentToken = NULL;
    cs->currentLineNumber = 0;
    cs->currentFileIndex = NK_INVALID_VALUE;
    nkiCompilerDestroyTokenList(cs->vm, &tokenList);

    nkiCompilerPopRecursion(cs);
    return success;
}

nkuint32_t nkiCompilerEmitJump(struct NKCompilerState *cs, nkuint32_t target)
{
    nkuint32_t instructionWriteIndex = cs->instructionWriteIndex;
    nkiCompilerEmitPushLiteralInt(cs, (target - instructionWriteIndex) - 3, nktrue);
    nkiCompilerAddInstructionSimple(cs, NK_OP_JUMP_RELATIVE, nktrue);
    return instructionWriteIndex;
}

// Note: Pops +1 item off the stack.
nkuint32_t nkiCompilerEmitJumpIfZero(struct NKCompilerState *cs, nkuint32_t target)
{
    nkuint32_t instructionWriteIndex = cs->instructionWriteIndex;
    nkiCompilerEmitPushLiteralInt(cs, (target - instructionWriteIndex) - 3, nktrue);
    nkiCompilerAddInstructionSimple(cs, NK_OP_JUMP_IF_ZERO, nktrue);
    return instructionWriteIndex;
}

struct NKInstruction *nkiCompilerGetInstruction(struct NKCompilerState *cs, nkuint32_t address)
{
    return &cs->vm->instructions[cs->vm->instructionAddressMask & address];
}

void nkiCompilerModifyJump(
    struct NKCompilerState *cs,
    nkuint32_t pushLiteralBeforeJumpAddress,
    nkuint32_t target)
{
    struct NKInstruction *pushLitInst =
        nkiCompilerGetInstruction(cs, pushLiteralBeforeJumpAddress);
    struct NKInstruction *jumpInst =
        nkiCompilerGetInstruction(cs, pushLiteralBeforeJumpAddress + 2);

    assert(pushLitInst->opcode == NK_OP_PUSHLITERAL_INT);
    assert(
        jumpInst->opcode == NK_OP_JUMP_RELATIVE ||
        jumpInst->opcode == NK_OP_JUMP_IF_ZERO);

    nkiCompilerGetInstruction(cs, pushLiteralBeforeJumpAddress + 1)->opData_int =
        (target - pushLiteralBeforeJumpAddress) - 3;
}

nkbool nkiCompilerCompileIfStatement(struct NKCompilerState *cs)
{
    nkuint32_t skipAddressWritePtr = 0;

    if(!nkiCompilerPushRecursion(cs)) {
        return nkfalse;
    }

    // Skip "if("
    NK_EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_IF);
    NK_EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_PAREN_OPEN);

    // Generate the expression code.
    if(!nkiCompilerCompileExpression(cs)) {
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }

    // Add the NK_OP_JUMP_IF_ZERO, and save the literal address so we can
    // fill it in after we know how much we're going to have to skip.
    skipAddressWritePtr = nkiCompilerEmitJumpIfZero(cs, 0);

    // Skip ")"
    NK_EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_PAREN_CLOSE);

    // Generate code to execute if test passes.
    nkiCompilerPushContext(cs);
    if(!nkiCompilerCompileStatement(cs)) {
        nkiCompilerPopContext(cs);
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }
    nkiCompilerPopContext(cs);

    // Fixup skip offset.
    nkiCompilerModifyJump(cs, skipAddressWritePtr, cs->instructionWriteIndex);

    if(nkiCompilerCurrentTokenType(cs) == NK_TOKENTYPE_ELSE) {

        nkuint32_t skipAddressWritePtrElse = 0;

        NK_EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_ELSE);

        // Emit instructions to skip past the contents of the "else"
        // block. Keep the index of the relative offset here so we can
        // go back and modify it after the inner code is complete.
        skipAddressWritePtrElse = nkiCompilerEmitJump(cs, 0);

        // Increase our skip amount to account for the
        // NK_OP_PUSHLITERAL_INT and NK_OP_JUMP_RELATIVE we added to skip
        // past the "else" clause.
        nkiCompilerModifyJump(cs, skipAddressWritePtr, cs->instructionWriteIndex);

        // Generate code to execute if test fails.
        nkiCompilerPushContext(cs);
        if(!nkiCompilerCompileStatement(cs)) {
            nkiCompilerPopContext(cs);
            nkiCompilerPopRecursion(cs);
            return nkfalse;
        }
        nkiCompilerPopContext(cs);

        // Fixup "else" skip offset.
        nkiCompilerModifyJump(cs, skipAddressWritePtrElse, cs->instructionWriteIndex);
    }

    nkiCompilerPopRecursion(cs);
    return nktrue;
}

void nkiCompilerFixupBreakJumpForContext(struct NKCompilerState *cs)
{
    while(cs->context->loopContextFixupCount) {
        nkuint32_t fixupLocation =
            cs->context->loopContextFixups[--cs->context->loopContextFixupCount];
        nkiCompilerModifyJump(cs, fixupLocation, cs->instructionWriteIndex);
    }
    nkiFree(cs->vm, cs->context->loopContextFixups);
    cs->context->loopContextFixups = NULL;
}


void nkiCompilerPopContextCount(struct NKCompilerState *cs, nkuint32_t n)
{
    nkuint32_t k;
    for(k = 0; k < n; k++) {
        nkiCompilerPopContext(cs);
    }
}

nkbool nkiCompilerCompileWhileStatement(struct NKCompilerState *cs)
{
    nkuint32_t skipAddressWritePtr = 0;
    nkuint32_t startAddress = cs->instructionWriteIndex;

    if(!nkiCompilerPushRecursion(cs)) {
        return nkfalse;
    }

    nkiCompilerPushContext(cs);
    cs->context->isLoopContext = nktrue;
    nkiCompilerPushContext(cs);

    // Skip "while("
    if(!nkiCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_WHILE) ||
        !nkiCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_PAREN_OPEN))
    {
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }

    // Generate the expression code.
    if(!nkiCompilerCompileExpression(cs)) {
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }

    // Add the NK_OP_JUMP_IF_ZERO, and save the literal address so we can
    // fill it in after we know how much we're going to have to skip.
    skipAddressWritePtr = nkiCompilerEmitJumpIfZero(cs, 0);

    // Skip ")"
    if(!nkiCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_PAREN_CLOSE)) {
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }

    // Generate code to execute if test passes.
    nkiCompilerPushContext(cs);
    if(!nkiCompilerCompileStatement(cs)) {
        nkiCompilerPopContextCount(cs, 3);
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }
    nkiCompilerPopContext(cs);

    // Emit jump back to start.
    nkiCompilerEmitJump(cs, startAddress);

    // Fixup skip offset.
    nkiCompilerModifyJump(cs, skipAddressWritePtr, cs->instructionWriteIndex);

    nkiCompilerPopContext(cs);
    nkiCompilerFixupBreakJumpForContext(cs);
    nkiCompilerPopContext(cs);

    nkiCompilerPopRecursion(cs);
    return nktrue;
}

nkbool nkiCompilerCompileDoWhileStatement(struct NKCompilerState *cs)
{
    nkuint32_t startAddress = cs->instructionWriteIndex;

    if(!nkiCompilerPushRecursion(cs)) {
        return nkfalse;
    }

    // Setup context.
    nkiCompilerPushContext(cs);
    cs->context->isLoopContext = nktrue;
    nkiCompilerPushContext(cs);

    // Skip "do"
    if(!nkiCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_DO)) {
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }

    // Generate code to execute if test passes.
    nkiCompilerPushContext(cs);
    if(!nkiCompilerCompileStatement(cs)) {
        nkiCompilerPopContextCount(cs, 3);
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }
    nkiCompilerPopContext(cs);

    // Skip "while("
    if(!nkiCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_WHILE) ||
        !nkiCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_PAREN_OPEN))
    {
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }

    // Generate the expression code.
    if(!nkiCompilerCompileExpression(cs)) {
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }

    // Skip ")"
    if(!nkiCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_PAREN_CLOSE)) {
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }

    // Emit conditional jump back to start.
    nkiCompilerAddInstructionSimple(cs, NK_OP_NOT, nktrue);
    nkiCompilerEmitJumpIfZero(cs, startAddress);

    // Tear down context.
    nkiCompilerPopContext(cs);
    nkiCompilerFixupBreakJumpForContext(cs);
    nkiCompilerPopContext(cs);

    nkiCompilerPopRecursion(cs);

    // Skip ";"
    if(!nkiCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_SEMICOLON)) {
        return nkfalse;
    }

    return nktrue;

}

nkbool nkiCompilerCompileForStatement(struct NKCompilerState *cs)
{
    nkuint32_t skipAddressWritePtr = 0;
    nkuint32_t loopStartAddress = 0;
    struct NKExpressionAstNode *incrementExpression = NULL;

    if(!nkiCompilerPushRecursion(cs)) {
        return nkfalse;
    }

    nkiCompilerPushContext(cs);
    cs->context->isLoopContext = nktrue;

    // Just in case we want to declare a variable inline in the init
    // code.
    nkiCompilerPushContext(cs);

    // Skip "for("
    if(!nkiCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_FOR) ||
        !nkiCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_PAREN_OPEN))
    {
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }

    // Generate the init expression code. Technically the first thing
    // can be a statement (so we can support variable declarations). I
    // really hope I don't regret that, because it'll mean some stupid
    // shit can go into that area.
    if(!nkiCompilerCompileStatement(cs)) {
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }
    // If we ever go back to using expressions for the init thing,
    // we'll have to remember to pop the value it leaves behind,
    // decrement the stack frame offset, and then
    // NK_EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_SEMICOLON).

    // Save the address we want to jump back to for the loop.
    loopStartAddress = cs->instructionWriteIndex;

    // Generate test expression code.
    if(!nkiCompilerCompileExpression(cs)) {
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }
    skipAddressWritePtr = nkiCompilerEmitJumpIfZero(cs, 0);

    if(!nkiCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_SEMICOLON)) {
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }

    // Parse the increment expression, but don't emit yet (we'll do
    // this at the end, before the jump back).
    incrementExpression = nkiCompilerCompileExpressionWithoutEmit(cs);

    // Skip ")"
    if(!incrementExpression || !nkiCompilerExpectAndSkipToken(cs, NK_TOKENTYPE_PAREN_CLOSE)) {
        nkiCompilerDeleteExpressionNode(cs->vm, incrementExpression);
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }

    // Generate code to execute if test passes.
    nkiCompilerPushContext(cs);
    if(!nkiCompilerCompileStatement(cs)) {
        nkiCompilerDeleteExpressionNode(cs->vm, incrementExpression);
        nkiCompilerPopContextCount(cs, 3);
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }
    nkiCompilerPopContext(cs);

    // Emit the increment expression.
    if(!nkiCompilerEmitExpression(cs, incrementExpression)) {
        nkiCompilerDeleteExpressionNode(cs->vm, incrementExpression);
        nkiCompilerPopContextCount(cs, 2);
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }
    nkiCompilerAddInstructionSimple(cs, NK_OP_POP, nktrue);

    // Emit jump back to start.
    nkiCompilerEmitJump(cs, loopStartAddress);

    // Fixup skip offset.
    nkiCompilerModifyJump(cs, skipAddressWritePtr, cs->instructionWriteIndex);

    nkiCompilerDeleteExpressionNode(cs->vm, incrementExpression);

    nkiCompilerPopContext(cs);
    nkiCompilerFixupBreakJumpForContext(cs);
    nkiCompilerPopContext(cs);
    nkiCompilerPopRecursion(cs);
    return nktrue;
}

nkbool nkiCompilerCompileBreakStatement(struct NKCompilerState *cs)
{
    nkuint32_t contextLevel = cs->context->stackFrameOffset;
    nkuint32_t loopContextLevel = 0;
    struct NKCompilerStateContext *searchContext;

    if(!nkiCompilerPushRecursion(cs)) {
        return nkfalse;
    }

    // Find a loop context we can break out of.
    searchContext = cs->context;
    while(searchContext) {
        if(searchContext->isLoopContext) {
            break;
        }
        searchContext = searchContext->parent;
    }

    if(!searchContext) {
        nkiCompilerAddError(cs, "Cannot break outside of a loop.");
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }

    if(!searchContext->parent) {
        nkiCompilerAddError(cs, "Cannot break out of the root context.");
        nkiCompilerPopRecursion(cs);
        return nkfalse;
    }

    loopContextLevel = searchContext->stackFrameOffset;

    // Pop off every context's data between the break statement and
    // the loop context.
    nkiCompilerEmitPushLiteralInt(cs, contextLevel - loopContextLevel, nkfalse);
    nkiCompilerAddInstructionSimple(cs, NK_OP_POPN, nkfalse);

    {
        nkuint32_t jumpFixup = nkiCompilerEmitJump(cs, 0);
        searchContext->loopContextFixupCount++;
        searchContext->loopContextFixups =
            (nkuint32_t *)nkiReallocArray(cs->vm, searchContext->loopContextFixups,
                sizeof(*searchContext->loopContextFixups),
                searchContext->loopContextFixupCount);
        searchContext->loopContextFixups[
            searchContext->loopContextFixupCount - 1] =
            jumpFixup;
    }

    NK_EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_BREAK);
    NK_EXPECT_AND_SKIP_STATEMENT(NK_TOKENTYPE_SEMICOLON);

    nkiCompilerPopRecursion(cs);
    return nktrue;
}

nkbool nkiCompilerPushRecursion(struct NKCompilerState *cs)
{
    // TODO: Make this limit configurable.
    if(cs->recursionCount > 128) {
        nkiCompilerAddError(cs, "Reached recursion limit during compilation.");
        return nkfalse;
    }
    cs->recursionCount++;
    return nktrue;
}

void nkiCompilerPopRecursion(struct NKCompilerState *cs)
{
    assert(cs->recursionCount != 0);
    cs->recursionCount--;
}

#undef NK_EXPECT_AND_SKIP_STATEMENT


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
            free(var->name);
            free(var);
            var = next;

            // TODO: Emit "pop" instructions.
            addInstructionSimple(cs, OP_POP);
            oldContext->stackFrameOffset--;

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

void addVariableWithoutStackAllocation(struct CompilerState *cs, const char *name)
{
    // Add a variable to our context.
    {
        struct CompilerStateContextVariable *var =
            malloc(sizeof(struct CompilerStateContextVariable));
        memset(var, 0, sizeof(*var));

        var->next = cs->context->variables;
        var->isGlobal = !cs->context->parent;
        var->name = strdup(name);
        var->stackPos = cs->context->stackFrameOffset - 1;

        cs->context->variables = var;
    }

    dbgWriteLine("Variable added.");
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

        case TOKENTYPE_CURLYBRACE_OPEN:
            // Curly braces mean we need to parse a block.
            return compileBlock(cs, currentToken);

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

bool compileBlock(struct CompilerState *cs, struct Token **currentToken)
{
    EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_CURLYBRACE_OPEN);

    pushContext(cs);

    while(*currentToken && (*currentToken)->type != TOKENTYPE_CURLYBRACE_CLOSE) {
        if(!compileStatement(cs, currentToken)) {
            return false;
        }
    }

    popContext(cs);

    EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_CURLYBRACE_CLOSE);

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


#include "common.h"

void addInstruction(struct CompilerState *cs, struct Instruction *inst)
{
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

void addVariable(struct CompilerState *cs, const char *name)
{
    // Add an instruction to make some stack space for this variable.
    struct Instruction inst;
    memset(&inst, 0, sizeof(inst));
    inst.opcode = OP_PUSHLITERAL;
    inst.pushLiteralData.value.type = VALUETYPE_INT;
    inst.pushLiteralData.value.intData = 0;
    addInstruction(cs, &inst);

    // Add a variable to our context.
    {
        struct CompilerStateContextVariable *var =
            malloc(sizeof(struct CompilerStateContextVariable));
        memset(var, 0, sizeof(*var));

        var->next = cs->context->variables;
        var->isGlobal = !cs->context->parent;
        var->name = strdup(name);
        var->stackPos = cs->context->stackFrameOffset++;

        cs->context->variables = var;
    }

    dbgWriteLine("Variable added.");
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
        }                                                           \
        *currentToken = (*currentToken)->next;                      \
    } while(0)

bool compileStatement(struct CompilerState *cs, struct Token **currentToken)
{
    if(!*currentToken) {
        errorStateAddError(
            &cs->vm->errorState, -1, "Ran out of tokens to parse.");
        return false;
    }

    switch((*currentToken)->type) {

        // TODO: Other statement types.

        default:

            if(!compileExpression(cs, currentToken)) {
                return false;
            }

            EXPECT_AND_SKIP_STATEMENT(TOKENTYPE_SEMICOLON);

            // TODO: Remove this. (Debugging output.)
            {
                addInstructionSimple(cs, OP_DUMP);
                cs->context->stackFrameOffset--;
            }

            break;
    }


    return true;
}

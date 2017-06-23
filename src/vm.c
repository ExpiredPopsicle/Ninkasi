#include "common.h"

// ----------------------------------------------------------------------
// Static opcode table setup.

typedef void (*VMOpcodeCall)(struct VM *vm, struct Instruction *instruction);
static VMOpcodeCall opcodeTable[OPCODE_PADDEDCOUNT];
static void vmInitOpcodeTable(void)
{
    assert(OPCODE_PADDEDCOUNT >= OPCODE_REALCOUNT);

    opcodeTable[OP_ADD]         = opcode_add;
    opcodeTable[OP_SUBTRACT]    = opcode_subtract;
    opcodeTable[OP_MULTIPLY]    = opcode_multiply;
    opcodeTable[OP_DIVIDE]      = opcode_divide;
    opcodeTable[OP_NEGATE]      = opcode_negate;
    opcodeTable[OP_PUSHLITERAL] = opcode_pushLiteral;
    opcodeTable[OP_NOP]         = opcode_nop;

    // Fill in the rest of the opcode table with no-ops. We just want
    // to pad up to a power of two so we can easily mask instructions
    // instead of branching to make sure they're valid.
    {
        uint32_t i;
        for(i = OPCODE_REALCOUNT; i < OPCODE_PADDEDCOUNT; i++) {
            opcodeTable[i] = opcode_nop;
        }
    }
}

// ----------------------------------------------------------------------
// Init/shutdown

void vmInit(struct VM *vm)
{
    vmInitOpcodeTable();

    errorStateInit(&vm->errorState);
    vmStackInit(&vm->stack);
    vm->instructionPointer = 0;

    vm->instructions =
        malloc(sizeof(struct Instruction) * 4);
    vm->instructionAddressMask = 0x3;
}

void vmDestroy(struct VM *vm)
{
    vmStackDestroy(&vm->stack);
    errorStateDestroy(&vm->errorState);
    free(vm->instructions);
}

// ----------------------------------------------------------------------
// Iteration

void vmIterate(struct VM *vm)
{
    struct Instruction *inst = &vm->instructions[
        vm->instructionPointer & vm->instructionAddressMask];
    opcodeTable[inst->opcode & (OPCODE_PADDEDCOUNT - 1)](vm, inst);
    vm->instructionPointer++;
}

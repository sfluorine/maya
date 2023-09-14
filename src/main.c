#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>

#define MAYA_STACK_CAP 1024
#define MAYA_REGISTERS_CAP 5

typedef enum MayaError_t {
    ERR_OK,
    ERR_STACK_OVERFLOW,
    ERR_STACK_UNDERFLOW,
    ERR_INVALID_OPERAND,
    ERR_INVALID_INSTRUCTION,
} MayaError;

const char* maya_error_to_str(MayaError error) {
    switch (error) {
    case ERR_OK:
        return "OK";
    case ERR_STACK_OVERFLOW:
        return "STACK OVERFLOW";
    case ERR_STACK_UNDERFLOW:
        return "STACK UNDERFLOW";
    case ERR_INVALID_OPERAND:
        return "INVALID OPERAND";
    case ERR_INVALID_INSTRUCTION:
        return "INVALID INSTRUCTION";
    }
}

typedef enum MayaOpCode_t {
    OP_HALT,
    OP_PUSH,
    OP_POP,
    OP_DUP,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_JMP,
    OP_JEQ,
    OP_JNEQ,
    OP_JGT,
    OP_JLT,
    OP_CALL,
    OP_RET,
    OP_LOAD,
    OP_STORE,
} MayaOpCode;

typedef struct MayaInstruction_t {
    MayaOpCode opcode;
    int64_t operand;
} MayaInstruction;

typedef struct MayaVm_t {
    size_t pc; // program counter
    size_t sp; // stack pointer
    MayaInstruction* program;
    int64_t stack[MAYA_STACK_CAP];
    int64_t registers[MAYA_REGISTERS_CAP];
    bool halt;
} MayaVm;

void maya_init(MayaVm* maya, MayaInstruction* program) {
    assert(maya != NULL);
    assert(program != NULL);

    maya->pc = 0;
    maya->sp = 0;
    maya->program = program;

    for (size_t i = 0; i < MAYA_REGISTERS_CAP; i++)
        maya->registers[i] = 0;
}

void maya_debug_stack(MayaVm* maya) {
    printf("STACK:\n");

    for (size_t i = 0; i < maya->sp; i++)
        printf("%ld\n", maya->stack[i]);
}

void maya_debug_registers(MayaVm* maya) {
    printf("REGISTERS:\n");

    for (size_t i = 0; i < MAYA_REGISTERS_CAP; i++)
        printf("%c: %ld\n", 'a' + (char)i, maya->registers[i]);
}

MayaError maya_push_stack(MayaVm* maya, int64_t value) {
    if (maya->sp >= MAYA_STACK_CAP)
        return ERR_STACK_OVERFLOW;

    maya->stack[maya->sp++] = value;
    return ERR_OK;
}

MayaError maya_pop_stack(MayaVm* maya, int64_t* output) {
    if (maya->sp <= 0)
        return ERR_STACK_UNDERFLOW;

    if (output != NULL)
        *output = maya->stack[maya->sp - 1];
    
    maya->sp--;
    return ERR_OK;
}

MayaError maya_execute_instruction(MayaVm* maya, MayaInstruction instruction) {
    MayaError error = ERR_OK;
    int64_t temps[2];

    switch (instruction.opcode) {
    case OP_HALT:
        maya->halt = true;
        maya->pc++;
        break;
    case OP_PUSH:
        error = maya_push_stack(maya, instruction.operand);
        maya->pc++;
        break;
    case OP_POP:
        error = maya_pop_stack(maya, NULL);
        maya->pc++;
        break;
    case OP_DUP:
        if (maya->sp - instruction.operand < 0)
            return ERR_STACK_UNDERFLOW;

        error = maya_push_stack(maya, maya->stack[maya->sp - instruction.operand]);
        maya->pc++;
        break;
    case OP_ADD:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        maya_push_stack(maya, temps[0] + temps[1]);

        maya->pc++;
        break;
    case OP_SUB:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        maya_push_stack(maya, temps[0] - temps[1]);

        maya->pc++;
        break;
    case OP_MUL:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        maya_push_stack(maya, temps[0] * temps[1]);

        maya->pc++;
        break;
    case OP_DIV:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        maya_push_stack(maya, temps[0] / temps[1]);

        maya->pc++;
        break;
    case OP_JMP:
        maya->pc = instruction.operand;
        break;
    case OP_JEQ:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0] == temps[1]) {
            maya->pc = instruction.operand;
        } else {
            maya->pc++;
        }

        break;
    case OP_JNEQ:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0] != temps[1]) {
            maya->pc = instruction.operand;
        } else {
            maya->pc++;
        }

        break;
    case OP_JGT:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0] > temps[1]) {
            maya->pc = instruction.operand;
        } else {
            maya->pc++;
        }

        break;
    case OP_JLT:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0] < temps[1]) {
            maya->pc = instruction.operand;
        } else {
            maya->pc++;
        }

        break;
    case OP_CALL:
        error = maya_push_stack(maya, maya->pc);
        maya->pc = instruction.operand;
        break;
    case OP_RET:
        error = maya_pop_stack(maya, &temps[0]);
        maya->pc = temps[0];
        break;
    case OP_LOAD:
        if (instruction.operand < 0 || instruction.operand >= MAYA_REGISTERS_CAP)
            return ERR_INVALID_OPERAND;

        error = maya_push_stack(maya, maya->registers[instruction.operand]);
        maya->pc++;
        break;
    case OP_STORE:
        if (instruction.operand < 0 || instruction.operand >= MAYA_REGISTERS_CAP)
            return ERR_INVALID_OPERAND;

        error = maya_pop_stack(maya, &maya->registers[instruction.operand]);
        maya->pc++;
        break;
    default:
        return ERR_INVALID_INSTRUCTION;
    }

    return error;
}

void maya_execute_program(MayaVm* maya) {
    while (!maya->halt) {
        MayaError error = maya_execute_instruction(maya, maya->program[maya->pc]);
        if (error != ERR_OK) {
            fprintf(stderr, "ERROR: %s\n", maya_error_to_str(error));
            return;
        }
    }
}

int main() {
    // this program computes factorial of 12, which is 479,001,600.
    MayaInstruction program[] = {
        { OP_PUSH, 1 },
        { OP_STORE, 0 },

        { OP_PUSH, 1 },

        { OP_LOAD, 0 },
        { OP_PUSH, 1 },
        { OP_ADD },

        { OP_DUP, 1 },
        { OP_STORE, 0 },

        { OP_MUL },

        { OP_LOAD, 0 },
        { OP_PUSH, 12 },

        { OP_JNEQ, 3 },

        { OP_HALT }
    };

    MayaVm maya;
    maya_init(&maya, program);
    maya_execute_program(&maya);
    maya_debug_stack(&maya);
    maya_debug_registers(&maya);
}

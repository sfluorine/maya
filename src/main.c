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
} MayaError;

const char* maya_error_to_str(MayaError error) {
    switch (error) {
    case ERR_OK:
        return "OK";
    case ERR_STACK_OVERFLOW:
        return "STACK OVERFLOW";
    case ERR_STACK_UNDERFLOW:
        return "STACK UNDERFLOW";
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
    int64_t stack[MAYA_STACK_CAP];
    int64_t registers[MAYA_REGISTERS_CAP];
    MayaInstruction* program;
    bool halt;
} MayaVm;

void maya_init(MayaVm* maya, MayaInstruction* program) {
    assert(maya != NULL);
    assert(program != NULL);

    maya->pc = 0;
    maya->sp = 0;
    maya->program = program;
}

void maya_debug_stack(MayaVm* maya) {
    printf("STACK:\n");

    for (size_t i = 0; i < maya->sp; i++)
        printf("%ld\n", maya->stack[i]);
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
    case OP_STORE:
        break;
    }

    return error;
}

void maya_execute_program(MayaVm* maya) {
    while (!maya->halt) {
        maya_debug_stack(maya);
        MayaError error = maya_execute_instruction(maya, maya->program[maya->pc]);
        if (error != ERR_OK) {
            fprintf(stderr, "ERROR: %s\n", maya_error_to_str(error));
            return;
        }
    }
}

int main() {
    MayaInstruction program[] = {
        { OP_PUSH, 1 },
        { OP_PUSH, 1 },
        { OP_ADD },
        { OP_DUP, 1 },
        { OP_PUSH, 10 },
        { OP_JLT, 1 },
        { OP_HALT },
    };

    MayaVm maya;
    maya_init(&maya, program);
    maya_execute_program(&maya);
    maya_debug_stack(&maya);
}

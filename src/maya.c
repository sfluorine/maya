#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "maya.h"

static const char* maya_error_to_str(MayaError error) {
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

static MayaError maya_push_stack(MayaVm* maya, int64_t value) {
    if (maya->sp >= MAYA_STACK_CAP)
        return ERR_STACK_OVERFLOW;

    maya->stack[maya->sp++] = value;
    return ERR_OK;
}

static MayaError maya_pop_stack(MayaVm* maya, int64_t* output) {
    if (maya->sp <= 0)
        return ERR_STACK_UNDERFLOW;

    if (output != NULL)
        *output = maya->stack[maya->sp - 1];
    
    maya->sp--;
    return ERR_OK;
}

static MayaError maya_execute_instruction(MayaVm* maya, MayaInstruction instruction) {
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

static void maya_init(MayaVm* maya, MayaInstruction* program) {
    maya->pc = 0;
    maya->sp = 0;
    maya->program = program;

    for (size_t i = 0; i < MAYA_REGISTERS_CAP; i++)
        maya->registers[i] = 0;

    maya->halt = false;
}

static void maya_debug_stack(MayaVm* maya) {
    printf("STACK:\n");

    for (size_t i = 0; i < maya->sp; i++)
        printf("%ld\n", maya->stack[i]);
}

static void maya_debug_registers(MayaVm* maya) {
    printf("REGISTERS:\n");

    for (size_t i = 0; i < MAYA_REGISTERS_CAP; i++)
        printf("%c: %ld\n", 'a' + (char)i, maya->registers[i]);
}

static void maya_execute_program(MayaVm* maya) {
    while (!maya->halt) {
        MayaError error = maya_execute_instruction(maya, maya->program[maya->pc]);
        if (error != ERR_OK) {
            fprintf(stderr, "ERROR: %s\n", maya_error_to_str(error));
            return;
        }
    }
}

static MayaInstruction* maya_load_program_from_file(const char* input_file) {
    FILE* input_stream = fopen(input_file, "rb");
    if (!input_stream) {
        fprintf(stderr, "ERROR: cannot open file '%s'\n", input_file);
        exit(EXIT_FAILURE);
    }

    fseek(input_stream, 0, SEEK_END);
    long size = ftell(input_stream) / sizeof(MayaInstruction);
    fseek(input_stream, 0, SEEK_SET);

    MayaInstruction* instruction = malloc(sizeof(MayaInstruction) * size);
    if (!instruction) {
        fprintf(stderr, "ERROR: cannot allocate memory!\n");
        exit(EXIT_FAILURE);
    }

    fread(instruction, sizeof(MayaInstruction), size, input_stream);
    if (ferror(input_stream)) {
        fprintf(stderr, "ERROR: error while reading file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    fclose(input_stream);
    return instruction;
}

static char* shift(int* argc, char*** argv) {
    if (*argc == 0)
        return NULL;

    char* arg = *argv[0];
    *argv += 1;
    *argc -= 1;

    return arg;
}

static void usage(const char* program) {
    fprintf(stderr, "Usage: %s <input_file.maya>\n", program);
}

int main(int argc, char** argv) {
    const char* program_name = shift(&argc, &argv);

    if (argc == 0) {
        usage(program_name);
        fprintf(stderr, "ERROR: expected input file!\n");
        return 1;
    }

    const char* input_file = shift(&argc, &argv);
    MayaInstruction* program = maya_load_program_from_file(input_file);

    MayaVm maya;
    maya_init(&maya, program);
    maya_execute_program(&maya);

    free(program);

    maya_debug_stack(&maya);
    maya_debug_registers(&maya);
}

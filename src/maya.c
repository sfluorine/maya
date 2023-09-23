#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

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
    case ERR_DIV_BY_ZERO:
        return "DIVIDE BY ZERO";
    default:
        return "UNKNOWN ERROR";
    }
}

static MayaError maya_push_stack(MayaVm* maya, Frame frame) {
    if (maya->sp >= MAYA_STACK_CAP)
        return ERR_STACK_OVERFLOW;

    maya->stack[maya->sp++] = frame;
    return ERR_OK;
}

static MayaError maya_pop_stack(MayaVm* maya, Frame* output) {
    if (maya->sp <= 0)
        return ERR_STACK_UNDERFLOW;

    if (output != NULL)
        *output = maya->stack[maya->sp - 1];
    
    maya->sp--;
    return ERR_OK;
}

static MayaError maya_execute_instruction(MayaVm* maya, MayaInstruction instruction) {
    MayaError error = ERR_OK;
    Frame temps[2];

    switch (instruction.opcode) {
    case OP_HALT:
        maya->halt = true;
        maya->rip++;
        break;
    case OP_PUSH:
        error = maya_push_stack(maya, instruction.operand);
        maya->rip++;
        break;
    case OP_POP:
        error = maya_pop_stack(maya, NULL);
        maya->rip++;
        break;
    case OP_DUP:
        if (maya->sp - instruction.operand.as_i64 < 0)
            return ERR_STACK_UNDERFLOW;

        error = maya_push_stack(maya, maya->stack[maya->sp - instruction.operand.as_i64]);
        maya->rip++;
        break;
    case OP_IADD:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);
        temps[0].as_i64 += temps[1].as_i64;

        maya_push_stack(maya, temps[0]);

        maya->rip++;
        break;
    case OP_FADD:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);
        temps[0].as_f64 += temps[1].as_f64;

        maya_push_stack(maya, temps[0]);

        maya->rip++;
        break;
    case OP_ISUB:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);
        temps[0].as_i64 -= temps[1].as_i64;

        maya_push_stack(maya, temps[0]);

        maya->rip++;
        break;
    case OP_FSUB:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);
        temps[0].as_f64 -= temps[1].as_f64;

        maya_push_stack(maya, temps[0]);

        maya->rip++;
        break;
    case OP_IMUL:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);
        temps[0].as_i64 *= temps[1].as_i64;

        maya_push_stack(maya, temps[0]);

        maya->rip++;
        break;
    case OP_FMUL:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);
        temps[0].as_f64 *= temps[1].as_f64;

        maya_push_stack(maya, temps[0]);

        maya->rip++;
        break;
    case OP_IDIV:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[1].as_i64 == 0)
            return ERR_DIV_BY_ZERO;

        temps[0].as_i64 /= temps[1].as_i64;

        maya_push_stack(maya, temps[0]);

        maya->rip++;
        break;
    case OP_FDIV:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);
        temps[0].as_f64 /= temps[1].as_f64;

        maya_push_stack(maya, temps[0]);

        maya->rip++;
        break;
    case OP_JMP:
        maya->rip = instruction.operand.as_i64;
        break;
    case OP_IJEQ:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0].as_i64 == temps[1].as_i64) {
            maya->rip = instruction.operand.as_i64;
        } else {
            
            maya->rip++;
        }

        break;
    case OP_FJEQ:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0].as_f64 == temps[1].as_f64) {
            maya->rip = instruction.operand.as_i64;
        } else {
            maya->rip++;
        }

        break;
    case OP_IJNEQ:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0].as_i64 != temps[1].as_i64) {
            maya->rip = instruction.operand.as_i64;
        } else {
            
            maya->rip++;
        }

        break;
    case OP_FJNEQ:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0].as_f64 != temps[1].as_f64) {
            maya->rip = instruction.operand.as_i64;
        } else {
            maya->rip++;
        }

        break;
    case OP_IJGT:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0].as_i64 > temps[1].as_i64) {
            maya->rip = instruction.operand.as_i64;
        } else {
            maya->rip++;
        }

        break;
    case OP_FJGT:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0].as_f64 > temps[1].as_f64) {
            maya->rip = instruction.operand.as_i64;
        } else {
            maya->rip++;
        }

        break;
    case OP_IJLT:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0].as_i64 < temps[1].as_i64) {
            maya->rip = instruction.operand.as_i64;
        } else {
            maya->rip++;
        }

        break;
    case OP_FJLT:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0].as_f64 < temps[1].as_f64) {
            maya->rip = instruction.operand.as_i64;
        } else {
            maya->rip++;
        }

        break;
    case OP_CALL:
        maya->registers[MAYA_STACK_POINTER_REG].as_i64 = maya->sp - maya->stack[maya->sp - 1].as_i64;
        maya->registers[MAYA_RETURN_VALUE_REG].as_i64 = maya->rip + 1;
        maya->rip = instruction.operand.as_i64;
        break;
    case OP_RET:
        maya->sp = maya->registers[MAYA_STACK_POINTER_REG].as_i64;
        maya->rip = maya->registers[MAYA_RETURN_VALUE_REG].as_i64;
        break;
    case OP_LOAD:
        if (instruction.operand.as_i64 < 0 || instruction.operand.as_i64 >= MAYA_REGISTERS_CAP)
            return ERR_INVALID_OPERAND;

        error = maya_push_stack(maya, maya->registers[instruction.operand.as_i64]);
        maya->rip++;
        break;
    case OP_STORE:
        if (instruction.operand.as_i64 < 0 || instruction.operand.as_i64 >= MAYA_REGISTERS_CAP)
            return ERR_INVALID_OPERAND;

        error = maya_pop_stack(maya, &maya->registers[instruction.operand.as_i64]);
        maya->rip++;
        break;
    case OP_DEBUG_PRINT_INT:
        if (maya->sp < 1)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[0]);
        printf("%ld\n", temps[0].as_i64);

        maya->rip++;
        break;
    case OP_DEBUG_PRINT_DOUBLE:
        if (maya->sp < 1)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[0]);
        printf("%lf\n", temps[0].as_f64);

        maya->rip++;
        break;
    case OP_DEBUG_PRINT_CHAR:
        if (maya->sp < 1)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[0]);
        printf("%c\n", (char)temps[0].as_i64);

        maya->rip++;
        break;
    default:
        return ERR_INVALID_INSTRUCTION;
    }

    return error;
}

static void maya_execute_program(MayaVm* maya) {
    while (!maya->halt) {
        MayaError error = maya_execute_instruction(maya, maya->program[maya->rip]);
        if (error != ERR_OK) {
            fprintf(stderr, "ERROR: %s\n", maya_error_to_str(error));
            return;
        }
    }
}

static char* shift(int* argc, char*** argv) {
    if (*argc == 0)
        return NULL;

    char* arg = *argv[0];
    *argv += 1;
    *argc -= 1;

    return arg;
}

static void usage(FILE* stream, const char* program_name) {
    fprintf(stream, "usage: %s [options]\n", program_name);
    fprintf(stream, "\n");
    fprintf(stream, "options:\n");
    fprintf(stream, "  -h                                   show usage.\n");
    fprintf(stream, "  -o <output.maya>                     set output file.\n");
    fprintf(stream, "  -a <input.masm>                      assemble mayasm file.\n");
    fprintf(stream, "  -e [<input.maya>...]                 execute maya files.\n");
    // fprintf(stream, "  -d <input.maya>                      disassemble maya file.\n");
}

static const char* get_actual_filename(const char* filepath) {
    const char* start_ptr = filepath;

    while (*filepath)
        filepath++;

    while (*filepath != '/' && filepath != start_ptr)
        filepath--;

    if (*filepath == '/')
        filepath++;

    return filepath;
}

static void maya_load_program_from_file(MayaVm* maya, const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        fprintf(stderr, "ERROR: cannot open file '%s'\n", filepath);
        exit(EXIT_FAILURE);
    }

    MayaHeader header = {0};
    fread(&header, sizeof(MayaHeader), 1, file);

    uint8_t* magic = (uint8_t*)&header.magic;
    if (magic[0] != 'M' || magic[1] != 'Y') {
        fprintf(stderr, "ERROR: invalid header '%.2s'\n", magic);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    maya->rip = header.starting_rip;
    maya->sp = 0;
    memset(maya->registers, 0, sizeof(maya->registers));
    maya->halt = false;

    MayaInstruction* instructions = malloc(sizeof(MayaInstruction) * header.program_length);
    if (!instructions) {
        fprintf(stderr, "ERROR: cannot allocate memory\n");
        exit(EXIT_FAILURE);
    }

    fread(instructions, sizeof(MayaInstruction), header.program_length, file);
    maya->program = instructions;

    fclose(file);
}

int main(int argc, char** argv) {
    maya_translate_asm("./examples/factorial.masm", "./factorial.maya");

    // MayaVm maya;
    // maya_load_program_from_file(&maya, "./factorial.maya");
    // free(maya.program);
}

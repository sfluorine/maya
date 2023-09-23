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
        if (maya->sp - instruction.operand.as_u64 < 0)
            return ERR_STACK_UNDERFLOW;

        error = maya_push_stack(maya, maya->stack[maya->sp - instruction.operand.as_u64]);
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
        maya->rip = instruction.operand.as_u64;
        break;
    case OP_IJEQ:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0].as_i64 == temps[1].as_i64) {
            maya->rip = instruction.operand.as_u64;
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
            maya->rip = instruction.operand.as_u64;
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
            maya->rip = instruction.operand.as_u64;
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
            maya->rip = instruction.operand.as_u64;
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
            maya->rip = instruction.operand.as_u64;
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
            maya->rip = instruction.operand.as_u64;
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
            maya->rip = instruction.operand.as_u64;
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
            maya->rip = instruction.operand.as_u64;
        } else {
            maya->rip++;
        }

        break;
    case OP_CALL:
        maya->registers[MAYA_STACK_POINTER_REG].as_u64 = maya->sp - maya->stack[maya->sp - 1].as_u64;
        maya->registers[MAYA_RETURN_VALUE_REG].as_u64 = maya->rip + 1;
        maya->rip = instruction.operand.as_i64;
        break;
    case OP_RET:
        maya->sp = maya->registers[MAYA_STACK_POINTER_REG].as_u64;
        maya->rip = maya->registers[MAYA_RETURN_VALUE_REG].as_u64;
        break;
    case OP_LOAD:
        if (instruction.operand.as_u64 < 0 || instruction.operand.as_u64 >= MAYA_REGISTERS_CAP)
            return ERR_INVALID_OPERAND;

        error = maya_push_stack(maya, maya->registers[instruction.operand.as_u64]);
        maya->rip++;
        break;
    case OP_STORE:
        if (instruction.operand.as_u64 < 0 || instruction.operand.as_u64 >= MAYA_REGISTERS_CAP)
            return ERR_INVALID_OPERAND;

        error = maya_pop_stack(maya, &maya->registers[instruction.operand.as_u64]);
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
    fprintf(stream, "  -a <input.masm>                      assemble mayasm file.\n");
    fprintf(stream, "  -e <input.maya>                      execute maya file.\n");
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

static const char* maya_instruction_to_str(MayaInstruction instruction) {
    switch (instruction.opcode) {
    case OP_HALT:
        return "halt";
    case OP_PUSH:
        return "push";
    case OP_POP:
        return "pop";
    case OP_DUP:
        return "dup";
    case OP_IADD:
        return "iadd";
    case OP_FADD:
        return "fadd";
    case OP_ISUB:
        return "isub";
    case OP_FSUB:
        return "fsub";
    case OP_IMUL:
        return "imul";
    case OP_FMUL:
        return "fmul";
    case OP_IDIV:
        return "idiv";
    case OP_FDIV:
        return "fdiv";
    case OP_JMP:
        return "jmp";
    case OP_IJEQ:
        return "ijeq";
    case OP_FJEQ:
        return "fjeq";
    case OP_IJNEQ:
        return "ijneq";
    case OP_FJNEQ:
        return "fjneq";
    case OP_IJGT:
        return "ijgt";
    case OP_FJGT:
        return "fjgt";
    case OP_IJLT:
        return "ijlt";
    case OP_FJLT:
        return "fjlt";
    case OP_CALL:
        return "call";
    case OP_RET:
        return "ret";
    case OP_LOAD:
        return "load";
    case OP_STORE:
        return "store";
    case OP_DEBUG_PRINT_INT:
        return "idebug_print";
    case OP_DEBUG_PRINT_DOUBLE:
        return "fdebug_print";
    case OP_DEBUG_PRINT_CHAR:
        return "cdebug_print";
    }
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

    MayaInstruction* instructions = malloc(sizeof(MayaInstruction) * header.program_size);
    if (!instructions) {
        fprintf(stderr, "ERROR: cannot allocate memory\n");
        exit(EXIT_FAILURE);
    }

    fread(instructions, sizeof(MayaInstruction), header.program_size, file);
    maya->program_size = header.program_size;
    maya->program = instructions;

    fclose(file);
}

static void maya_debug(MayaVm* maya) {
    if (!maya->program)
        return;

    while (!maya->halt) {
        printf("rip -> %zu\n", maya->rip);
        printf("sp  -> %zu\n", maya->sp);
        printf("instruction -> %s\n", maya_instruction_to_str(maya->program[maya->rip]));

        maya_execute_instruction(maya, maya->program[maya->rip]);
        getchar();
    }
}

static void maya_disassemble(MayaVm* maya) {
    for (size_t i = 0; i < maya->program_size; i++)
        printf("%s\n", maya_instruction_to_str(maya->program[i]));
}

int main(int argc, char** argv) {
    const char* program = shift(&argc, &argv);

    if (argc == 0) {
        usage(stderr, program);
        exit(EXIT_FAILURE);
    }

    const char* flag = shift(&argc, &argv);

    if (strcmp(flag, "-a") == 0) {
        char output[256];

        const char* input = shift(&argc, &argv);
        if (input == NULL) {
            fprintf(stderr, "ERROR: expected input file\n");
            exit(EXIT_FAILURE);
        }

        const char* actual_input = get_actual_filename(input);
        strcpy(output, actual_input);

        char* output_ptr = output;
        while (*output_ptr && *output_ptr != '.')
            output_ptr++;

        if (*output_ptr == '.')
            *output_ptr = 0;

        strcat(output, ".maya");

        maya_translate_asm(input, output);
    } else if (strcmp(flag, "-e") == 0) {
        const char* input = shift(&argc, &argv);
        if (input == NULL) {
            fprintf(stderr, "ERROR: expected input file\n");
            exit(EXIT_FAILURE);
        }

        MayaVm maya;
        maya_load_program_from_file(&maya, input);
        maya_execute_program(&maya);
        free(maya.program);
    }

    // maya_translate_asm("./examples/fibonacci.masm", "./fibonacci.maya");
    //
    // MayaVm maya;
    // maya_load_program_from_file(&maya, "./fibonacci.maya");
    // maya_execute_program(&maya);
    // maya_debug(&maya);
    // maya_disassemble(&maya);
    // free(maya.program);
}

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
        if (maya->sp - instruction.operand.as_i64 < 0)
            return ERR_STACK_UNDERFLOW;

        error = maya_push_stack(maya, maya->stack[maya->sp - instruction.operand.as_i64]);
        maya->pc++;
        break;
    case OP_IADD:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);
        temps[0].as_i64 += temps[1].as_i64;

        maya_push_stack(maya, temps[0]);

        maya->pc++;
        break;
    case OP_FADD:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);
        temps[0].as_f64 += temps[1].as_f64;

        maya_push_stack(maya, temps[0]);

        maya->pc++;
        break;
    case OP_ISUB:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);
        temps[0].as_i64 -= temps[1].as_i64;

        maya_push_stack(maya, temps[0]);

        maya->pc++;
        break;
    case OP_FSUB:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);
        temps[0].as_f64 -= temps[1].as_f64;

        maya_push_stack(maya, temps[0]);

        maya->pc++;
        break;
    case OP_IMUL:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);
        temps[0].as_i64 *= temps[1].as_i64;

        maya_push_stack(maya, temps[0]);

        maya->pc++;
        break;
    case OP_FMUL:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);
        temps[0].as_f64 *= temps[1].as_f64;

        maya_push_stack(maya, temps[0]);

        maya->pc++;
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

        maya->pc++;
        break;
    case OP_FDIV:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);
        temps[0].as_f64 /= temps[1].as_f64;

        maya_push_stack(maya, temps[0]);

        maya->pc++;
        break;
    case OP_JMP:
        maya->pc = instruction.operand.as_i64;
        break;
    case OP_IJEQ:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0].as_i64 == temps[1].as_i64) {
            maya->pc = instruction.operand.as_i64;
        } else {
            
            maya->pc++;
        }

        break;
    case OP_FJEQ:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0].as_f64 == temps[1].as_f64) {
            maya->pc = instruction.operand.as_i64;
        } else {
            maya->pc++;
        }

        break;
    case OP_IJNEQ:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0].as_i64 != temps[1].as_i64) {
            maya->pc = instruction.operand.as_i64;
        } else {
            
            maya->pc++;
        }

        break;
    case OP_FJNEQ:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0].as_f64 != temps[1].as_f64) {
            maya->pc = instruction.operand.as_i64;
        } else {
            maya->pc++;
        }

        break;
    case OP_IJGT:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0].as_i64 > temps[1].as_i64) {
            maya->pc = instruction.operand.as_i64;
        } else {
            maya->pc++;
        }

        break;
    case OP_FJGT:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0].as_f64 > temps[1].as_f64) {
            maya->pc = instruction.operand.as_i64;
        } else {
            maya->pc++;
        }

        break;
    case OP_IJLT:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0].as_i64 < temps[1].as_i64) {
            maya->pc = instruction.operand.as_i64;
        } else {
            maya->pc++;
        }

        break;
    case OP_FJLT:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[1]);
        maya_pop_stack(maya, &temps[0]);

        if (temps[0].as_f64 < temps[1].as_f64) {
            maya->pc = instruction.operand.as_i64;
        } else {
            maya->pc++;
        }

        break;
    case OP_CALL:
        maya->registers[MAYA_STACK_POINTER_REG].as_i64 = maya->sp - maya->stack[maya->sp - 1].as_i64;
        maya->registers[MAYA_RETURN_VALUE_REG].as_i64 = maya->pc + 1;
        maya->pc = instruction.operand.as_i64;
        break;
    case OP_RET:
        maya->sp = maya->registers[MAYA_STACK_POINTER_REG].as_i64;
        maya->pc = maya->registers[MAYA_RETURN_VALUE_REG].as_i64;
        break;
    case OP_LOAD:
        if (instruction.operand.as_i64 < 0 || instruction.operand.as_i64 >= MAYA_REGISTERS_CAP)
            return ERR_INVALID_OPERAND;

        error = maya_push_stack(maya, maya->registers[instruction.operand.as_i64]);
        maya->pc++;
        break;
    case OP_STORE:
        if (instruction.operand.as_i64 < 0 || instruction.operand.as_i64 >= MAYA_REGISTERS_CAP)
            return ERR_INVALID_OPERAND;

        error = maya_pop_stack(maya, &maya->registers[instruction.operand.as_i64]);
        maya->pc++;
        break;
    case OP_DEBUG_PRINT_INT:
        if (maya->sp < 1)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[0]);
        printf("%ld\n", temps[0].as_i64);

        maya->pc++;
        break;
    case OP_DEBUG_PRINT_DOUBLE:
        if (maya->sp < 1)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[0]);

        printf("%lf\n", temps[0].as_f64);

        maya->pc++;
        break;
    case OP_DEBUG_PRINT_CHAR:
        if (maya->sp < 1)
            return ERR_STACK_UNDERFLOW;

        maya_pop_stack(maya, &temps[0]);
        printf("%c\n", (char)temps[0].as_i64);

        maya->pc++;
        break;
    default:
        return ERR_INVALID_INSTRUCTION;
    }

    return error;
}

static void maya_init(MayaVm* maya, MayaProgram program) {
    maya->pc = program.starting_rip;
    maya->sp = 0;
    maya->program = program.instructions;

    for (size_t i = 0; i < MAYA_REGISTERS_CAP; i++)
        maya->registers[i] = (Frame) {0};

    maya->halt = false;
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

static char* shift(int* argc, char*** argv) {
    if (*argc == 0)
        return NULL;

    char* arg = *argv[0];
    *argv += 1;
    *argc -= 1;

    return arg;
}

#define CLI_STATE_CAPACITY 25

typedef enum CliState_t {
    STATE_ASSEMBLE_FILE,
    STATE_EXECUTE_FILE,
} CliState;

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

static void assemble_file(const char* input_file, const char* output_file) {
    char out_file[256];
    char* out_file_ptr = out_file;

    if (output_file == 0) {
        strcpy(out_file, get_actual_filename(input_file));

        while (*out_file_ptr && *out_file_ptr != '.')
            out_file_ptr++;

        if (*out_file_ptr != 0)
            *out_file_ptr = 0;

        strcat(out_file, ".maya");
    } else {
        strcpy(out_file, output_file);
    }

    FILE* input_stream = fopen(input_file, "r");
    if (!input_stream) {
        fprintf(stderr, "ERROR: cannot open file '%s': %s\n", input_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    fseek(input_stream, 0, SEEK_END);
    long file_size = ftell(input_stream);
    fseek(input_stream, 0, SEEK_SET);

    if (file_size == 0) {
        fprintf(stderr, "ERROR: reading empty file!\n");
        fclose(input_stream);
        exit(EXIT_FAILURE);
    }

    char* buffer = malloc(sizeof(char) * file_size + 1);
    fread(buffer, sizeof(char), (size_t)file_size, input_stream);

    if (ferror(input_stream)) {
        fprintf(stderr, "ERROR: error while reading file: %s\n", strerror(errno));
        free(buffer);
        fclose(input_stream);
        exit(EXIT_FAILURE);
    }

    fclose(input_stream);
    buffer[file_size] = 0;

    MayaProgram program = parse_program(buffer);
    generate_bytecode(out_file, program);

    free(program.instructions);
    free(buffer);
}

static void execute_file(const char* input_file) {
    FILE* input_stream = fopen(input_file, "rb");
    if (!input_stream) {
        fprintf(stderr, "ERROR: cannot open file '%s': %s\n", input_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    size_t starting_rip = 0;
    size_t instructions_len = 0;

    char header[5];
    fread(header, sizeof(char), 5, input_stream);

    if (strcmp(header, "MAYA") != 0) {
        fprintf(stderr, "ERROR: input file was not a maya bytecode\n");
        exit(EXIT_FAILURE);
    }

    fread(&starting_rip, sizeof(size_t), 1, input_stream);
    fread(&instructions_len, sizeof(size_t), 1, input_stream);

    MayaInstruction* instructions = malloc(sizeof(MayaInstruction) * instructions_len);
    if (!instructions){
        fprintf(stderr, "ERROR: cannot allocate memory\n");
        exit(EXIT_FAILURE);
    }

    fread(instructions, sizeof(MayaInstruction), instructions_len, input_stream);
    fclose(input_stream);

    MayaProgram program = {
        .starting_rip = starting_rip,
        .instructions_len = instructions_len,
        .instructions = instructions,
    };

    MayaVm maya;
    maya_init(&maya, program);
    maya_execute_program(&maya);

    free(instructions);
}

int main(int argc, char** argv) {
    const char* program_name = shift(&argc, &argv);

    if (argc == 0) {
        usage(stderr, program_name);
        exit(EXIT_FAILURE);
    }

    const char* assembly_input_file[25];
    size_t aif_ptr = 0;
    size_t aif_len = 0;

    const char* maya_input_file[25];
    size_t mif_ptr = 0;
    size_t mif_len = 0;

    const char* output_file[25];
    size_t of_ptr = 0;
    size_t of_len = 0;

    memset(output_file, 0, sizeof(char*) * 25);

    CliState states[CLI_STATE_CAPACITY];
    size_t states_len = 0;

    while (argc != 0) {
        if (states_len >= CLI_STATE_CAPACITY) {
            fprintf(stderr, "ERROR: too much argument was provided\n");
            return 1;
        }

        const char* option = shift(&argc, &argv);

        if (strcmp(option, "-h") == 0) {
            usage(stdout, program_name);
            exit(EXIT_SUCCESS);
        }

        if (strcmp(option, "-o") == 0) {
            output_file[of_len] = shift(&argc, &argv);

            if (output_file[of_len] == NULL) {
                usage(stderr, program_name);
                fprintf(stderr, "\n");
                fprintf(stderr, "ERROR: no output file was provided!\n");
                exit(EXIT_FAILURE);
            }

            of_len++;

            continue;
        }

        if (strcmp(option, "-a") == 0) {
            assembly_input_file[aif_len] = shift(&argc, &argv);
            if (assembly_input_file[aif_len] == NULL) {
                usage(stderr, program_name);
                fprintf(stderr, "\n");
                fprintf(stderr, "ERROR: no input file was provided!\n");
                exit(EXIT_FAILURE);
            }

            aif_len++;
            states[states_len++] = STATE_ASSEMBLE_FILE;

            continue;
        }

        if (strcmp(option, "-e") == 0) {
            maya_input_file[mif_len] = shift(&argc, &argv);

            while (maya_input_file[mif_len] != NULL) {
                mif_len++;
                states[states_len++] = STATE_EXECUTE_FILE;

                maya_input_file[mif_len] = shift(&argc, &argv);
            }

            if (mif_len == 0) {
                usage(stderr, program_name);
                fprintf(stderr, "\n");
                fprintf(stderr, "ERROR: no input file was provided!\n");
                exit(EXIT_FAILURE);
            }

            continue;
        }

        usage(stderr, program_name);
        fprintf(stderr, "\n");
        fprintf(stderr, "ERROR: invalid argument was provided\n");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < states_len; i++) {
        switch (states[i]) {
        case STATE_ASSEMBLE_FILE:
            assemble_file(assembly_input_file[aif_ptr++], output_file[of_ptr++]);
            break;
        case STATE_EXECUTE_FILE:
            execute_file(maya_input_file[mif_ptr++]);
            break;
        }
    }
}

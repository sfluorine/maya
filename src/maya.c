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

static void maya_init(MayaVm* maya, MayaProgram program) {
    maya->pc = program.starting_rip;
    maya->sp = 0;
    maya->program = program.instructions;

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

static void usage(FILE* stream, const char* program_name) {
    fprintf(stream, "usage: %s [options]\n", program_name);
    fprintf(stream, "\n");
    fprintf(stream, "options:\n");
    fprintf(stream, "  -h                                   show usage.\n");
    fprintf(stream, "  -a <input.masm> [output.maya]        assemble mayasm file.\n");
    fprintf(stream, "  -e <input.maya>                      execute maya file.\n");
    fprintf(stream, "  -d <input.maya>                      disassemble maya file.\n");
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

int main(int argc, char** argv) {
    const char* program_name = shift(&argc, &argv);

    if (argc == 0) {
        usage(stderr, program_name);
        exit(EXIT_FAILURE);
    }

    bool success = false;

    while (argc != 0) {
        const char* option = shift(&argc, &argv);

        if (strcmp(option, "-h") == 0) {
            if (!success) {
                usage(stdout, program_name);
                exit(EXIT_SUCCESS);
            }

            fprintf(stderr, "WARNING: ignoring the -h flag\n");
            continue;
        }

        if (strcmp(option, "-a") == 0) {
            char out_file[256];
            char* out_file_ptr = out_file;

            const char* in_file = shift(&argc, &argv);
            if (in_file == NULL) {
                usage(stderr, program_name);
                fprintf(stderr, "\n");
                fprintf(stderr, "ERROR: no input file was provided!\n");
                exit(EXIT_FAILURE);
            }

            const char* actual_out_file = shift(&argc, &argv);

            if (actual_out_file == NULL) {
                strcpy(out_file, get_actual_filename(in_file));

                while (*out_file_ptr && *out_file_ptr != '.')
                    out_file_ptr++;

                if (*out_file_ptr != 0)
                    *out_file_ptr = 0;

                strcat(out_file, ".maya");
            } else {
                strcpy(out_file, actual_out_file);
            }

            FILE* input_stream = fopen(in_file, "r");
            if (!input_stream) {
                fprintf(stderr, "ERROR: cannot open file '%s': %s\n", in_file, strerror(errno));
                return 1;
            }

            fseek(input_stream, 0, SEEK_END);
            long file_size = ftell(input_stream);
            fseek(input_stream, 0, SEEK_SET);

            if (file_size == 0) {
                fprintf(stderr, "ERROR: reading empty file!\n");
                fclose(input_stream);
                return 1;
            }

            char* buffer = malloc(sizeof(char) * file_size + 1);
            fread(buffer, sizeof(char), (size_t)file_size, input_stream);

            if (ferror(input_stream)) {
                fprintf(stderr, "ERROR: error while reading file: %s\n", strerror(errno));
                free(buffer);
                fclose(input_stream);
                return 1;
            }

            fclose(input_stream);
            buffer[file_size] = 0;

            MayaProgram program = parse_program(buffer);
            generate_bytecode(out_file, program);

            free(program.instructions);
            free(buffer);

            success = true;
            continue;
        }

        if (strcmp(option, "-e") == 0) {
            const char* in_file = shift(&argc, &argv);
            if (in_file == NULL) {
                usage(stderr, program_name);
                fprintf(stderr, "\n");
                fprintf(stderr, "ERROR: no input file was provided!\n");
                exit(EXIT_FAILURE);
            }

            FILE* input_stream = fopen(in_file, "rb");
            if (!input_stream) {
                fprintf(stderr, "ERROR: cannot open file '%s': %s\n", in_file, strerror(errno));
                return 1;
            }

            size_t starting_rip = 0;
            size_t instructions_len = 0;

            fread(&starting_rip, sizeof(size_t), 1, input_stream);
            fread(&instructions_len, sizeof(size_t), 1, input_stream);

            MayaInstruction* instructions = malloc(sizeof(MayaInstruction) * instructions_len);
            if (!instructions){
                fprintf(stderr, "ERROR: cannot allocate memory!\n");
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
            maya_debug_stack(&maya);
            maya_debug_registers(&maya);

            free(instructions);

            success = true;
            continue;
        }

        if (strcmp(option, "-d") == 0) {
            const char* in_file = shift(&argc, &argv);
            if (in_file == NULL) {
                usage(stderr, program_name);
                fprintf(stderr, "\n");
                fprintf(stderr, "ERROR: no input file was provided!\n");
                exit(EXIT_FAILURE);
            }

            FILE* input_stream = fopen(in_file, "rb");
            if (!input_stream) {
                fprintf(stderr, "ERROR: cannot open file '%s': %s\n", in_file, strerror(errno));
                return 1;
            }

            size_t starting_rip = 0;
            size_t instructions_len = 0;

            fread(&starting_rip, sizeof(size_t), 1, input_stream);
            fread(&instructions_len, sizeof(size_t), 1, input_stream);

            MayaInstruction* instructions = malloc(sizeof(MayaInstruction) * instructions_len);
            if (!instructions){
                fprintf(stderr, "ERROR: cannot allocate memory!\n");
                exit(EXIT_FAILURE);
            }

            fread(instructions, sizeof(MayaInstruction), instructions_len, input_stream);
            fclose(input_stream);

            printf("starting rip: %zu\n", starting_rip);
            printf("instructions length: %zu\n", instructions_len);
            printf("\n");

            for (size_t i = 0; i < instructions_len; i++)
                printf("%s\n", maya_instruction_to_str(instructions[i]));

            free(instructions);

            success = true;
            continue;
        }

        usage(stderr, program_name);
        fprintf(stderr, "\n");
        fprintf(stderr, "ERROR: invalid argument was provided!\n");
        exit(EXIT_FAILURE);
    }
}

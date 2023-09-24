#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static MayaError maya_execute_instruction(MayaVm* maya, MayaInstruction instruction) {
    switch (instruction.opcode) {
    case OP_HALT:
        maya->halt = true;
        break;
    case OP_PUSH:
        if (maya->sp >= MAYA_STACK_CAP)
            return ERR_STACK_OVERFLOW;

        maya->stack[maya->sp++] = instruction.operand;
        maya->rip++;
        break;
    case OP_POP:
        if (maya->sp <= 0)
            return ERR_STACK_UNDERFLOW;

        maya->sp--;
        maya->rip++;
        break;
    case OP_DUP:
        if (maya->sp >= MAYA_STACK_CAP)
            return ERR_STACK_OVERFLOW;

        if ((int64_t)maya->sp - instruction.operand.as_i64 < 0)
            return ERR_STACK_UNDERFLOW;

        maya->stack[maya->sp] = maya->stack[maya->sp - instruction.operand.as_u64];
        maya->sp++;
        maya->rip++;
        break;
    case OP_IADD:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya->stack[maya->sp - 2].as_i64 += maya->stack[maya->sp - 1].as_i64;
        maya->sp--;
        maya->rip++;
        break;
    case OP_FADD:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya->stack[maya->sp - 2].as_f64 += maya->stack[maya->sp - 1].as_f64;
        maya->sp--;
        maya->rip++;
        break;
    case OP_ISUB:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya->stack[maya->sp - 2].as_i64 -= maya->stack[maya->sp - 1].as_i64;
        maya->sp--;
        maya->rip++;
        break;
    case OP_FSUB:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya->stack[maya->sp - 2].as_f64 -= maya->stack[maya->sp - 1].as_f64;
        maya->sp--;
        maya->rip++;
        break;
    case OP_IMUL:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya->stack[maya->sp - 2].as_i64 *= maya->stack[maya->sp - 1].as_i64;
        maya->sp--;
        maya->rip++;
        break;
    case OP_FMUL:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya->stack[maya->sp - 2].as_f64 *= maya->stack[maya->sp - 1].as_f64;
        maya->sp--;
        maya->rip++;
        break;
    case OP_IDIV:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        if (maya->stack[maya->sp - 1].as_i64 == 0)
            return ERR_DIV_BY_ZERO;

        maya->stack[maya->sp - 2].as_i64 /= maya->stack[maya->sp - 1].as_i64;
        maya->sp--;
        maya->rip++;
        break;
    case OP_FDIV:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        maya->stack[maya->sp - 2].as_f64 /= maya->stack[maya->sp - 1].as_f64;
        maya->sp--;
        maya->rip++;
        break;
    case OP_JMP:
        maya->rip = instruction.operand.as_u64;
        break;
    case OP_IJEQ:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        if (maya->stack[maya->sp - 2].as_i64 == maya->stack[maya->sp - 1].as_i64) {
            maya->rip = instruction.operand.as_u64;
        } else {
            maya->rip++;
        }
            
        maya->sp -= 2;
        break;
    case OP_FJEQ:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        if (maya->stack[maya->sp - 2].as_f64 == maya->stack[maya->sp - 1].as_f64) {
            maya->rip = instruction.operand.as_u64;
        } else {
            maya->rip++;
        }

        maya->sp -= 2;
        break;
    case OP_IJNEQ:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        if (maya->stack[maya->sp - 2].as_i64 != maya->stack[maya->sp - 1].as_i64) {
            maya->rip = instruction.operand.as_u64;
        } else {
            maya->rip++;
        }
            
        maya->sp -= 2;
        break;
    case OP_FJNEQ:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        if (maya->stack[maya->sp - 2].as_f64 != maya->stack[maya->sp - 1].as_f64) {
            maya->rip = instruction.operand.as_u64;
        } else {
            maya->rip++;
        }

        maya->sp -= 2;
        break;
    case OP_IJGT:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        if (maya->stack[maya->sp - 2].as_i64 > maya->stack[maya->sp - 1].as_i64) {
            maya->rip = instruction.operand.as_u64;
        } else {
            maya->rip++;
        }
            
        maya->sp -= 2;
        break;
    case OP_FJGT:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        if (maya->stack[maya->sp - 2].as_f64 > maya->stack[maya->sp - 1].as_f64) {
            maya->rip = instruction.operand.as_u64;
        } else {
            maya->rip++;
        }

        maya->sp -= 2;
        break;
    case OP_IJLT:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        if (maya->stack[maya->sp - 2].as_i64 < maya->stack[maya->sp - 1].as_i64) {
            maya->rip = instruction.operand.as_u64;
        } else {
            maya->rip++;
        }
            
        maya->sp -= 2;
        break;
    case OP_FJLT:
        if (maya->sp < 2)
            return ERR_STACK_UNDERFLOW;

        if (maya->stack[maya->sp - 2].as_f64 < maya->stack[maya->sp - 1].as_f64) {
            maya->rip = instruction.operand.as_u64;
        } else {
            maya->rip++;
        }

        maya->sp -= 2;
        break;
    case OP_CALL:
        maya->registers[MAYA_RETURN_VALUE_REG].as_u64 = maya->rip + 1;
        maya->registers[MAYA_STACK_POINTER_REG].as_u64 = maya->sp;
        maya->rip = instruction.operand.as_u64;
        break;
    case OP_NATIVE:
        if (maya->sp < 1)
            return ERR_STACK_UNDERFLOW;

        if (instruction.operand.as_u64 >= maya->natives_size)
            return ERR_INVALID_OPERAND;

        {
            MayaError error = maya->natives[instruction.operand.as_u64](maya);
            if (error != ERR_OK)
                return error;
        }
        maya->rip++;
        break;
    case OP_RET:
        maya->sp = maya->registers[MAYA_STACK_POINTER_REG].as_u64;
        maya->rip = maya->registers[MAYA_RETURN_VALUE_REG].as_u64;
        break;
    case OP_LOAD:
        if (maya->sp >= MAYA_STACK_CAP)
            return ERR_STACK_OVERFLOW;

        if (instruction.operand.as_i64 < 0 || instruction.operand.as_u64 >= MAYA_REGISTERS_CAP)
            return ERR_INVALID_OPERAND;

        maya->stack[maya->sp++] = maya->registers[instruction.operand.as_u64];
        maya->rip++;
        break;
    case OP_STORE:
        if (maya->sp < 1)
            return ERR_STACK_UNDERFLOW;

        if (instruction.operand.as_i64 < 0 || instruction.operand.as_u64 >= MAYA_REGISTERS_CAP)
            return ERR_INVALID_OPERAND;

        maya->registers[instruction.operand.as_u64] = maya->stack[maya->sp - 1];
        maya->sp--;
        maya->rip++;
        break;
    default:
        return ERR_INVALID_INSTRUCTION;
    }

    return ERR_OK;
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
    case OP_NATIVE:
        return "native";
    case OP_RET:
        return "ret";
    case OP_LOAD:
        return "load";
    case OP_STORE:
        return "store";
    default:
        return "invalid opcode";
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
    if (strncmp((const char*) magic, "MAYA", 4) != 0) {
        fprintf(stderr, "ERROR: invalid header: '%.4s'\n", magic);
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

static void maya_load_stdlib(MayaVm* maya) {
    maya->natives_size = 0;

    maya->libhandle = dlopen("./libmaya_stdlib.so", RTLD_LOCAL | RTLD_LAZY);
    if (!maya->libhandle) {
        fprintf(stderr, "ERROR: cannot load standard library: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    maya->natives[maya->natives_size++] = dlsym(maya->libhandle, "maya_alloc");
    maya->natives[maya->natives_size++] = dlsym(maya->libhandle, "maya_free");
    maya->natives[maya->natives_size++] = dlsym(maya->libhandle, "maya_print_f64");
    maya->natives[maya->natives_size++] = dlsym(maya->libhandle, "maya_print_i64");
}

static void maya_unload_stdlib(MayaVm* maya) {
    maya->natives_size = 0;
    dlclose(maya->libhandle);
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

    if (strcmp(flag, "-h") == 0) {
        usage(stdout, program);
        exit(EXIT_SUCCESS);
    } else if (strcmp(flag, "-a") == 0) {
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
        maya_load_stdlib(&maya);
        maya_load_program_from_file(&maya, input);
        maya_execute_program(&maya);
        maya_unload_stdlib(&maya);
        free(maya.program);
    } else if (strcmp(flag, "-d") == 0) {
        const char* input = shift(&argc, &argv);
        if (input == NULL) {
            fprintf(stderr, "ERROR: expected input file\n");
            exit(EXIT_FAILURE);
        }

        MayaVm maya;
        maya_load_program_from_file(&maya, input);
        maya_disassemble(&maya);
        free(maya.program);
    } else {
        usage(stderr, program);
        fprintf(stderr, "ERROR: invalid flag: '%s'\n", flag);
        exit(EXIT_FAILURE);
    }
}

#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#include "maya.h"
#include "sv.h"

#define CHECK_EOL(line)                                                                     \
    {                                                                                       \
        StringView* ln = line;                                                              \
                                                                                            \
        if (ln->len != 0) {                                                                 \
            StringView tok = sv_chop_by_delim(ln, " \n");                                   \
            fprintf(stderr, "ERROR: invalid operand: '%.*s'\n", (int)tok.len, tok.str);     \
            exit(EXIT_FAILURE);                                                             \
        }                                                                                   \
    }                                                                                       \

#define STRIP_COMMENT(line)                                                                 \
    {                                                                                       \
        StringView* ln = line;                                                              \
        *ln = sv_strip_by_delim(*ln, " ");                                                  \
                                                                                            \
        if (ln->str[0] == '#')                                                              \
            sv_chop_by_delim(ln, "\n");                                                     \
    }                                                                                       \

#define EXPECT_OPERAND(operand, op)                                                         \
    {                                                                                       \
        if (operand.len == 0) {                                                             \
            fprintf(stderr, "ERROR: %s is expecting an operand\n", op);                     \
            exit(EXIT_FAILURE);                                                             \
        }                                                                                   \
    }                                                                                       \

#define SINGLE_INSTRUCTION(ins)                                                             \
    {                                                                                       \
        instructions[len++] = (MayaInstruction) {                                           \
            .opcode = ins,                                                                  \
        };                                                                                  \
                                                                                            \
        STRIP_COMMENT(&line);                                                               \
        CHECK_EOL(&line);                                                                   \
                                                                                            \
        goto reallocate;                                                                    \
    }                                                                                       \

#define JMPS_INSTRUCTION(ins, op_ins)                                                           \
    {                                                                                           \
        StringView operand = sv_chop_by_delim(&line, " ");                                      \
        EXPECT_OPERAND(operand, op_ins);                                                        \
                                                                                                \
        char type = 0;                                                                          \
        if (check_is_valid_identifier(operand)) {                                               \
            env->deferred_symbol[env->deferred_symbol_size++] = (MayaDeferredSymbol) {          \
                .rip = len,                                                                     \
                .symbol = operand,                                                              \
            };                                                                                  \
                                                                                                \
            instructions[len++] = (MayaInstruction) {                                           \
                .opcode = ins,                                                                  \
            };                                                                                  \
                                                                                                \
            STRIP_COMMENT(&line);                                                               \
            CHECK_EOL(&line);                                                                   \
                                                                                                \
            goto reallocate;                                                                    \
        } else if (check_is_valid_number(operand, &type)) {                                     \
            Frame frame;                                                                        \
                                                                                                \
            if (type == 0 || type == 'U') {                                                     \
                frame.as_u64 = strtoull(operand.str, NULL, 10);                                 \
            } else {                                                                            \
                fprintf(stderr, "ERROR: %s only accept integer values\n", op_ins);              \
                exit(EXIT_FAILURE);                                                             \
            }                                                                                   \
                                                                                                \
            instructions[len++] = (MayaInstruction) {                                           \
                .opcode = ins,                                                                  \
                .operand = frame,                                                               \
            };                                                                                  \
                                                                                                \
            STRIP_COMMENT(&line);                                                               \
            CHECK_EOL(&line);                                                                   \
                                                                                                \
            goto reallocate;                                                                    \
        } else {                                                                                \
            fprintf(stderr, "ERROR: invalid operand: %.*s\n", (int)operand.len, operand.str);   \
            exit(EXIT_FAILURE);                                                                 \
        }                                                                                       \
    }                                                                                           \

static void* xmalloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "ERROR: cannot allocate memory!\n");
        exit(EXIT_FAILURE);
    }

    return ptr;
}

static void* xrealloc(void* old_ptr, size_t size) {
    void* ptr = realloc(old_ptr, size);
    if (!ptr) {
        fprintf(stderr, "ERROR: cannot reallocate memory!\n");
        exit(EXIT_FAILURE);
    }

    return ptr;
}

static bool check_is_valid_identifier(StringView sv) {
    if (isalpha(sv.str[0]) || sv.str[0] == '_') {
        do {
            sv.str++;
            sv.len--;
        } while (sv.len != 0 && (isalnum(sv.str[0]) || sv.str[0] == '_'));

        if (sv.len != 0)
            return false;

        return true;
    }

    return false;
}

static bool check_is_valid_number(StringView sv, char* type) {
    if (isdigit(sv.str[0])) {
        do {
            sv.str++;
            sv.len--;
        } while (sv.len != 0 && isdigit(sv.str[0]));

        if (sv.len == 0)
            return true;

        if (sv.str[0] == '.') {
            sv.str++;
            sv.len--;

            size_t mantissa_size = 0;
            while (sv.len != 0 && isdigit(sv.str[0])) {
                mantissa_size++;
                sv.str++;
                sv.len--;
            }

            if (mantissa_size == 0)
                return false;

            if (sv.len == 0 || (sv.len == 1 && sv.str[0] == 'F')) {
                *type = 'F';
                return true;
            }

            return false;
        }

        switch (sv.str[0]) {
        case 'U':
            sv.str++;
            sv.len--;

            if (sv.len == 0) {
                *type = 'U';
                return true;
            }

            return false;
        case 'F':
            sv.str++;
            sv.len--;

            if (sv.len == 0) {
                *type = 'F';
                return true;
            }

            return false;
        default:
            return false;
        }
    }

    return false;
}

static bool check_is_valid_string(StringView sv) {
    if (sv.str[0] == '"') {
        sv.str++;
        sv.len--;

        while (sv.len != 1 && sv.str[0] != '"') {
            sv.str++;
            sv.len--;
        }

        if (sv.str[0] != '"')
            return false;

        return true;
    }

    return false;
}

void maya_translate_asm(MayaEnv* env, const char* buffer, const char* output_path) {
    size_t len = 0;
    size_t cap = 1;
    MayaInstruction* instructions = xmalloc(sizeof(MayaInstruction) * cap);

    StringView entry = {.str = NULL, .len = 0};

    StringView str = sv_from_cstr(buffer);
    while (str.len != 0) {
        str = sv_strip_by_delim(str, " \n");

        if (str.len == 0)
            break;

        if (str.str[0] == '#') {
            sv_chop_by_delim(&str, "\n");
            continue;
        }

        StringView line = sv_chop_by_delim(&str, "\n");
        while (line.len != 0) {
            StringView opcode = sv_chop_by_delim(&line, " ");

            if (opcode.str[0] == '%' && sv_equals((StringView) {.str = opcode.str + 1, .len = opcode.len - 1}, sv_from_cstr("define"))) {
                StringView id = sv_chop_by_delim(&line, " ");
                EXPECT_OPERAND(id, "define");

                if (!check_is_valid_identifier(id)) {
                    fprintf(stderr, "ERROR: expected name for macro\n");
                    exit(EXIT_FAILURE);
                }

                StringView operand = sv_chop_by_delim(&line, " ");
                EXPECT_OPERAND(operand, "define");

                char type = 0;
                Frame frame;

                if (check_is_valid_number(operand, &type)) {
                    switch (type) {
                    case 'U':
                        frame.as_u64 = strtoull(operand.str, NULL, 10);
                        break;
                    case 'F':
                        frame.as_f64 = strtod(operand.str, NULL);
                        break;
                    default:
                        frame.as_i64 = strtoll(operand.str, NULL, 10);
                        break;
                    }
                } else {
                    fprintf(stderr, "ERROR: invalid operand: '%.*s'\n", (int)operand.len, operand.str);
                    exit(EXIT_FAILURE);
                }

                env->macros[env->macros_size++] = (MayaMacro) {
                    .name = id,
                    .frame = frame,
                };

                STRIP_COMMENT(&line);
                CHECK_EOL(&line);

                continue;
            }

            // handle label
            if (check_is_valid_identifier((StringView) {.str = opcode.str, .len = opcode.len - 1}) && opcode.str[opcode.len - 1] == ':') {
                env->labels[env->labels_size++] = (MayaLabel) {
                    .id = (StringView) {
                        .str = opcode.str,
                        .len = opcode.len - 1,
                    },
                    .rip = len,
                };

                STRIP_COMMENT(&line);
                CHECK_EOL(&line);

                continue;
            }

            if (sv_equals(opcode, sv_from_cstr("entry"))) {
                StringView operand = sv_chop_by_delim(&line, " ");
                EXPECT_OPERAND(operand, "entry");

                if (check_is_valid_identifier(operand)) {
                    entry = operand;

                    STRIP_COMMENT(&line);
                    CHECK_EOL(&line);

                    continue;
                }

                fprintf(stderr, "ERROR: invalid operand: '%.*s'\n", (int)operand.len, operand.str);
                exit(EXIT_FAILURE);
            }

            if (sv_equals(opcode, sv_from_cstr("halt")))
                SINGLE_INSTRUCTION(OP_HALT);

            if (sv_equals(opcode, sv_from_cstr("push"))) {
                line = sv_strip_by_delim(line, " ");
                if (check_is_valid_string(line)) {
                    StringView string_literal = sv_chop_by_string_literal(&line);

                    env->str_literals[env->str_literals_size++] = (MayaStringLiteral) {
                        .literal = string_literal,
                        .rip = len,
                    };

                    instructions[len++] = (MayaInstruction) {
                        .opcode = OP_PUSH,
                    };

                    STRIP_COMMENT(&line);
                    CHECK_EOL(&line);

                    goto reallocate;
                }

                StringView operand = sv_chop_by_delim(&line, " ");
                EXPECT_OPERAND(operand, "push");

                char type = 0;
                if (check_is_valid_number(operand, &type)) {
                    Frame frame;

                    switch (type) {
                    case 'U':
                        frame.as_u64 = strtoull(operand.str, NULL, 10);
                        break;
                    case 'F':
                        frame.as_f64 = strtod(operand.str, NULL);
                        break;
                    default:
                        frame.as_i64 = strtoll(operand.str, NULL, 10);
                        break;
                    }

                    instructions[len++] = (MayaInstruction) {
                        .opcode = OP_PUSH,
                        .operand = frame,
                    };

                    STRIP_COMMENT(&line);
                    CHECK_EOL(&line);

                    goto reallocate;
                } else if (check_is_valid_identifier(operand)) {
                    env->deferred_symbol[env->deferred_symbol_size++] = (MayaDeferredSymbol) {
                        .rip = len,
                        .symbol = operand,
                    };

                    instructions[len++] = (MayaInstruction) {
                        .opcode = OP_PUSH,
                    };

                    STRIP_COMMENT(&line);
                    CHECK_EOL(&line);

                    goto reallocate;
                } else {
                    fprintf(stderr, "ERROR: invalid operand: '%.*s'\n", (int)operand.len, operand.str);
                    exit(EXIT_FAILURE);
                }
            }

            if (sv_equals(opcode, sv_from_cstr("pop")))
                SINGLE_INSTRUCTION(OP_POP);

            if (sv_equals(opcode, sv_from_cstr("dup"))) {
                StringView operand = sv_chop_by_delim(&line, " ");
                EXPECT_OPERAND(operand, "dup");

                char type = 0;
                if (check_is_valid_number(operand, &type)) {
                    Frame frame;

                    if (type == 0 || type == 'U') {
                        frame.as_u64 = strtoull(operand.str, NULL, 10);
                    } else {
                        fprintf(stderr, "ERROR: dup only accept integer values\n");
                        exit(EXIT_FAILURE);
                    }

                    instructions[len++] = (MayaInstruction) {
                        .opcode = OP_DUP,
                        .operand = frame,
                    };

                    STRIP_COMMENT(&line);
                    CHECK_EOL(&line);

                    goto reallocate;
                } else {
                    fprintf(stderr, "ERROR: invalid operand: '%.*s'\n", (int)operand.len, operand.str);
                    exit(EXIT_FAILURE);
                }
            }

            if (sv_equals(opcode, sv_from_cstr("iadd")))
                SINGLE_INSTRUCTION(OP_IADD);

            if (sv_equals(opcode, sv_from_cstr("fadd")))
                SINGLE_INSTRUCTION(OP_FADD);

            if (sv_equals(opcode, sv_from_cstr("isub")))
                SINGLE_INSTRUCTION(OP_ISUB);

            if (sv_equals(opcode, sv_from_cstr("fsub")))
                SINGLE_INSTRUCTION(OP_FSUB);

            if (sv_equals(opcode, sv_from_cstr("imul")))
                SINGLE_INSTRUCTION(OP_IMUL);

            if (sv_equals(opcode, sv_from_cstr("fmul")))
                SINGLE_INSTRUCTION(OP_FMUL);

            if (sv_equals(opcode, sv_from_cstr("idiv")))
                SINGLE_INSTRUCTION(OP_IDIV);

            if (sv_equals(opcode, sv_from_cstr("fdiv")))
                SINGLE_INSTRUCTION(OP_FDIV);

            if (sv_equals(opcode, sv_from_cstr("jmp")))
                JMPS_INSTRUCTION(OP_JMP, "jmp");

            if (sv_equals(opcode, sv_from_cstr("ijeq")))
                JMPS_INSTRUCTION(OP_IJEQ, "ijmp");

            if (sv_equals(opcode, sv_from_cstr("fjeq")))
                JMPS_INSTRUCTION(OP_FJEQ, "fjeq");

            if (sv_equals(opcode, sv_from_cstr("ijneq")))
                JMPS_INSTRUCTION(OP_IJNEQ, "ijneq");

            if (sv_equals(opcode, sv_from_cstr("fjneq")))
                JMPS_INSTRUCTION(OP_FJNEQ, "fjneq");

            if (sv_equals(opcode, sv_from_cstr("ijgt")))
                JMPS_INSTRUCTION(OP_IJGT, "ijgt");

            if (sv_equals(opcode, sv_from_cstr("fjgt")))
                JMPS_INSTRUCTION(OP_FJGT, "fjgt");

            if (sv_equals(opcode, sv_from_cstr("ijlt")))
                JMPS_INSTRUCTION(OP_IJLT, "ijlt");

            if (sv_equals(opcode, sv_from_cstr("fjlt")))
                JMPS_INSTRUCTION(OP_FJLT, "fjlt");

            if (sv_equals(opcode, sv_from_cstr("call"))) {
                StringView operand = sv_chop_by_delim(&line, " ");
                EXPECT_OPERAND(operand, "call");

                if (check_is_valid_identifier(operand)) {
                    env->deferred_symbol[env->deferred_symbol_size++] = (MayaDeferredSymbol) {
                        .rip = len,
                        .symbol = operand,
                    };

                    instructions[len++] = (MayaInstruction) {
                        .opcode = OP_CALL,
                    };

                    STRIP_COMMENT(&line);
                    CHECK_EOL(&line);

                    goto reallocate;
                }

                fprintf(stderr, "ERROR: invalid operand: '%.*s'\n", (int)operand.len, operand.str);
                exit(EXIT_FAILURE);
            }

            if (sv_equals(opcode, sv_from_cstr("native"))) {
                StringView operand = sv_chop_by_delim(&line, " ");
                EXPECT_OPERAND(operand, "native");

                char type = 0;
                if (check_is_valid_number(operand, &type)) {
                    Frame frame;

                    if (type == 0 || type == 'U') {
                        frame.as_u64 = strtoull(operand.str, NULL, 10);
                    } else {
                        fprintf(stderr, "ERROR: native only accept integer values\n");
                        exit(EXIT_FAILURE);
                    }

                    instructions[len++] = (MayaInstruction) {
                        .opcode = OP_NATIVE,
                        .operand = frame,
                    };

                    STRIP_COMMENT(&line);
                    CHECK_EOL(&line);

                    goto reallocate;
                } else if (check_is_valid_identifier(operand)) {
                    env->deferred_symbol[env->deferred_symbol_size++] = (MayaDeferredSymbol) {
                        .rip = len,
                        .symbol = operand,
                    };

                    instructions[len++] = (MayaInstruction) {
                        .opcode = OP_NATIVE,
                    };

                    STRIP_COMMENT(&line);
                    CHECK_EOL(&line);

                    goto reallocate;
                } else {
                    fprintf(stderr, "ERROR: invalid operand: '%.*s'\n", (int)operand.len, operand.str);
                    exit(EXIT_FAILURE);
                }
            }

            if (sv_equals(opcode, sv_from_cstr("ret")))
                SINGLE_INSTRUCTION(OP_RET);

            if (sv_equals(opcode, sv_from_cstr("load"))) {
                StringView operand = sv_chop_by_delim(&line, " ");
                EXPECT_OPERAND(operand, "load");

                char type = 0;
                if (check_is_valid_number(operand, &type)) {
                    Frame frame;

                    if (type == 0 || type == 'U') {
                        frame.as_u64 = strtoull(operand.str, NULL, 10);
                    } else {
                        fprintf(stderr, "ERROR: load only accept integer values\n");
                        exit(EXIT_FAILURE);
                    }

                    instructions[len++] = (MayaInstruction) {
                        .opcode = OP_LOAD,
                        .operand = frame,
                    };

                    STRIP_COMMENT(&line);
                    CHECK_EOL(&line);

                    goto reallocate;
                } else {
                    fprintf(stderr, "ERROR: invalid operand: '%.*s'\n", (int)operand.len, operand.str);
                    exit(EXIT_FAILURE);
                }
            }

            if (sv_equals(opcode, sv_from_cstr("store"))) {
                StringView operand = sv_chop_by_delim(&line, " ");
                EXPECT_OPERAND(operand, "store");

                char type = 0;
                if (check_is_valid_number(operand, &type)) {
                    Frame frame;

                    if (type == 0 || type == 'U') {
                        frame.as_u64 = strtoull(operand.str, NULL, 10);
                    } else {
                        fprintf(stderr, "ERROR: store only accept integer values\n");
                        exit(EXIT_FAILURE);
                    }

                    instructions[len++] = (MayaInstruction) {
                        .opcode = OP_STORE,
                        .operand = frame,
                    };

                    STRIP_COMMENT(&line);
                    CHECK_EOL(&line);

                    goto reallocate;
                } else {
                    fprintf(stderr, "ERROR: invalid operand: '%.*s'\n", (int)operand.len, operand.str);
                    exit(EXIT_FAILURE);
                }
            }

            fprintf(stderr, "ERROR: invalid opcode: '%.*s'\n", (int)opcode.len, opcode.str);
            exit(EXIT_FAILURE);
        }

    reallocate:
        cap++;
        instructions = xrealloc(instructions, sizeof(MayaInstruction) * cap);
    }

    MayaHeader header = {0};

    uint8_t* magic = (uint8_t*)&header.magic;
    memcpy(magic, "MAYA", 4);
    header.program_size = len;

    if (entry.str != NULL && entry.len != 0) {
        bool found = false;
        for (size_t i = 0; i < env->labels_size; i++) {
            if (sv_equals(entry, env->labels[i].id)) {
                header.starting_rip = env->labels[i].rip;
                found = true;
                break;
            }
        }

        if (!found) {
            fprintf(stderr, "ERROR: no such label for the entry point: '%.*s'\n", (int)entry.len, entry.str);
            exit(EXIT_FAILURE);
        }
    }

    FILE* ostream = fopen(output_path, "wb");
    if (!ostream) {
        fprintf(stderr, "ERROR: cannot open file '%s'\n", output_path);
        exit(EXIT_FAILURE);
    }

    fwrite(&header, sizeof(MayaHeader), 1, ostream);
    fwrite(instructions, sizeof(MayaInstruction), len, ostream);
    
    for (size_t i = 0; i < env->str_literals_size; i++) {
        StringView literal = env->str_literals[i].literal;
        fwrite(literal.str, sizeof(uint8_t), literal.len, ostream);
        uint8_t null = 0;
        fwrite(&null, sizeof(uint8_t), 1, ostream);
        fwrite(&env->str_literals[i].rip, sizeof(size_t), 1, ostream);
    }

    fclose(ostream);
    free(instructions);

    size_t bigger = 0;
    size_t lower = 0;
    char type = 0;
    if (env->labels_size >= env->macros_size) {
        type = 'L';
        bigger = env->labels_size;
        lower = env->macros_size;
    } else {
        bigger = env->macros_size;
        lower = env->labels_size;
    }

    for (size_t i = 0; i < bigger; i++) {
        for (size_t j = 0; j < lower; j++) {
            if (type == 'L') {
                if (sv_equals(env->labels[i].id, env->macros[j].name)) {
                    fprintf(stderr, "ERROR: duplicate label and macro name '%.*s'\n", (int)env->macros[i].name.len, env->macros[i].name.str);
                    exit(EXIT_FAILURE);
                }
            } else {
                if (sv_equals(env->labels[j].id, env->macros[i].name)) {
                    fprintf(stderr, "ERROR: duplicate label and macro name '%.*s'\n", (int)env->macros[i].name.len, env->macros[i].name.str);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
}

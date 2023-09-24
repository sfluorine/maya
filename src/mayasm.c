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
            m_deferred[m_deferred_size++] = (DeferredSymbol) {                                  \
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

#define LABELS_MAX_CAP 100

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

typedef struct Label_t {
    StringView label;
    size_t rip;
} Label;

static Label m_labels[LABELS_MAX_CAP];
static size_t m_labels_size = 0;

typedef struct DeferredSymbol_t {
    size_t rip;
    StringView symbol;
} DeferredSymbol;

static DeferredSymbol m_deferred[LABELS_MAX_CAP];
static size_t m_deferred_size = 0;

typedef struct NativeLabel_t {
    StringView label;
    StringView native_fun_name;
    size_t index;
} NativeLabel;

static NativeLabel m_natives[LABELS_MAX_CAP];
static size_t m_natives_size = 0;

typedef struct DeferredNativeSymbol_t {
    size_t rip;
    StringView symbol;
} DeferredNativeSymbol;

static DeferredNativeSymbol m_deferred_natives[LABELS_MAX_CAP];
static size_t m_deferred_natives_size = 0;

static void resolve_deferred_symbols(MayaInstruction* instructions) {
    for (size_t i = 0; i < m_deferred_size; i++) {
        bool found = false;
        for (size_t j = 0; j < m_labels_size; j++) {
            if (sv_equals(m_deferred[i].symbol, m_labels[j].label)) {
                instructions[m_deferred[i].rip].operand.as_u64 = m_labels[j].rip;
                found = true;
                break;
            } 
        }

        if (!found) {
            StringView symbol = m_deferred[i].symbol;
            fprintf(stderr, "ERROR: no such label: '%.*s'\n", (int)symbol.len, symbol.str);
            exit(EXIT_FAILURE);
        }
    }
}

static void resolve_deferred_natives_symbols(MayaInstruction* instructions, MayaVm* maya) {
    for (size_t i = 0; i < m_natives_size; i++) {
        char name[256];
        StringView native_fun_name = m_natives[i].native_fun_name;

        strncpy(name, native_fun_name.str, native_fun_name.len);
        name[native_fun_name.len] = 0;

        MayaNative native = dlsym(maya->libhandle, name);
        if (!native) {
            fprintf(stderr, "ERROR: no such native function: '%s'\n", name);
            exit(EXIT_FAILURE);
        }

        for (size_t j = 0; j < maya->natives_size; j++) {
            if (native == maya->natives[j]) {
                m_natives[i].index = j;
                break;
            }
        }
    }

    for (size_t i = 0; i < m_deferred_natives_size; i++) {
        bool found = false;
        for (size_t j = 0; j < m_natives_size; j++) {
            if (sv_equals(m_deferred_natives[i].symbol, m_natives[j].label)) {
                instructions[m_deferred_natives[i].rip].operand.as_u64 = m_natives[j].index;
                found = true;
                break;
            } 
        }

        if (!found) {
            StringView symbol = m_deferred_natives[i].symbol;
            fprintf(stderr, "ERROR: no such native function: '%.*s'\n", (int)symbol.len, symbol.str);
            exit(EXIT_FAILURE);
        }
    }
}

static bool check_is_regular_file(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

void maya_translate_asm(const char* input_path, const char* output_path, MayaVm* maya) {
    m_labels_size = 0;

    FILE* istream = fopen(input_path, "r");
    if (!istream) {
        fprintf(stderr, "ERROR: cannot open file '%s'\n", input_path);
        exit(EXIT_FAILURE);
    }

    if (!check_is_regular_file(input_path)) {
        fprintf(stderr, "ERROR: '%s' is not a file!\n", input_path);
        exit(EXIT_FAILURE);
    }

    fseek(istream, 0, SEEK_END);
    long size = ftell(istream);
    fseek(istream, 0, SEEK_SET);

    if (size == 0) {
        fprintf(stderr, "ERROR: reading empty file!\n");
        exit(EXIT_FAILURE);
    }

    char* buffer = malloc(sizeof(char) * size + 1);
    fread(buffer, sizeof(char), size, istream);
    buffer[size] = 0;

    fclose(istream);

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

            // handle label
            if (check_is_valid_identifier((StringView) {.str = opcode.str, .len = opcode.len - 1}) && opcode.str[opcode.len - 1] == ':') {
                m_labels[m_labels_size++] = (Label) {
                    .label = (StringView) {
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

            if (sv_equals(opcode, sv_from_cstr("extern"))) {
                StringView operand = sv_chop_by_delim(&line, " ");
                EXPECT_OPERAND(operand, "extern");

                if (check_is_valid_identifier(operand)) {
                    line = sv_strip_by_delim(line, " ");
                    if (check_is_valid_string(line)) {
                        StringView actual_function_name = sv_chop_by_string_literal(&line);

                        m_natives[m_natives_size++] = (NativeLabel) {
                            .label = operand,
                            .native_fun_name = actual_function_name,
                        };

                        STRIP_COMMENT(&line);
                        CHECK_EOL(&line);

                        continue;
                    }

                    fprintf(stderr, "ERROR: expected a native function name\n");
                    exit(EXIT_FAILURE);
                }

                fprintf(stderr, "ERROR: invalid operand: '%.*s'\n", (int)operand.len, operand.str);
                exit(EXIT_FAILURE);
            }

            if (sv_equals(opcode, sv_from_cstr("halt")))
                SINGLE_INSTRUCTION(OP_HALT);

            if (sv_equals(opcode, sv_from_cstr("push"))) {
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
                    m_deferred[m_deferred_size++] = (DeferredSymbol) {
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
                    m_deferred_natives[m_deferred_natives_size++] = (DeferredNativeSymbol) {
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
    magic[0] = 'M';
    magic[1] = 'A';
    magic[2] = 'Y';
    magic[3] = 'A';

    header.program_size = len;

    if (entry.str != NULL && entry.len != 0) {
        bool found = false;
        for (size_t i = 0; i < m_labels_size; i++) {
            if (sv_equals(entry, m_labels[i].label)) {
                header.starting_rip = m_labels[i].rip;
                found = true;
                break;
            }
        }

        if (!found) {
            fprintf(stderr, "ERROR: no such label for the entry point: '%.*s'\n", (int)entry.len, entry.str);
            exit(EXIT_FAILURE);
        }
    }

    resolve_deferred_natives_symbols(instructions, maya);
    resolve_deferred_symbols(instructions);

    FILE* ostream = fopen(output_path, "wb");
    if (!ostream) {
        fprintf(stderr, "ERROR: cannot open file '%s'\n", output_path);
        exit(EXIT_FAILURE);
    }

    fwrite(&header, sizeof(MayaHeader), 1, ostream);
    fwrite(instructions, sizeof(MayaInstruction), len, ostream);

    fclose(ostream);
    free(instructions);
    free(buffer);
}

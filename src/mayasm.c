#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "maya.h"

typedef enum TokenType_t {
    TOK_HALT,
    TOK_PUSH,
    TOK_POP,
    TOK_DUP,
    TOK_ADD,
    TOK_SUB,
    TOK_MUL,
    TOK_DIV,
    TOK_JMP,
    TOK_JEQ,
    TOK_JNEQ,
    TOK_JGT,
    TOK_JLT,
    TOK_CALL,
    TOK_RET,
    TOK_LOAD,
    TOK_STORE,
    TOK_IDENT,
    TOK_LABEL,
    TOK_STRING,
    TOK_INTLIT,
} TokenType;

typedef struct Token_t {
    TokenType type;
    const char* start;
    size_t len;
} Token;

typedef struct SymbolTable_t {
    const char* id;
    size_t len;
    size_t rip;
} SymbolTable;

static void print_str(FILE* stream, const char* str, size_t len) {
    for (size_t i = 0; i < len; i++)
        fprintf(stream, "%c", str[i]);
}

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

static Token* get_tokens(const char* source, size_t* length) {
    size_t cap = 1;
    Token* tokens = xmalloc(sizeof(Token) * cap);

    while (*source) {
        // skip whitespaces.
        while (*source && isspace(*source))
            source++;

        // skip comments.
        if (*source == '#') {
            while (*source && *source != '\n')
                source++;
        }

        // skip whitespaces, again.
        while (*source && isspace(*source))
            source++;

        const char* start = source;

        if (*source == 0)
            return tokens;

        if (isalpha(*source)) {
            size_t len = 0;
            do {
                len++;
                source++;
            } while (*source && isalnum(*source));

            if (*source == ':') {
                source++;

                tokens[*length] = (Token) {
                    .type = TOK_LABEL,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "halt", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_HALT,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "push", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_PUSH,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "pop", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_POP,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "dup", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_DUP,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "add", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_ADD,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "sub", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_SUB,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "mul", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_MUL,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "div", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_DIV,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "jmp", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_JMP,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "jeq", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_JEQ,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "jneq", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_JNEQ,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "jgt", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_JGT,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "jlt", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_JLT,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "call", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_CALL,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "ret", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_RET,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "load", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_LOAD,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "store", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_STORE,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            tokens[*length] = (Token) {
                .type = TOK_IDENT,
                .start = start,
                .len = len,
            };

            *length += 1;

            goto reallocate;
        }

        if (isdigit(*source)) {
            size_t len = 0;
            do {
                len++;
                source++;
            } while (*source && isdigit(*source));

            tokens[*length] = (Token) {
                .type = TOK_INTLIT,
                .start = start,
                .len = len,
            };

            *length += 1;

            goto reallocate;
        }

        fprintf(stderr, "ERROR: unknown token '%c'\n", *source);
        exit(EXIT_FAILURE);

    reallocate:
        cap++;
        tokens = xrealloc(tokens, sizeof(Token) * cap);
    }

    return tokens;
}

static SymbolTable m_symtable[100];
static size_t m_symtable_len = 0;

static void symtable_insert(const char* id, size_t len, size_t rip) {
    if (m_symtable_len >= 100) {
        fprintf(stderr, "ERROR: symbol table overflow!\n");
        exit(EXIT_FAILURE);
    }

    m_symtable[m_symtable_len++] = (SymbolTable) {
        .id = id,
        .len = len,
        .rip = rip,
    };
}

static int symtable_search(const char* id, size_t len) {
    for (int i = 0; i < m_symtable_len; i++) {
        if (strncmp(m_symtable[i].id, id, len) == 0)
            return i;
    }

    return -1;
}

static MayaInstruction* parse_instructions(const char* source, size_t* out_len) {
    size_t tokens_cursor = 0;
    size_t tokens_length = 0;
    Token* tokens = get_tokens(source, &tokens_length);

    size_t rip = 0;

    size_t cap = 1;
    size_t len = 0;
    MayaInstruction* instructions = xmalloc(sizeof(MayaInstruction) * cap);

    while (tokens_cursor < tokens_length) {
        Token current_token = tokens[tokens_cursor];

        if (current_token.type == TOK_LABEL) {
            if (symtable_search(current_token.start, current_token.len) != -1) {
                fprintf(stderr, "ERROR: label '");
                print_str(stderr, current_token.start, current_token.len);
                fprintf(stderr, "' already exists!\n");
                exit(EXIT_FAILURE);
            }

            symtable_insert(current_token.start, current_token.len, rip);
            tokens_cursor++;
            continue;
        }

        if (current_token.type == TOK_HALT) {
            instructions[len] = (MayaInstruction) {
                .opcode = OP_HALT,
            };

            tokens_cursor++;
            rip++;

            goto reallocate;
        }

        if (current_token.type == TOK_PUSH) {
            tokens_cursor++;

            if (tokens[tokens_cursor].type == TOK_INTLIT) {
                instructions[len] = (MayaInstruction) {
                    .opcode = OP_PUSH,
                    .operand = strtoll(tokens[tokens_cursor].start, NULL, 10),
                };

                tokens_cursor++;
                rip++;

                goto reallocate;
            } 

            if (tokens[tokens_cursor].type == TOK_STRING) {
            }

            fprintf(stderr, "ERROR: unexpected operand: '");
            print_str(stderr, tokens[tokens_cursor].start, tokens[tokens_cursor].len);
            fprintf(stderr, "'\n");
            exit(EXIT_FAILURE);
        }

        if (current_token.type == TOK_POP) {
            instructions[len] = (MayaInstruction) {
                .opcode = OP_POP,
            };

            tokens_cursor++;
            rip++;

            goto reallocate;
        }

        if (current_token.type == TOK_DUP) {
            tokens_cursor++;

            if (tokens[tokens_cursor].type == TOK_INTLIT) {
                instructions[len] = (MayaInstruction) {
                    .opcode = OP_DUP,
                    .operand = strtoll(tokens[tokens_cursor].start, NULL, 10),
                };

                tokens_cursor++;
                rip++;

                goto reallocate;
            } 

            fprintf(stderr, "ERROR: unexpected operand: '");
            print_str(stderr, tokens[tokens_cursor].start, tokens[tokens_cursor].len);
            fprintf(stderr, "'\n");
            exit(EXIT_FAILURE);
        }

        if (current_token.type == TOK_ADD) {
            instructions[len] = (MayaInstruction) {
                .opcode = OP_ADD,
            };

            tokens_cursor++;
            rip++;

            goto reallocate;
        }

        if (current_token.type == TOK_SUB) {
            instructions[len] = (MayaInstruction) {
                .opcode = OP_SUB,
            };

            tokens_cursor++;
            rip++;

            goto reallocate;
        }

        if (current_token.type == TOK_MUL) {
            instructions[len] = (MayaInstruction) {
                .opcode = OP_MUL,
            };

            tokens_cursor++;
            rip++;

            goto reallocate;
        }

        if (current_token.type == TOK_DIV) {
            instructions[len] = (MayaInstruction) {
                .opcode = OP_DIV,
            };

            tokens_cursor++;
            rip++;

            goto reallocate;
        }

        if (current_token.type == TOK_JMP) {
            tokens_cursor++;

            if (tokens[tokens_cursor].type == TOK_INTLIT) {
                instructions[len] = (MayaInstruction) {
                    .opcode = OP_JMP,
                    .operand = strtoll(tokens[tokens_cursor].start, NULL, 10),
                };

                tokens_cursor++;
                rip++;

                goto reallocate;
            } 

            if (tokens[tokens_cursor].type == TOK_IDENT) {
                int index = symtable_search(tokens[tokens_cursor].start, tokens[tokens_cursor].len);
                if (index == -1) {
                    fprintf(stderr, "ERROR: no such label '");
                    print_str(stderr, tokens[tokens_cursor].start, tokens[tokens_cursor].len);
                    fprintf(stderr, "'\n");
                    exit(EXIT_FAILURE);
                }

                instructions[len] = (MayaInstruction) {
                    .opcode = OP_JMP,
                    .operand = m_symtable[index].rip,
                };

                tokens_cursor++;
                rip++;

                goto reallocate;
            }

            fprintf(stderr, "ERROR: unexpected operand: '");
            print_str(stderr, tokens[tokens_cursor].start, tokens[tokens_cursor].len);
            fprintf(stderr, "'\n");
            exit(EXIT_FAILURE);
        }

        if (current_token.type == TOK_JEQ) {
            tokens_cursor++;

            if (tokens[tokens_cursor].type == TOK_INTLIT) {
                instructions[len] = (MayaInstruction) {
                    .opcode = OP_JEQ,
                    .operand = strtoll(tokens[tokens_cursor].start, NULL, 10),
                };

                tokens_cursor++;
                rip++;

                goto reallocate;
            } 

            if (tokens[tokens_cursor].type == TOK_IDENT) {
                int index = symtable_search(tokens[tokens_cursor].start, tokens[tokens_cursor].len);
                if (index == -1) {
                    fprintf(stderr, "ERROR: no such label '");
                    print_str(stderr, tokens[tokens_cursor].start, tokens[tokens_cursor].len);
                    fprintf(stderr, "'\n");
                    exit(EXIT_FAILURE);
                }

                instructions[len] = (MayaInstruction) {
                    .opcode = OP_JEQ,
                    .operand = m_symtable[index].rip,
                };

                tokens_cursor++;
                rip++;

                goto reallocate;
            }

            fprintf(stderr, "ERROR: unexpected operand: '");
            print_str(stderr, tokens[tokens_cursor].start, tokens[tokens_cursor].len);
            fprintf(stderr, "'\n");
            exit(EXIT_FAILURE);
        }

        if (current_token.type == TOK_JNEQ) {
            tokens_cursor++;

            if (tokens[tokens_cursor].type == TOK_INTLIT) {
                instructions[len] = (MayaInstruction) {
                    .opcode = OP_JNEQ,
                    .operand = strtoll(tokens[tokens_cursor].start, NULL, 10),
                };

                tokens_cursor++;
                rip++;

                goto reallocate;
            } 

            if (tokens[tokens_cursor].type == TOK_IDENT) {
                int index = symtable_search(tokens[tokens_cursor].start, tokens[tokens_cursor].len);
                if (index == -1) {
                    fprintf(stderr, "ERROR: no such label '");
                    print_str(stderr, tokens[tokens_cursor].start, tokens[tokens_cursor].len);
                    fprintf(stderr, "'\n");
                    exit(EXIT_FAILURE);
                }

                instructions[len] = (MayaInstruction) {
                    .opcode = OP_JNEQ,
                    .operand = m_symtable[index].rip,
                };

                tokens_cursor++;
                rip++;

                goto reallocate;
            }

            fprintf(stderr, "ERROR: unexpected operand: '");
            print_str(stderr, tokens[tokens_cursor].start, tokens[tokens_cursor].len);
            fprintf(stderr, "'\n");
            exit(EXIT_FAILURE);
        }

        if (current_token.type == TOK_JGT) {
            tokens_cursor++;

            if (tokens[tokens_cursor].type == TOK_INTLIT) {
                instructions[len] = (MayaInstruction) {
                    .opcode = OP_JGT,
                    .operand = strtoll(tokens[tokens_cursor].start, NULL, 10),
                };

                tokens_cursor++;
                rip++;

                goto reallocate;
            } 

            if (tokens[tokens_cursor].type == TOK_IDENT) {
                int index = symtable_search(tokens[tokens_cursor].start, tokens[tokens_cursor].len);
                if (index == -1) {
                    fprintf(stderr, "ERROR: no such label '");
                    print_str(stderr, tokens[tokens_cursor].start, tokens[tokens_cursor].len);
                    fprintf(stderr, "'\n");
                    exit(EXIT_FAILURE);
                }

                instructions[len] = (MayaInstruction) {
                    .opcode = OP_JGT,
                    .operand = m_symtable[index].rip,
                };

                tokens_cursor++;
                rip++;

                goto reallocate;
            }

            fprintf(stderr, "ERROR: unexpected operand: '");
            print_str(stderr, tokens[tokens_cursor].start, tokens[tokens_cursor].len);
            fprintf(stderr, "'\n");
            exit(EXIT_FAILURE);
        }

        if (current_token.type == TOK_JLT) {
            tokens_cursor++;

            if (tokens[tokens_cursor].type == TOK_INTLIT) {
                instructions[len] = (MayaInstruction) {
                    .opcode = OP_JLT,
                    .operand = strtoll(tokens[tokens_cursor].start, NULL, 10),
                };

                tokens_cursor++;
                rip++;

                goto reallocate;
            } 

            if (tokens[tokens_cursor].type == TOK_IDENT) {
                int index = symtable_search(tokens[tokens_cursor].start, tokens[tokens_cursor].len);
                if (index == -1) {
                    fprintf(stderr, "ERROR: no such label '");
                    print_str(stderr, tokens[tokens_cursor].start, tokens[tokens_cursor].len);
                    fprintf(stderr, "'\n");
                    exit(EXIT_FAILURE);
                }

                instructions[len] = (MayaInstruction) {
                    .opcode = OP_JLT,
                    .operand = m_symtable[index].rip,
                };

                tokens_cursor++;
                rip++;

                goto reallocate;
            }

            fprintf(stderr, "ERROR: unexpected operand: '");
            print_str(stderr, tokens[tokens_cursor].start, tokens[tokens_cursor].len);
            fprintf(stderr, "'\n");
            exit(EXIT_FAILURE);
        }

        if (current_token.type == TOK_CALL) {
            tokens_cursor++;

            if (tokens[tokens_cursor].type == TOK_IDENT) {
                int index = symtable_search(tokens[tokens_cursor].start, tokens[tokens_cursor].len);
                if (index == -1) {
                    fprintf(stderr, "ERROR: no such label '");
                    print_str(stderr, tokens[tokens_cursor].start, tokens[tokens_cursor].len);
                    fprintf(stderr, "'\n");
                    exit(EXIT_FAILURE);
                }

                instructions[len] = (MayaInstruction) {
                    .opcode = OP_CALL,
                    .operand = m_symtable[index].rip,
                };

                tokens_cursor++;
                rip++;

                goto reallocate;
            }

            fprintf(stderr, "ERROR: unexpected operand: '");
            print_str(stderr, tokens[tokens_cursor].start, tokens[tokens_cursor].len);
            fprintf(stderr, "'\n");
            exit(EXIT_FAILURE);
        }

        if (current_token.type == TOK_RET) {
            instructions[len] = (MayaInstruction) {
                .opcode = OP_RET,
            };

            tokens_cursor++;
            rip++;

            goto reallocate;
        }

        if (current_token.type == TOK_LOAD) {
            tokens_cursor++;

            if (tokens[tokens_cursor].type == TOK_INTLIT) {
                instructions[len] = (MayaInstruction) {
                    .opcode = OP_LOAD,
                    .operand = strtoll(tokens[tokens_cursor].start, NULL, 10),
                };

                tokens_cursor++;
                rip++;

                goto reallocate;
            } 

            fprintf(stderr, "ERROR: unexpected operand: '");
            print_str(stderr, tokens[tokens_cursor].start, tokens[tokens_cursor].len);
            fprintf(stderr, "'\n");
            exit(EXIT_FAILURE);
        }

        if (current_token.type == TOK_STORE) {
            tokens_cursor++;

            if (tokens[tokens_cursor].type == TOK_INTLIT) {
                instructions[len] = (MayaInstruction) {
                    .opcode = OP_STORE,
                    .operand = strtoll(tokens[tokens_cursor].start, NULL, 10),
                };

                tokens_cursor++;
                rip++;

                goto reallocate;
            } 

            fprintf(stderr, "ERROR: unexpected operand: '");
            print_str(stderr, tokens[tokens_cursor].start, tokens[tokens_cursor].len);
            fprintf(stderr, "'\n");
            exit(EXIT_FAILURE);
        }

        fprintf(stderr, "ERROR: invalid opcode '");
        print_str(stderr, current_token.start, current_token.len);
        fprintf(stderr, "'\n");
        exit(EXIT_FAILURE);

    reallocate:
        cap++;
        len++;
        instructions = xrealloc(instructions, sizeof(MayaInstruction) * cap);
    }

    *out_len = len;

    free(tokens);
    return instructions;
}

static void generate_bytecode(const char* out_file, MayaInstruction* instructions, size_t len) {
    FILE* out_stream = fopen(out_file, "wb");
    if (!out_stream) {
        fprintf(stderr, "ERROR: cannot open file '%s'\n", out_file);
        exit(EXIT_FAILURE);
    }

    fwrite(instructions, sizeof(MayaInstruction), len, out_stream);
    fclose(out_stream);
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
    fprintf(stderr, "Usage: %s <input_file.masm> <out_file.maya> \n", program);
}

int main(int argc, char** argv) {
    const char* program = shift(&argc, &argv);

    if (argc == 0) {
        usage(program);
        fprintf(stderr, "ERROR: expected input file!\n");
        return 1;
    }

    const char* input_file = shift(&argc, &argv);

    if (argc == 0) {
        usage(program);
        fprintf(stderr, "ERROR: expected output file!\n");
        return 1;
    }

    const char* output_file = shift(&argc, &argv);

    FILE* input_stream = fopen(input_file, "r");
    if (!input_stream) {
        fprintf(stderr, "ERROR: cannot open file '%s': %s\n", input_file, strerror(errno));
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

    size_t instructions_len = 0;
    MayaInstruction* instructions = parse_instructions(buffer, &instructions_len);

    generate_bytecode(output_file, instructions, instructions_len);

    free(instructions);
    free(buffer);
}

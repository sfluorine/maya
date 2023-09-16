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
    TOK_IADD,
    TOK_FADD,
    TOK_ISUB,
    TOK_FSUB,
    TOK_IMUL,
    TOK_FMUL,
    TOK_IDIV,
    TOK_FDIV,
    TOK_JMP,
    TOK_JEQ,
    TOK_JNEQ,
    TOK_JGT,
    TOK_JLT,
    TOK_CALL,
    TOK_RET,
    TOK_LOAD,
    TOK_STORE,
    TOK_DEBUG_PRINT_INT,
    TOK_DEBUG_PRINT_DOUBLE,
    TOK_DEBUG_PRINT_CHAR,
    TOK_IDENT,
    TOK_LABEL,
    TOK_INTLIT,
    TOK_STRINGLIT,
    TOK_DOUBLELIT,
    TOK_ENTRY,
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
        fprintf(stderr, "ERROR: cannot allocate memory\n");
        exit(EXIT_FAILURE);
    }

    return ptr;
}

static void* xrealloc(void* old_ptr, size_t size) {
    void* ptr = realloc(old_ptr, size);
    if (!ptr) {
        fprintf(stderr, "ERROR: cannot reallocate memory\n");
        exit(EXIT_FAILURE);
    }

    return ptr;
}

static Token* get_tokens(const char* source, size_t* length) {
    size_t cap = 1;
    Token* tokens = xmalloc(sizeof(Token) * cap);

    while (*source) {
        while (*source && isspace(*source) || *source == '#') {
            // skip whitespaces.
            while (*source && isspace(*source))
                source++;

            // skip comments.
            if (*source == '#') {
                while (*source && *source != '\n')
                    source++;
            }
        }

        const char* start = source;

        if (*source == 0)
            return tokens;

        if (isalpha(*source) || *source == '_') {
            size_t len = 0;
            do {
                len++;
                source++;
            } while (*source && (isalnum(*source) || *source == '_'));

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

            if (strncmp(start, "iadd", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_IADD,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "fadd", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_FADD,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "isub", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_ISUB,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "fsub", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_FSUB,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "imul", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_IMUL,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "fmul", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_FMUL,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "idiv", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_IDIV,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "fdiv", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_FDIV,
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

            if (strncmp(start, "idebug_print", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_DEBUG_PRINT_INT,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "fdebug_print", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_DEBUG_PRINT_DOUBLE,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "cdebug_print", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_DEBUG_PRINT_CHAR,
                    .start = start,
                    .len = len,
                };

                *length += 1;

                goto reallocate;
            }

            if (strncmp(start, "entry", len) == 0) {
                tokens[*length] = (Token) {
                    .type = TOK_ENTRY,
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
            bool is_float = false;
            size_t len = 0;
            size_t mantissa_len = 0;

            do {
                len++;
                source++;
            } while (*source && isdigit(*source));

            if (*source == '.') {
                is_float = true;
                len++;
                source++;

                while (*source && isdigit(*source)) {
                    mantissa_len++;
                    source++;
                }

                if (mantissa_len == 0) {
                    fprintf(stderr, "ERROR: invalid floating point number\n");
                    exit(EXIT_FAILURE);
                }
            }

            if (is_float) {
                tokens[*length] = (Token) {
                    .type = TOK_DOUBLELIT,
                    .start = start,
                    .len = len,
                };
            } else {
                tokens[*length] = (Token) {
                    .type = TOK_INTLIT,
                    .start = start,
                    .len = len,
                };
            }

            *length += 1;

            goto reallocate;
        }

        if (*source == '"') {
            source++;
            start++;

            size_t len = 0;
            while (*source && *source != '"') {
                len++;
                source++;
            }

            if (*source == 0) {
                fprintf(stderr, "ERROR: unterminated string literal\n");
                exit(EXIT_FAILURE);
            }

            source++;

            tokens[*length] = (Token) {
                .type = TOK_STRINGLIT,
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
        fprintf(stderr, "ERROR: symbol table overflow\n");
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

MayaProgram parse_program(const char* source) {
    size_t tokens_cursor = 0;
    size_t tokens_length = 0;
    Token* tokens = get_tokens(source, &tokens_length);

    size_t rip = 0;

    size_t cap = 1;
    size_t len = 0;
    MayaInstruction* instructions = xmalloc(sizeof(MayaInstruction) * cap);

    const char* entry = NULL;
    size_t entry_len = 0;
    size_t starting_rip = 0;

    while (tokens_cursor < tokens_length) {
        Token current_token = tokens[tokens_cursor];

        if (current_token.type == TOK_LABEL) {
            if (symtable_search(current_token.start, current_token.len) != -1) {
                fprintf(stderr, "ERROR: label '");
                print_str(stderr, current_token.start, current_token.len);
                fprintf(stderr, "' already exists\n");
                exit(EXIT_FAILURE);
            }

            symtable_insert(current_token.start, current_token.len, rip);
            tokens_cursor++;
            continue;
        }

        if (current_token.type == TOK_ENTRY) {
            tokens_cursor++;

            if (tokens[tokens_cursor].type == TOK_IDENT) {
                entry = tokens[tokens_cursor].start;
                entry_len = tokens[tokens_cursor].len;

                tokens_cursor++;
                continue;
            }

            fprintf(stderr, "ERROR: unexpected operand: '");
            print_str(stderr, tokens[tokens_cursor].start, tokens[tokens_cursor].len);
            fprintf(stderr, "'\n");
            exit(EXIT_FAILURE);
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

            if (tokens[tokens_cursor].type == TOK_DOUBLELIT) {
                instructions[len].opcode = OP_PUSH;
                double x = atof(tokens[tokens_cursor].start);
                memcpy(&instructions[len].operand, &x, 8);

                tokens_cursor++;
                rip++;

                goto reallocate;
            }

            if (tokens[tokens_cursor].type == TOK_STRINGLIT) {
                for (size_t i = 0; i < tokens[tokens_cursor].len; i++) {
                    instructions[len] = (MayaInstruction) {
                        .opcode = OP_PUSH,
                        .operand = tokens[tokens_cursor].start[i],
                    };

                    cap++;
                    len++;
                    instructions = xrealloc(instructions, sizeof(MayaInstruction) * cap);
                    rip++;
                }

                instructions[len] = (MayaInstruction) {
                    .opcode = OP_PUSH,
                    .operand = 0,
                };

                cap++;
                len++;
                instructions = xrealloc(instructions, sizeof(MayaInstruction) * cap);

                tokens_cursor++;
                rip++;

                continue;
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

        if (current_token.type == TOK_IADD) {
            instructions[len] = (MayaInstruction) {
                .opcode = OP_IADD,
            };

            tokens_cursor++;
            rip++;

            goto reallocate;
        }

        if (current_token.type == TOK_FADD) {
            instructions[len] = (MayaInstruction) {
                .opcode = OP_FADD,
            };

            tokens_cursor++;
            rip++;

            goto reallocate;
        }

        if (current_token.type == TOK_ISUB) {
            instructions[len] = (MayaInstruction) {
                .opcode = OP_ISUB,
            };

            tokens_cursor++;
            rip++;

            goto reallocate;
        }

        if (current_token.type == TOK_FSUB) {
            instructions[len] = (MayaInstruction) {
                .opcode = OP_FSUB,
            };

            tokens_cursor++;
            rip++;

            goto reallocate;
        }

        if (current_token.type == TOK_IMUL) {
            instructions[len] = (MayaInstruction) {
                .opcode = OP_IMUL,
            };

            tokens_cursor++;
            rip++;

            goto reallocate;
        }

        if (current_token.type == TOK_FMUL) {
            instructions[len] = (MayaInstruction) {
                .opcode = OP_FMUL,
            };

            tokens_cursor++;
            rip++;

            goto reallocate;
        }

        if (current_token.type == TOK_IDIV) {
            instructions[len] = (MayaInstruction) {
                .opcode = OP_IDIV,
            };

            tokens_cursor++;
            rip++;

            goto reallocate;
        }

        if (current_token.type == TOK_FDIV) {
            instructions[len] = (MayaInstruction) {
                .opcode = OP_FDIV,
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

        if (current_token.type == TOK_DEBUG_PRINT_INT) {
            instructions[len] = (MayaInstruction) {
                .opcode = OP_DEBUG_PRINT_INT,
            };

            tokens_cursor++;
            rip++;

            goto reallocate;
        }

        if (current_token.type == TOK_DEBUG_PRINT_DOUBLE) {
            instructions[len] = (MayaInstruction) {
                .opcode = OP_DEBUG_PRINT_DOUBLE,
            };

            tokens_cursor++;
            rip++;

            goto reallocate;
        }

        if (current_token.type == TOK_DEBUG_PRINT_CHAR) {
            instructions[len] = (MayaInstruction) {
                .opcode = OP_DEBUG_PRINT_CHAR,
            };

            tokens_cursor++;
            rip++;

            goto reallocate;
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

    if (entry != NULL) {
        int index = symtable_search(entry, entry_len);
        if (index == -1) {
            fprintf(stderr, "ERROR: no such label '");
            print_str(stderr, entry, entry_len);
            fprintf(stderr, "' to set the entry\n");
            exit(EXIT_FAILURE);
        }

        starting_rip = m_symtable[index].rip;
    }

    free(tokens);
    return (MayaProgram) {
        .instructions_len = len,
        .starting_rip = starting_rip,
        .instructions = instructions,
    };
}

void generate_bytecode(const char* out_file, MayaProgram program) {
    FILE* out_stream = fopen(out_file, "wb");
    if (!out_stream) {
        fprintf(stderr, "ERROR: cannot open file '%s'\n", out_file);
        exit(EXIT_FAILURE);
    }

    fwrite("MAYA", sizeof(char), 5, out_stream);
    fwrite(&program.starting_rip, sizeof(size_t), 1, out_stream);
    fwrite(&program.instructions_len, sizeof(size_t), 1, out_stream);
    fwrite(program.instructions, sizeof(MayaInstruction), program.instructions_len, out_stream);
    fclose(out_stream);
}

static char instruction_to_str[256];

const char* maya_instruction_to_str(MayaInstruction instruction) {
    double actual_x = (*(double*)&instruction.operand);

    switch (instruction.opcode) {
    case OP_HALT:
        return "halt";
    case OP_PUSH:
        sprintf(instruction_to_str, "push %lf", actual_x);
        break;
    case OP_POP:
        return "pop";
    case OP_DUP:
        sprintf(instruction_to_str, "dup %ld", instruction.operand);
        break;
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
        sprintf(instruction_to_str, "jmp %ld", instruction.operand);
        break;
    case OP_JEQ:
        sprintf(instruction_to_str, "jeq %ld", instruction.operand);
        break;
    case OP_JNEQ:
        sprintf(instruction_to_str, "jneq %ld", instruction.operand);
        break;
    case OP_JGT:
        sprintf(instruction_to_str, "jgt %ld", instruction.operand);
        break;
    case OP_JLT:
        sprintf(instruction_to_str, "jlt %ld", instruction.operand);
        break;
    case OP_CALL:
        sprintf(instruction_to_str, "call %ld", instruction.operand);
        break;
    case OP_RET:
        return "ret";
    case OP_LOAD:
        sprintf(instruction_to_str, "load %ld", instruction.operand);
        break;
    case OP_STORE:
        sprintf(instruction_to_str, "store %ld", instruction.operand);
        break;
    case OP_DEBUG_PRINT_INT:
        return "idebug_print";
    case OP_DEBUG_PRINT_DOUBLE:
        return "fdebug_print";
    case OP_DEBUG_PRINT_CHAR:
        return "cdebug_print";
    default:
        break;
    }

    return instruction_to_str;
}

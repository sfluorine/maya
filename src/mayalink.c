#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "maya.h"

static void* xmalloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "ERROR: cannot allocate memory!\n");
        exit(EXIT_FAILURE);
    }

    return ptr;
}

void maya_link_program(MayaEnv* env, const char* input_path) {
    FILE* file = fopen(input_path, "rb");
    if (!file) {
        fprintf(stderr, "ERROR: cannot open file '%s'\n", input_path);
        exit(EXIT_FAILURE);
    }

    MayaHeader header = {0};
    fread(&header, sizeof(MayaHeader), 1, file);

    MayaInstruction* instructions = xmalloc(sizeof(MayaInstruction) * header.program_size);
    fread(instructions, sizeof(MayaInstruction), header.program_size, file);

    long literals_start = ftell(file);

    fseek(file, 0, SEEK_END);
    long whole_file_size = ftell(file);
    fseek(file, literals_start, SEEK_SET);

    long literals_size = whole_file_size - literals_start;
    uint8_t* literals = xmalloc(sizeof(uint8_t) * literals_size);
    fread(literals, sizeof(char), literals_size, file);

    fclose(file);

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

    // check for duplicate labels in both labels and macros
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

    // resolve deferred label
    for (size_t i = 0; i < env->deferred_symbol_size; i++) {
        StringView symbol = env->deferred_symbol[i].symbol;
        bool found = false;

        for (size_t j = 0; j < env->labels_size; j++) {
            if (sv_equals(symbol, env->labels[j].id)) {
                instructions[env->deferred_symbol[i].rip].operands[0].as_u64 = env->labels[j].rip;
                found = true;
                break;
            }
        }

        if (found)
            continue;

        for (size_t j = 0; j < env->macros_size; j++) {
            if (sv_equals(symbol, env->macros[j].name)) {
                instructions[env->deferred_symbol[i].rip].operands[0] = env->macros[j].frame;
                found = true;
                break;
            }
        }

        if (!found) {
            fprintf(stderr, "ERROR: no such label '%.*s'\n", (int)symbol.len, symbol.str);
            exit(EXIT_FAILURE);
        }
    }

    file = fopen(input_path, "wb");
    if (!file) {
        fprintf(stderr, "ERROR: cannot open file '%s'\n", input_path);
        exit(EXIT_FAILURE);
    }

    fwrite(&header, sizeof(MayaHeader), 1, file);
    fwrite(instructions, sizeof(MayaInstruction), header.program_size, file);
    fwrite(literals, sizeof(uint8_t), literals_size, file);

    fclose(file);

    free(literals);
    free(instructions);
}

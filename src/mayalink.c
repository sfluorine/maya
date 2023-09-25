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

    // resolve deferred label
    for (size_t i = 0; i < env->deferred_symbol_size; i++) {
        StringView symbol = env->deferred_symbol[i].symbol;
        bool found = false;

        for (size_t j = 0; j < env->labels_size; j++) {
            if (sv_equals(env->deferred_symbol[i].symbol, env->labels[j].id)) {
                instructions[env->deferred_symbol[i].rip].operand.as_u64 = env->labels[j].rip;
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

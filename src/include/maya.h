#pragma once

#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "sv.h"

#define MAYA_STACK_CAP 1024
#define MAYA_NATIVES_CAP 1024
#define MAYA_REGISTERS_CAP 7
#define MAYA_STACK_POINTER_REG 5
#define MAYA_RETURN_VALUE_REG 6

typedef enum MayaError_t {
    ERR_OK,
    ERR_STACK_OVERFLOW,
    ERR_STACK_UNDERFLOW,
    ERR_INVALID_OPERAND,
    ERR_INVALID_INSTRUCTION,
    ERR_DIV_BY_ZERO,
} MayaError;

typedef enum MayaOpCode_t {
    OP_HALT,
    OP_PUSH,
    OP_POP,
    OP_DUP,
    OP_IADD,
    OP_FADD,
    OP_ISUB,
    OP_FSUB,
    OP_IMUL,
    OP_FMUL,
    OP_IDIV,
    OP_FDIV,
    OP_JMP,
    OP_IJEQ,
    OP_FJEQ,
    OP_IJNEQ,
    OP_FJNEQ,
    OP_IJGT,
    OP_FJGT,
    OP_IJLT,
    OP_FJLT,
    OP_CALL,
    OP_NATIVE,
    OP_RET,
    OP_LOAD,
    OP_STORE,
} MayaOpCode;

typedef union Frame_t {
    int64_t as_i64;
    uint64_t as_u64;
    double as_f64;
    void* as_ptr;
} Frame;

static_assert(sizeof(Frame) == 8, "Maya's frame size is expected to be 64 bit.");

typedef struct MayaInstruction_t {
    MayaOpCode opcode;
    Frame operand;
} MayaInstruction;

typedef struct MayaVm_t MayaVm;

typedef MayaError (*MayaNative)(MayaVm*);

struct MayaVm_t {
    MayaInstruction* program;
    size_t rip;
    size_t program_size;

    Frame stack[MAYA_STACK_CAP];
    size_t sp; // stack pointer
    Frame registers[MAYA_REGISTERS_CAP];

    MayaNative natives[MAYA_NATIVES_CAP];
    size_t natives_size;

    char* literals;
    size_t literals_size;

    void* stdlib_handle;

    bool halt;
};

typedef struct MayaHeader_t {
    uint32_t magic;
    size_t starting_rip;
    size_t program_size;
} MayaHeader;

typedef struct MayaLabel_t {
    size_t rip;
    StringView id;
} MayaLabel;

typedef struct MayaDeferredSymbol_t {
    size_t rip;
    StringView symbol;
} MayaDeferredSymbol;

typedef struct MayaStringLiteral_t {
    size_t rip;
    StringView literal;
} MayaStringLiteral;

typedef struct MayaEnv_t {
    MayaLabel labels[100];
    size_t labels_size;

    MayaDeferredSymbol deferred_symbol[100];
    size_t deferred_symbol_size;

    MayaStringLiteral str_literals[100];
    size_t str_literals_size;
} MayaEnv;

void maya_translate_asm(MayaEnv* env, const char* input_path, const char* output_path);
void maya_link_program(MayaEnv* env, const char* input_path);

#pragma once

#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MAYA_STACK_CAP 1024
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
    OP_RET,
    OP_LOAD,
    OP_STORE,
    OP_DEBUG_PRINT_INT,
    OP_DEBUG_PRINT_DOUBLE,
    OP_DEBUG_PRINT_CHAR,
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

typedef struct MayaVm_t {
    size_t rip; // program counter
    size_t sp; // stack pointer
    MayaInstruction* program;
    Frame stack[MAYA_STACK_CAP];
    Frame registers[MAYA_REGISTERS_CAP];
    bool halt;
} MayaVm;

typedef struct MayaHeader_t {
    uint16_t magic;
    size_t starting_rip;
    size_t program_length;
} MayaHeader;

void maya_translate_asm(const char* input_path, const char* output_path);

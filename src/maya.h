#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MAYA_STACK_CAP 1024
#define MAYA_REGISTERS_CAP 5

typedef enum MayaError_t {
    ERR_OK,
    ERR_STACK_OVERFLOW,
    ERR_STACK_UNDERFLOW,
    ERR_INVALID_OPERAND,
    ERR_INVALID_INSTRUCTION,
} MayaError;

typedef enum MayaOpCode_t {
    OP_HALT,
    OP_PUSH,
    OP_POP,
    OP_DUP,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_JMP,
    OP_JEQ,
    OP_JNEQ,
    OP_JGT,
    OP_JLT,
    OP_CALL,
    OP_RET,
    OP_LOAD,
    OP_STORE,
} MayaOpCode;

typedef struct MayaInstruction_t {
    MayaOpCode opcode;
    int64_t operand;
} MayaInstruction;

typedef struct MayaVm_t {
    size_t pc; // program counter
    size_t sp; // stack pointer
    MayaInstruction* program;
    int64_t stack[MAYA_STACK_CAP];
    int64_t registers[MAYA_REGISTERS_CAP];
    bool halt;
} MayaVm;
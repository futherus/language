#ifndef ENCODE_H
#define ENCODE_H

#include <stddef.h>
#include <stdint.h>

#include "buffer.h"

typedef enum
{
    OP_NOTYPE    = 0,
    OP_REGISTER  = 1,
    OP_MEMORY    = 2,
    OP_IMM32     = 3,
} Operand_type;

struct Register
{
    uint8_t id;
};

struct Memory
{
    uint8_t scale;
    uint8_t index;
    uint8_t base;

    int32_t disp;

    bool is_index;
    bool is_base;
    bool is_disp;
};

struct Operand
{
    Operand_type type;

    union
    {
        Memory   mem;
        Register reg;
        int32_t  imm32;
    };
};

enum Mnemonic
{
    ADD = 1,
    MOV,
    LEA,
    SUB,
    CMP,
    JMP,
    JE,
    JNE,
    PUSH,
    POP,
    RET,
    CALL,
};

struct Instruction
{
    Mnemonic mnemonic;
    Operand  op1;
    Operand  op2;
};

Operand IMM8(int8_t val);
Operand IMM32(int32_t val);
Operand MEM(uint8_t scale, Operand index, Operand base, int32_t disp);

void encode(Buffer* buffer, Instruction instr);

extern const Operand RAX;
extern const Operand RCX;
extern const Operand RDX;
extern const Operand RBX;
extern const Operand RSP;
extern const Operand RBP;
extern const Operand RSI;
extern const Operand RDI;

#endif // ENCODE_H

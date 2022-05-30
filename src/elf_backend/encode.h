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

#define DEF_INSTR(ENUM, FUNC)   \
    ENUM,                       \

enum Mnemonic
{
    INVALID_INSTR = 0,
    
    #include "encode.inc"
};

#undef DEF_INSTR

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

extern const Operand R8;
extern const Operand R9;
extern const Operand R10;
extern const Operand R11;
extern const Operand R12;
extern const Operand R13;
extern const Operand R14;
extern const Operand R15;

#endif // ENCODE_H

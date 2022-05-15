#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "encode.h"

Operand IMM(int32_t val)
{
    Operand op = {};
    op.type = OP_IMMEDIATE;
    op.imm.val = val;
    return op;
}

Operand MEM(uint8_t scale, Operand index, Operand base, int32_t disp)
{
    Operand op = {.type = OP_MEMORY};

    assert(index.type == OP_REGISTER || index.type == OP_NOTYPE);
    assert(base.type  == OP_REGISTER || base.type  == OP_NOTYPE);

    if(index.type == OP_REGISTER)
    {
        op.mem.index    = index.reg.id;
        op.mem.is_index = true;

        switch(scale)
        {
            case 1:
                op.mem.scale = 0b00;
                break;
            case 2:
                op.mem.scale = 0b01;
                break;
            case 4:
                op.mem.scale = 0b10;
                break;
            case 8:
                op.mem.scale = 0b11;
                break;
            default:
                assert(0 && "Unsupported register scaling");
        }
    }

    if(base.type == OP_REGISTER)
    {
        op.mem.base    = base.reg.id;
        op.mem.is_base = true;
    }

    // FIXME: if no register addressing and displacement==0 displacement shouldn't be dropped
    if(disp)
    {
        op.mem.disp    = disp;
        op.mem.is_disp = true;
    }

    return op;
}

const Operand RAX = {
    .type = OP_REGISTER,
    .reg = (Register) {.id = 0b0000}
};
const Operand RCX = {
    .type = OP_REGISTER,
    .reg = (Register) {.id = 0b0001}
};
const Operand RDX = {
    .type = OP_REGISTER,
    .reg = (Register) {.id = 0b0010}
};
const Operand RBX = {
    .type = OP_REGISTER,
    .reg = (Register) {.id = 0b0011}
};
const Operand RSP = {
    .type = OP_REGISTER,
    .reg = (Register) {.id = 0b0100}
};
const Operand RBP = {
    .type = OP_REGISTER,
    .reg = (Register) {.id = 0b0101}
};
const Operand RSI = {
    .type = OP_REGISTER,
    .reg = (Register) {.id = 0b0110}
};
const Operand RDI = {
    .type = OP_REGISTER,
    .reg = (Register) {.id = 0b0111}
};

#define GEN_PREFIX(PREFIX)  \
    (PREFIX)                \

#define GEN_OPCODE(OP, DIR_EXT, SZ)            \
    ( (OP) | ((uint8_t) ((DIR_EXT & 0b1u) << 1u)) | ((uint8_t) (SZ & 0b1u)) ) \

#define GEN_MODRM(MOD, REG_OP, RM)                             \
    ( ((uint8_t) ((MOD & 0b11u) << 6u)) | ((uint8_t) ((REG_OP & 0b111u) << 3u)) | (RM & 0b111u) ) \

#define GEN_SIB(SCALE, INDEX, BASE)                                 \
    ( ((uint8_t) ((SCALE & 0b11u) << 6u)) | ((uint8_t) ((INDEX & 0b111u) << 3u)) | (BASE & 0b111u) )   \

/*
void instr_rr(Buffer* buffer, Mnemonic opcode, Register op1, Register op2)
{
    Instruction_bytes instr = {};
    
    instr.prefix = 0x48;

    instr.opcode.op  = opcode >> 2; // opcodes are presented in 8-bit values, so we need a shift
    instr.opcode.dir = 0x0;
    instr.opcode.sz  = 0x1;
    
    instr.modrm.mode = 0x3;
    instr.modrm.reg  = op2.id;
    instr.modrm.rm   = op1.id;

    instr.is_prefix = true;
    instr.is_opcode = true;
    instr.is_modrm  = true;
    instr.is_sib    = false;
    instr.is_disp   = false;

    write_instr(buffer, instr);
}

// TODO: conditional compilation on addressing type
void instr_rm(Buffer* buffer, Mnemonic opcode, Register op1, Memory op2)
{
    Instruction_bytes instr = {};

    instr.prefix = 0x48;

    instr.opcode.op  = opcode >> 2; // opcodes are presented in 8-bit values, we need only highest 6 bits
    instr.opcode.dir = 0x1;   // 0: R/M <- REG (mr), 1: REG <- R/M (rm)
    instr.opcode.sz  = 0x1;
    
    instr.modrm.reg = op1.id;

    instr.is_opcode = true;
    instr.is_prefix = true;

    memory_operand(&instr, op2);
    
    write_instr(buffer, instr);
}

void instr_mr(Buffer* buffer, Mnemonic opcode, Memory op1, Register op2)
{
    Instruction_bytes instr = {};

    instr.prefix = 0x48;

    instr.opcode.op  = opcode >> 2; // opcodes are presented in 8-bit values, we need only highest 6 bits
    instr.opcode.dir = 0x0;   // 0: R/M <- REG (mr), 1: REG <- R/M (rm)
    instr.opcode.sz  = 0x1;
    
    instr.modrm.reg = op2.id;

    instr.is_opcode = true;
    instr.is_prefix = true;

    memory_operand(&instr, op1);
    
    write_instr(buffer, instr);
}

void instr_ri(Buffer* buffer, Mnemonic opcode, Register op1, Immediate op2)
{
    assert(buffer);

    
}
*/

static void memory_operand(Buffer* buffer, uint8_t reg_op, Memory op)
{
    // Base only (indirect register addressing)
    // [00 reg base] [---xx---] [---xx---]
    // [00 011 000 ] [---xx---] [---xx---]      add ebx, [rax]
    if(!op.is_index && op.is_base && !op.is_disp)
    {
        buffer_append_u8(buffer, GEN_MODRM(0b00u, reg_op, op.base));

        // no SIB
        
        // no disp
    }

    // Disp only
    // [00 reg 101 ] [---xx---] [displace]
    // [00 011 101 ] [---xx---] [11000000]      add ebx, [rip + 3]
    if(!op.is_index && !op.is_base && op.is_disp)
    {
        buffer_append_u8(buffer, GEN_MODRM(0b00, reg_op, 0b101));

        // no SIB

        buffer_append_i32(buffer, op.disp);
    }

    // index && base
    // [00 reg 100] [sc idx bas] [---xx---]
    // [00 011 100] [01 010 000] [---xx---]     add ebx, [2*rdx + rax]
    if(op.is_index && op.is_base && !op.is_disp)
    {
        buffer_append_u8(buffer, GEN_MODRM(0b00, reg_op, 0b100));

        buffer_append_u8(buffer, GEN_SIB(op.scale, op.index, op.base));

        // no disp
    }

    // index && base && disp
    // [10 reg 100] [sc idx bas] [displace]
    // [10 011 100] [01 010 000] [11100000]     add ebx, [2*rdx + rax + 7]
    if(op.is_index && op.is_base && op.is_disp)
    {
        // buffer_append_u8(buffer, GEN_MODRM(0b01, reg_op, 0b100));
        buffer_append_u8(buffer, GEN_MODRM(0b10, reg_op, 0b100));
        
        buffer_append_u8(buffer, GEN_SIB(op.scale, op.index, op.base));
        
        buffer_append_i32(buffer, op.disp);
    }

    // base && disp
    // [10 reg bas] [---xx---] [displace]
    // [10 011 010] [---xx---] [11100000]       add ebx, [rdx + 7]
    if(!op.is_index && op.is_base && op.is_disp)
    {
        buffer_append_u8(buffer, GEN_MODRM(0b10, reg_op, op.base));

        // no SIB

        buffer_append_i32(buffer, op.disp);
    }

    // index && disp
    // [00 reg 100] [sc idx 101] [displace]
    // [00 011 100] [01 010 101] [11100000]     add ebx, [2*rdx + 7]
    if(op.is_index && !op.is_base && op.is_disp)
    {
        buffer_append_u8(buffer, GEN_MODRM(0b00, reg_op, 0b100));

        buffer_append_u8(buffer, GEN_SIB(op.scale, op.index, 0b101));

        buffer_append_i32(buffer, op.disp);
    }
}

#define instr_mov(BUFFER, INSTR)                                \
    instr_ariphmetic(BUFFER, INSTR, 0x88, 0x88, 0xC7, 0b000)    \

#define instr_add(BUFFER, INSTR)                                \
    instr_ariphmetic(BUFFER, INSTR, 0x00, 0x00, 0x80, 0b000)    \

#define instr_sub(BUFFER, INSTR)                                \
    instr_ariphmetic(BUFFER, INSTR, 0x28, 0x28, 0x80, 0b101)    \
    
static void instr_ariphmetic(Buffer* buffer, Instruction instr, uint8_t register_op, uint8_t memory_op, uint8_t immediate_op, uint8_t immediate_reg_op)
{
    assert(buffer);

    buffer_append_u8(buffer, GEN_PREFIX(0x48));

    switch(instr.op1.type)
    {
        case OP_REGISTER:
        {
            switch(instr.op2.type)
            {
                case OP_REGISTER:
                {
                    buffer_append_u8(buffer, GEN_OPCODE(register_op, 0b0, 0b1)); // dir = 0: R/M <- REG; sz = 1: 32 bits (64 because of prefix)
                    buffer_append_u8(buffer, GEN_MODRM(0b11, instr.op2.reg.id, instr.op1.reg.id));
                    break;
                }
                case OP_MEMORY:
                {
                    buffer_append_u8(buffer, GEN_OPCODE(memory_op, 0b1, 0b1)); // dir = 1: REG <- R/M; sz = 1: 32 bits (64 because of prefix)
                    memory_operand(buffer, instr.op1.reg.id, instr.op2.mem);
                    break;
                }
                case OP_IMMEDIATE:
                {
                    buffer_append_u8(buffer, GEN_OPCODE(immediate_op, 0b0, 0b1));
                    buffer_append_u8(buffer, GEN_MODRM(0b11, immediate_reg_op, instr.op1.reg.id));
                    buffer_append_i32(buffer, instr.op2.imm.val);
                    break;
                }
                case OP_NOTYPE: default:
                {
                    assert(0);
                }
            }
            break;
        }
        case OP_MEMORY:
        {
            switch(instr.op2.type)
            {
                case OP_REGISTER:
                {
                    buffer_append_u8(buffer, GEN_OPCODE(register_op, 0b0, 0b1)); // dir = 0: R/M <- REG; sz = 1: 32 bits (64 because of prefix)
                    memory_operand(buffer, instr.op2.reg.id, instr.op1.mem);
                    break;
                }
                case OP_IMMEDIATE:
                {
                    buffer_append_u8(buffer, GEN_OPCODE(immediate_op, 0b0, 0b1));
                    memory_operand(buffer, immediate_reg_op, instr.op1.mem);
                    buffer_append_i32(buffer, instr.op2.imm.val);
                    break;
                }
                case OP_MEMORY: case OP_NOTYPE: default:
                {
                    assert(0);
                }
            }
            break;
        }
        case OP_IMMEDIATE: case OP_NOTYPE: default:
        {
            assert(0);
        }
    }
}

static void instr_push(Buffer* buffer, Instruction instr)
{
    assert(buffer);
    assert(instr.op2.type == OP_NOTYPE);

    switch(instr.op1.type)
    {
        case OP_REGISTER:
        {
            buffer_append_u8(buffer, GEN_OPCODE(0x50 + instr.op1.reg.id, 0b0, 0b0));
            break;
        }
        case OP_MEMORY:
        {
            buffer_append_u8(buffer, GEN_OPCODE(0xFF, 0b0, 0b0));
            memory_operand(buffer, 0b110, instr.op1.mem);
            break;
        }
        case OP_IMMEDIATE:
        {
            buffer_append_u8(buffer, GEN_OPCODE(0x68, 0b0, 0b0));
            buffer_append_i32(buffer, instr.op1.imm.val);
            break;
        }
        case OP_NOTYPE: default:
        {
            assert(0);
        }
    }
}

static void instr_pop(Buffer* buffer, Instruction instr)
{
    assert(buffer);
    assert(instr.op2.type == OP_NOTYPE);

    switch(instr.op1.type)
    {
        case OP_REGISTER:
        {
            buffer_append_u8(buffer, GEN_OPCODE(0x58 + instr.op1.reg.id, 0b0, 0b0));
            break;
        }
        case OP_MEMORY:
        {
            buffer_append_u8(buffer, GEN_OPCODE(0x8F, 0b0, 0b0));
            memory_operand(buffer, 0b000, instr.op1.mem);
            break;
        }
        case OP_NOTYPE: case OP_IMMEDIATE: default:
        {
            assert(0);
        }
    }
}

static void instr_call(Buffer* buffer, Instruction instr)
{
    assert(buffer);
    assert(instr.op2.type == OP_NOTYPE);

    switch(instr.op1.type)
    {
        case OP_REGISTER:
        {
            buffer_append_u8(buffer, GEN_OPCODE(0xFF, 0b0, 0b0));
            buffer_append_u8(buffer, GEN_MODRM(0b11, 0b010, instr.op1.reg.id));
            break;
        }
        case OP_MEMORY:
        {
            buffer_append_u8(buffer, GEN_OPCODE(0xFF, 0b0, 0b0));
            memory_operand(buffer, 0b010, instr.op1.mem);
            break;
        }
        case OP_IMMEDIATE:
        {
            buffer_append_u8(buffer, GEN_OPCODE(0xE8, 0b0, 0b0));
            buffer_append_i32(buffer, instr.op1.imm.val);
            break;
        }
        case OP_NOTYPE: default:
        {
            assert(0);
        }
    }
}

static void instr_ret(Buffer* buffer, Instruction instr)
{
    assert(buffer);
    assert(instr.op1.type == OP_NOTYPE && instr.op2.type == OP_NOTYPE);

    buffer_append_u8(buffer, GEN_OPCODE(0xC3, 0b0, 0b0));
}

void encode(Buffer* buffer, Instruction instr)
{
    assert(buffer);

    switch(instr.mnemonic)
    {
        case ADD:
            instr_add(buffer, instr);
            break;
        case MOV:
            instr_mov(buffer, instr);
            break;
        case SUB:
            instr_sub(buffer, instr);
            break;
        case PUSH:
            instr_push(buffer, instr);
            break;
        case POP:
            instr_pop(buffer, instr);
            break;
        case CALL:
            instr_call(buffer, instr);
            break;
        case RET:
            instr_ret(buffer, instr);
            break;
        default:
            assert(0);
    }
}

#undef instr_add
#undef instr_mov
#undef instr_sub

/*

int main()
{
    Buffer buffer = {};

    encode(&buffer, {ADD, RAX, RAX});
    encode(&buffer, {ADD, RDX, MEM(2, RDI, RAX, -6)});
    encode(&buffer, {ADD, MEM(2, RDI, RAX, 6), RCX});
    encode(&buffer, {ADD, RAX, IMM(0x12345678)});
    
    encode(&buffer, {SUB, RAX, RAX});
    encode(&buffer, {SUB, RDX, MEM(2, RDI, RAX, -6)});
    encode(&buffer, {SUB, MEM(2, RDI, RAX, 6), RCX});
    encode(&buffer, {SUB, RAX, IMM(0x12345678)});

    encode(&buffer, {MOV, RBX, RBX});
    encode(&buffer, {MOV, RDX, MEM(2, RDI, RAX, -6)});
    encode(&buffer, {MOV, MEM(2, RDI, RAX, 6), RCX});
    encode(&buffer, {MOV, RAX, IMM(0x12345678)});

    encode(&buffer, {PUSH, RDX, {}});
    encode(&buffer, {PUSH, MEM(2, RDI, RAX, 6), {}});
    encode(&buffer, {PUSH, IMM(0x12345678)});

    encode(&buffer, {CALL, RDX, {}});
    encode(&buffer, {CALL, MEM(2, RDI, RAX, 6), {}});
    encode(&buffer, {CALL, IMM(0x12345678)});

    encode(&buffer, {RET, {}, {}});

    FILE* outfile = fopen("output", "wb");
    assert(outfile);

    fwrite(buffer.arr, sizeof(uint8_t), buffer.size, outfile);

    fclose(outfile);
    
    return 0;
}

*/
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "encode.h"
#include "../../include/logs/logs.h"

///////////////////////////////////////////////////////////////////////////////

static bool EQUAL(Operand op1, Operand op2)
{
    return !memcmp(&op1, &op2, sizeof(Operand));
}

Operand IMM32(int32_t val)
{
    Operand op = {};
    op.type = OP_IMM32;
    op.imm32 = val;
    return op;
}

Operand MEM(uint8_t scale, Operand index, Operand base, int32_t disp)
{
    Operand op = {.type = OP_MEMORY};

    assert(index.type == OP_REGISTER || index.type == OP_NOTYPE);
    assert(base.type  == OP_REGISTER || base.type  == OP_NOTYPE);

    if(index.type == OP_REGISTER)
    {
        if(EQUAL(index, RSP))
            assert(0 && "RSP cannot be index register");

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

    // RBP addressing needs displacement field
    if(disp || EQUAL(base, RBP) || EQUAL(index, RBP) || (!op.mem.is_index && !op.mem.is_base))
    {
        op.mem.disp    = disp;
        op.mem.is_disp = true;
    }

    return op;
}

///////////////////////////////////////////////////////////////////////////////

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
const Operand R8 = {
    .type = OP_REGISTER,
    .reg = (Register) {.id = 0b1000}
};
const Operand R9 = {
    .type = OP_REGISTER,
    .reg = (Register) {.id = 0b1001}
};
const Operand R10 = {
    .type = OP_REGISTER,
    .reg = (Register) {.id = 0b1010}
};
const Operand R11 = {
    .type = OP_REGISTER,
    .reg = (Register) {.id = 0b1011}
};
const Operand R12 = {
    .type = OP_REGISTER,
    .reg = (Register) {.id = 0b1100}
};
const Operand R13 = {
    .type = OP_REGISTER,
    .reg = (Register) {.id = 0b1101}
};
const Operand R14 = {
    .type = OP_REGISTER,
    .reg = (Register) {.id = 0b1110}
};
const Operand R15 = {
    .type = OP_REGISTER,
    .reg = (Register) {.id = 0b1111}
};

///////////////////////////////////////////////////////////////////////////////

#define GEN_PREFIX(PREFIX)  \
    (PREFIX)                \

#define GEN_OPCODE(OP, DIR_EXT, SZ)                                           \
    ( (OP) | ((uint8_t) ((DIR_EXT & 0b1u) << 1u)) | ((uint8_t) (SZ & 0b1u)) ) \

#define GEN_MODRM(MOD, REG_OP, RM)                                                                \
    ( ((uint8_t) ((MOD & 0b11u) << 6u)) | ((uint8_t) ((REG_OP & 0b111u) << 3u)) | (RM & 0b111u) ) \

#define GEN_SIB(SCALE, INDEX, BASE)                                                                    \
    ( ((uint8_t) ((SCALE & 0b11u) << 6u)) | ((uint8_t) ((INDEX & 0b111u) << 3u)) | (BASE & 0b111u) )   \

const uint8_t REX_B = 0x40 + 0b0001; // MODRM.r/m / SIB.base / opcode extension
const uint8_t REX_X = 0x40 + 0b0010; // SIB.index extension
const uint8_t REX_R = 0x40 + 0b0100; // MODRM.reg extension
const uint8_t REX_W = 0x40 + 0b1000; // 64 bit operands

static void rex_prefix(Buffer* buffer, Instruction instr, uint8_t rex)
{
    switch(instr.op1.type)
    {
        case OP_REGISTER:
        {
            switch(instr.op2.type)
            {
                case OP_REGISTER: // R/M <- reg
                {
                    if(instr.op1.reg.id & 0b1000)
                        rex |= REX_B;
                    if(instr.op2.reg.id & 0b1000)
                        rex |= REX_R;
                    break;
                }
                case OP_MEMORY:   // reg <- R/M
                {
                    if(instr.op1.reg.id & 0b1000)
                        rex |= REX_R;
                    if((instr.op2.mem.base & 0b1000) && instr.op2.mem.is_base)
                        rex |= REX_B;
                    if((instr.op2.mem.index & 0b1000) && instr.op2.mem.is_index)
                        rex |= REX_X;
                    break;
                }
                case OP_IMM32: case OP_NOTYPE:    // R/M <-
                {
                    if(instr.op1.reg.id & 0b1000)
                        rex |= REX_B;
                    break;
                }                
                default:
                {
                    assert(0 && "Invalid operand combination");
                }
            }
            break;
        }
        case OP_MEMORY:
        {
            if((instr.op1.mem.base & 0b1000) && instr.op1.mem.is_base)
                rex |= REX_B;
            if((instr.op1.mem.index & 0b1000) && instr.op1.mem.is_index)
                rex |= REX_X;

            switch(instr.op2.type)
            {
                case OP_REGISTER:   // R/M <- reg
                {
                    if(instr.op2.reg.id & 0b1000)
                        rex |= REX_R;
                }
                case OP_IMM32: case OP_NOTYPE:
                {
                    break;
                }
                default: case OP_MEMORY:
                {
                    assert(0 && "Invalid operand combination");
                }
            }
            break;
        }
        case OP_IMM32:
        {
            assert(instr.op2.type == OP_NOTYPE);
            break;
        }
        default: case OP_NOTYPE:
        {
            assert(0 && "Operand missing or is no type");
        }
    }

    if(rex)
        buffer_append_u8(buffer, rex); 
}

static void memory_operand(Buffer* buffer, uint8_t reg_op, Memory op)
{
    if(op.is_base && op.base == RSP.reg.id)
    {
        // base only -> index && base
        // [00 011 100] [00 100 100] [---xx---]     add ebx, [rsp]
        if(!op.is_index && !op.is_disp)
        {
            buffer_append_u8(buffer, GEN_MODRM(0b00, reg_op, 0b100));

            buffer_append_u8(buffer, GEN_SIB(op.scale, 0b100, 0b100));

            // no disp

            return;
        }

        // index && base -- OK
        // [00 011 100] [01 010 100] [---xx---]     add ebx, [2*rdx + rsp]

        // index && base && disp -- OK
        // [10 011 100] [01 011 100] [11100000]     add ebx, [rsp + 2*rbx + 7]

        // base && disp -> index && base && disp
        // [10 011 100] [00 100 100] [11100000]     add ebx, [rsp + 7]
        if(!op.is_index && op.is_disp)
        {
            // buffer_append_u8(buffer, GEN_MODRM(0b01, reg_op, 0b100));
            buffer_append_u8(buffer, GEN_MODRM(0b10, reg_op, 0b100));
            
            buffer_append_u8(buffer, GEN_SIB(0b00, 0b100, 0b100));
            
            buffer_append_i32(buffer, op.disp);

            return;
        }
    }

    // base only (indirect register addressing)
    // [00 reg bas] [---xx---] [---xx---]
    // [00 011 000] [---xx---] [---xx---]      add ebx, [rax]
    // <see index && base>                     add ebx, [rsp]
    if(!op.is_index && op.is_base && !op.is_disp)
    {
        buffer_append_u8(buffer, GEN_MODRM(0b00u, reg_op, op.base));

        // no SIB
        
        // no disp
        
        return;
    }

    // disp only
    // [00 reg 101] [---xx---] [displace]
    // [00 011 101] [---xx---] [11000000]      add ebx, [rip + 3]
    if(!op.is_index && !op.is_base && op.is_disp)
    {
        buffer_append_u8(buffer, GEN_MODRM(0b00, reg_op, 0b101));

        // no SIB

        buffer_append_i32(buffer, op.disp);

        return;
    }

    // index && base
    // [00 reg 100] [sc idx bas] [---xx---]
    // [00 011 100] [01 010 000] [---xx---]     add ebx, [2*rdx + rax]
    // [00 011 100] [00 100 100] [---xx---]     add ebx, [rsp]
    // [00 011 100] [01 010 100] [---xx---]     add ebx, [2*rdx + rsp]
    if(op.is_index && op.is_base && !op.is_disp)
    {
        buffer_append_u8(buffer, GEN_MODRM(0b00, reg_op, 0b100));

        buffer_append_u8(buffer, GEN_SIB(op.scale, op.index, op.base));

        // no disp

        return;
    }

    // index && base && disp
    // [10 reg 100] [sc idx bas] [displace]
    // [10 011 100] [01 010 000] [11100000]     add ebx, [2*rdx + rax + 7]
    // [10 011 100] [00 100 100] [11100000]     add ebx, [rsp + 7]
    // [10 011 100] [01 011 100] [11100000]     add ebx, [rsp + 2*rbx + 7]
    // <not valid>                              add ebx, [rbx + 2*rsp + 7]
    if(op.is_index && op.is_base && op.is_disp)
    {
        // buffer_append_u8(buffer, GEN_MODRM(0b01, reg_op, 0b100));
        buffer_append_u8(buffer, GEN_MODRM(0b10, reg_op, 0b100));
        
        buffer_append_u8(buffer, GEN_SIB(op.scale, op.index, op.base));
        
        buffer_append_i32(buffer, op.disp);

        return;
    }

    // base && disp
    // [10 reg bas] [---xx---] [displace]
    // [10 011 010] [---xx---] [11100000]       add ebx, [rdx + 7]
    // <see index && base && disp>              add ebx, [rsp + 7]
    if(!op.is_index && op.is_base && op.is_disp)
    {
        buffer_append_u8(buffer, GEN_MODRM(0b10, reg_op, op.base));

        // no SIB

        buffer_append_i32(buffer, op.disp);

        return;
    }

    // index && disp
    // [00 reg 100] [sc idx 101] [displace]
    // [00 011 100] [01 010 101] [11100000]     add ebx, [2*rdx + 7]
    // <not valid>                              add ebx, [2*rsp + 7]
    if(op.is_index && !op.is_base && op.is_disp)
    {
        buffer_append_u8(buffer, GEN_MODRM(0b00, reg_op, 0b100));

        buffer_append_u8(buffer, GEN_SIB(op.scale, op.index, 0b101));

        buffer_append_i32(buffer, op.disp);

        return;
    }
}

///////////////////////////////////////////////////////////////////////////////

#define instr_mov(BUFFER, INSTR)                            \
    instr_ariphmetic(BUFFER, INSTR, 0x88, 0xC7, 0b000)      \

#define instr_add(BUFFER, INSTR)                            \
    instr_ariphmetic(BUFFER, INSTR, 0x00, 0x80, 0b000)      \

#define instr_sub(BUFFER, INSTR)                            \
    instr_ariphmetic(BUFFER, INSTR, 0x28, 0x80, 0b101)      \
    
#define instr_cmp(BUFFER, INSTR)                            \
    instr_ariphmetic(BUFFER, INSTR, 0x38, 0x80, 0b111)      \

#define instr_and(BUFFER, INSTR)                            \
    instr_ariphmetic(BUFFER, INSTR, 0x20, 0x80, 0b100)      \

#define instr_xor(BUFFER, INSTR)                            \
    instr_ariphmetic(BUFFER, INSTR, 0x30, 0x80, 0b110)      \

#define instr_or(BUFFER, INSTR)                             \
    instr_ariphmetic(BUFFER, INSTR, 0x08, 0x80, 0b001)      \

// TEST doesn't need to have REG <- R/M and R/M -> REG, therefore
// only op1 can be memory operand.
#define instr_test(BUFFER, INSTR)                           \
    do                                                      \
    {                                                       \
        assert(INSTR.op2.type != OP_MEMORY);                \
        instr_ariphmetic(BUFFER, INSTR, 0x84, 0xF7, 0b000); \
    } while (0)                                             \
    

static void instr_ariphmetic(Buffer* buffer, Instruction instr, uint8_t regmem_op, uint8_t immediate_op, uint8_t immediate_reg_op)
{
    assert(buffer);

    rex_prefix(buffer, instr, REX_W);

    switch(instr.op1.type)
    {
        case OP_REGISTER:
        {
            switch(instr.op2.type)
            {
                case OP_REGISTER:
                {
                    buffer_append_u8(buffer, GEN_OPCODE(regmem_op, 0b0, 0b1)); // dir = 0: R/M <- REG; sz = 1: 32 bits (64 because of prefix)
                    buffer_append_u8(buffer, GEN_MODRM(0b11, instr.op2.reg.id, instr.op1.reg.id));
                    break;
                }
                case OP_MEMORY:
                {
                    buffer_append_u8(buffer, GEN_OPCODE(regmem_op, 0b1, 0b1)); // dir = 1: REG <- R/M; sz = 1: 32 bits (64 because of prefix)
                    memory_operand(buffer, instr.op1.reg.id, instr.op2.mem);
                    break;
                }
                case OP_IMM32:
                {
                    buffer_append_u8(buffer, GEN_OPCODE(immediate_op, 0b0, 0b1));
                    buffer_append_u8(buffer, GEN_MODRM(0b11, immediate_reg_op, instr.op1.reg.id));
                    buffer_append_i32(buffer, instr.op2.imm32);
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
                    buffer_append_u8(buffer, GEN_OPCODE(regmem_op, 0b0, 0b1)); // dir = 0: R/M <- REG; sz = 1: 32 bits (64 because of prefix)
                    memory_operand(buffer, instr.op2.reg.id, instr.op1.mem);
                    break;
                }
                case OP_IMM32:
                {
                    buffer_append_u8(buffer, GEN_OPCODE(immediate_op, 0b0, 0b1));
                    memory_operand(buffer, immediate_reg_op, instr.op1.mem);
                    buffer_append_i32(buffer, instr.op2.imm32);
                    break;
                }
                case OP_MEMORY: case OP_NOTYPE: default:
                {
                    assert(0);
                }
            }
            break;
        }
        case OP_IMM32: case OP_NOTYPE: default:
        {
            assert(0);
        }
    }
}

#define instr_sete(BUFFER, INSTR)          \
    instr_set_cond(BUFFER, INSTR, 0x94)    \

#define instr_setne(BUFFER, INSTR)         \
    instr_set_cond(BUFFER, INSTR, 0x95)    \

#define instr_setge(BUFFER, INSTR)         \
    instr_set_cond(BUFFER, INSTR, 0x9D)    \

#define instr_setle(BUFFER, INSTR)         \
    instr_set_cond(BUFFER, INSTR, 0x9E)    \

#define instr_setg(BUFFER, INSTR)          \
    instr_set_cond(BUFFER, INSTR, 0x9F)    \

#define instr_setl(BUFFER, INSTR)          \
    instr_set_cond(BUFFER, INSTR, 0x9C)    \

static void instr_set_cond(Buffer* buffer, Instruction instr, uint8_t op)
{
    assert(buffer);
    assert(instr.op1.type == OP_REGISTER && instr.op2.type == OP_NOTYPE);

    buffer_append_u8(buffer, GEN_PREFIX(0x0F));
    buffer_append_u8(buffer, GEN_OPCODE(op, 0b0, 0b0));
    buffer_append_u8(buffer, GEN_MODRM(0b11, 0b000, instr.op1.reg.id));
}

static void instr_imul(Buffer* buffer, Instruction instr)
{
    assert(buffer);
    assert(instr.op1.type == OP_REGISTER);

    rex_prefix(buffer, instr, REX_W);
    buffer_append_u8(buffer, GEN_PREFIX(0x0F));

    // Dst operand is always in REG field
    buffer_append_u8(buffer, GEN_OPCODE(0xAF, 0b1, 0b1)); // dir = 1: REG <- R/M; sz = 1: 32 bits (64 because of prefix)

    switch(instr.op2.type)
    {
        case OP_REGISTER:
        {
            buffer_append_u8(buffer, GEN_MODRM(0b11, instr.op1.reg.id, instr.op2.reg.id));
            break;
        }
        case OP_MEMORY:
        {
            memory_operand(buffer, instr.op1.reg.id, instr.op2.mem);
            break;
        }
        default: case OP_IMM32: case OP_NOTYPE:
        {
            assert(0);
        }
    }
}

static void instr_cqo(Buffer* buffer, Instruction instr)
{
    assert(buffer);
    assert(instr.op1.type == OP_NOTYPE && instr.op2.type == OP_NOTYPE);

    buffer_append_u8(buffer, GEN_PREFIX(REX_W));
    buffer_append_u8(buffer, GEN_OPCODE(0x99, 0b0, 0b0));
}

static void instr_idiv(Buffer* buffer, Instruction instr)
{
    assert(buffer);
    assert(instr.op2.type == OP_NOTYPE);

    rex_prefix(buffer, instr, REX_W);

    buffer_append_u8(buffer, GEN_OPCODE(0xF7, 0b1, 0b1)); // dir = 1: REG <- R/M; sz = 1: 32 bits (64 because of prefix)

    switch(instr.op1.type)
    {
        case OP_REGISTER:
        {
            buffer_append_u8(buffer, GEN_MODRM(0b11, 0b111, instr.op1.reg.id));
            break;
        }
        case OP_MEMORY:
        {
            memory_operand(buffer, 0b111, instr.op1.mem);
            break;
        }
        default: case OP_IMM32: case OP_NOTYPE:
        {
            assert(0);
        }
    }
}

static void instr_lea(Buffer* buffer, Instruction instr)
{
    assert(buffer);
    assert(instr.op1.type == OP_REGISTER);
    assert(instr.op2.type == OP_MEMORY);

    rex_prefix(buffer, instr, REX_W);

    buffer_append_u8(buffer, GEN_OPCODE(0x8D, 0b0, 0b0));
    memory_operand(buffer, instr.op1.reg.id, instr.op2.mem);
}

static void instr_push(Buffer* buffer, Instruction instr)
{
    assert(buffer);
    assert(instr.op2.type == OP_NOTYPE);

    rex_prefix(buffer, instr, 0x0);

    switch(instr.op1.type)
    {
        case OP_REGISTER:
        {
            buffer_append_u8(buffer, GEN_OPCODE(0x50 + (instr.op1.reg.id & 0b111), 0b0, 0b0));
            break;
        }
        case OP_MEMORY:
        {
            buffer_append_u8(buffer, GEN_OPCODE(0xFF, 0b0, 0b0));
            memory_operand(buffer, 0b110, instr.op1.mem);
            break;
        }
        case OP_IMM32:
        {
            buffer_append_u8(buffer, GEN_OPCODE(0x68, 0b0, 0b0));
            buffer_append_i32(buffer, instr.op1.imm32);
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

    rex_prefix(buffer, instr, 0x0);

    switch(instr.op1.type)
    {
        case OP_REGISTER:
        {
            buffer_append_u8(buffer, GEN_OPCODE(0x58 + (instr.op1.reg.id & 0b111), 0b0, 0b0));
            break;
        }
        case OP_MEMORY:
        {
            buffer_append_u8(buffer, GEN_OPCODE(0x8F, 0b0, 0b0));
            memory_operand(buffer, 0b000, instr.op1.mem);
            break;
        }
        case OP_NOTYPE: case OP_IMM32: default:
        {
            assert(0);
        }
    }
}

static void instr_jmp(Buffer* buffer, Instruction instr)
{
    assert(buffer);
    assert(instr.op1.type == OP_IMM32);
    assert(instr.op2.type == OP_NOTYPE);

    buffer_append_u8(buffer, GEN_OPCODE(0xE9, 0b0, 0b0));
    buffer_append_i32(buffer, instr.op1.imm32);
}

#define instr_je(BUFFER, INSTR)              \
    instr_cond_jumps(BUFFER, INSTR, 0x84)    \

#define instr_jne(BUFFER, INSTR)             \
    instr_cond_jumps(BUFFER, INSTR, 0x85)    \

static void instr_cond_jumps(Buffer* buffer, Instruction instr, uint8_t immediate_op)
{
    assert(buffer);
    assert(instr.op1.type == OP_IMM32);
    assert(instr.op2.type == OP_NOTYPE);

    buffer_append_u8(buffer, 0x0F);
    buffer_append_u8(buffer, GEN_OPCODE(immediate_op, 0b0, 0b0));
    buffer_append_i32(buffer, instr.op1.imm32);
}

static void instr_call(Buffer* buffer, Instruction instr)
{
    assert(buffer);
    assert(instr.op2.type == OP_NOTYPE);

    rex_prefix(buffer, instr, 0x0);

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
        case OP_IMM32:
        {
            buffer_append_u8(buffer, GEN_OPCODE(0xE8, 0b0, 0b0));
            buffer_append_i32(buffer, instr.op1.imm32);
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

///////////////////////////////////////////////////////////////////////////////

#define DEF_INSTR(ENUM, FUNC)           \
    case ENUM:                          \
        instr_##FUNC(buffer, instr);    \
        break;                          \

void encode(Buffer* buffer, Instruction instr)
{
    assert(buffer);

    switch(instr.mnemonic)
    {

        #include "encode.inc"

        default: case INVALID_INSTR:
            assert(0);
    }
}

#undef DEF_INSTR
#undef GEN_PREFIX
#undef GEN_OPCODE
#undef GEN_MODRM
#undef GEN_SIB

///////////////////////////////////////////////////////////////////////////////

// Tests

/*

int main()
{
    Buffer buffer = {};
    printf("ADD\n");

    encode(&buffer, {ADD, RAX, RAX});
    encode(&buffer, {ADD, RDX, MEM(2, RDI, RAX, -6)});
    encode(&buffer, {ADD, MEM(2, RDI, RAX, 6), RCX});
    encode(&buffer, {ADD, RAX, IMM32(0x12345678)});
    encode(&buffer, {ADD, MEM(2, RDI, RAX, -6), IMM32(0x12345678)});

    encode(&buffer, {PUSH, RAX, {}});
    printf("RBP with -6 disp\n");

    // encode(&buffer, {ADD, RBP, RBP});
    // encode(&buffer, {ADD, RAX, MEM(2, RBP, RAX, -6)});
    // encode(&buffer, {ADD, RDX, MEM(2, RAX, RBP, -6)});
    // encode(&buffer, {ADD, RAX, MEM(2, RBP, RBP, -6)});
    
    // encode(&buffer, {ADD, RAX, MEM(0x0, {}, RBP, -6)});
    // encode(&buffer, {ADD, RAX, MEM(2, RBP, {}, -6)});

    // encode(&buffer, {PUSH, RBX, {}});
    // printf("RBP with 0 disp\n");
    
    // encode(&buffer, {ADD, RAX, MEM(2, RBP, RAX, {})});
    // encode(&buffer, {ADD, RDX, MEM(2, RAX, RBP, {})});
    // encode(&buffer, {ADD, RAX, MEM(2, RBP, RBP, {})});
    
    // encode(&buffer, {ADD, RAX, MEM(0x0, {}, RBP, {})});
    // encode(&buffer, {ADD, RAX, MEM(2, RBP, {}, {})});

    // encode(&buffer, {PUSH, RCX, {}});
    // printf("RSP\n");

    // encode(&buffer, {ADD, RSP, RSP});
    // encode(&buffer, {ADD, RDX, MEM(2, RAX, RSP, {})});
    // // encode(&buffer, {ADD, RDX, MEM(2, RSP, RAX, {})});  not valid
    // encode(&buffer, {ADD, RAX, MEM(0x0, {}, RSP, {})});
    // encode(&buffer, {ADD, RAX, MEM(2, RBP, RSP, {})});

    encode(&buffer, {PUSH, RSI, {}});
    printf("R8\n");

    encode(&buffer, {ADD, R8, R8});
    encode(&buffer, {ADD, RDX, MEM(2, RAX, R8, -6)});
    encode(&buffer, {ADD, RDX, MEM(2, R8, RAX, -6)});
    encode(&buffer, {ADD, RAX, MEM(0x0, {}, R8, -6)});
    encode(&buffer, {ADD, RAX, MEM(2, RDX, R8, -6)});
    encode(&buffer, {ADD, R8,  MEM(0, {}, {}, 0x30)});

    encode(&buffer, {PUSH, RDI, {}});
    printf("CMP\n");

    encode(&buffer, {CMP, RAX, RAX});
    encode(&buffer, {CMP, RBX, MEM(0, {}, {}, 0x30)});
    encode(&buffer, {CMP, RDX, MEM(2, RDI, RAX, -6)});
    encode(&buffer, {CMP, MEM(2, RDI, RAX, 6), RCX});
    encode(&buffer, {CMP, RAX, IMM32(0x12345678)});
    printf("SUB\n");

    encode(&buffer, {SUB, RAX, RAX});
    encode(&buffer, {SUB, RBX, MEM(0, {}, {}, 0x30)});
    encode(&buffer, {SUB, RDX, MEM(2, RDI, RAX, -6)});
    encode(&buffer, {SUB, MEM(2, RDI, RAX, 6), RCX});
    encode(&buffer, {SUB, RAX, IMM32(0x12345678)});
    printf("MOV\n");

    encode(&buffer, {MOV, RBX, RBX});
    encode(&buffer, {MOV, RBX, MEM(0, {}, {}, 0x30)});
    encode(&buffer, {MOV, RDX, MEM(2, RDI, RAX, -6)});
    encode(&buffer, {MOV, MEM(2, RDI, RAX, 6), RCX});
    encode(&buffer, {MOV, RAX, IMM32(0x12345678)});
    printf("PUSH\n");

    encode(&buffer, {PUSH, RDX, {}});
    encode(&buffer, {PUSH, MEM(0, {}, {}, 0x30)});
    encode(&buffer, {PUSH, MEM(2, RDI, RAX, 6), {}});
    encode(&buffer, {PUSH, MEM(0, {}, RAX, 6), {}});
    encode(&buffer, {PUSH, IMM32(0x12345678)});
    printf("JMP\n");

    encode(&buffer, {JMP, IMM32(0x12345678)});
    encode(&buffer, {JE,  IMM32(0x12345678)});
    encode(&buffer, {JNE, IMM32(0x12345678)});

    printf("POP\n");

    encode(&buffer, {POP, RDX, {}});
    encode(&buffer, {POP, MEM(0, {}, {}, 0x30)});
    encode(&buffer, {POP, MEM(2, RDI, RAX, 6), {}});
    encode(&buffer, {POP, MEM(0, {}, RAX, 6), {}});

    printf("CALL\n");

    encode(&buffer, {CALL, RDX, {}});
    encode(&buffer, {CALL, MEM(2, RDI, RAX, 6), {}});
    encode(&buffer, {CALL, IMM32(0x12345678)});
    printf("IMUL, IDIV\n");

    encode(&buffer, {IMUL, RAX, RBX});
    encode(&buffer, {IMUL, RBX, MEM(0, {}, {}, 0x30)});
    encode(&buffer, {IMUL, RDX, MEM(2, RDI, RAX, -6)});
    // encode(&buffer, {IMUL, MEM(2, RDI, RAX, 6), RCX});
    // encode(&buffer, {IMUL, RAX, IMM32(0x12345678)});

    encode(&buffer, {IDIV, RAX, {}});
    encode(&buffer, {IDIV, MEM(0, {}, {}, 0x30), {}});
    encode(&buffer, {IDIV, MEM(2, RDI, RAX, -6), {}});

    printf("TEST\n");

    encode(&buffer, {TEST, RAX, RAX});
    encode(&buffer, {TEST, MEM(0, {}, {}, 0x30), RBX});
    encode(&buffer, {TEST, MEM(2, RDI, RAX, -6), RCX});
    encode(&buffer, {TEST, RAX, IMM32(0x12345678)});
    encode(&buffer, {TEST, MEM(2, RDI, RAX, -6), IMM32(0x12345678)});

    printf("XOR\n");

    encode(&buffer, {XOR, RAX, RAX});
    encode(&buffer, {XOR, RDX, MEM(2, RDI, RAX, -6)});
    encode(&buffer, {XOR, MEM(2, RDI, RAX, 6), RCX});
    encode(&buffer, {XOR, RAX, IMM32(0x12345678)});
    encode(&buffer, {XOR, MEM(2, RDI, RAX, -6), IMM32(0x12345678)});

    printf("RET\n");

    encode(&buffer, {RET, {}, {}});

    FILE* outfile = fopen("output", "wb");
    assert(outfile);

    fwrite(buffer.buf, sizeof(uint8_t), buffer.size, outfile);

    fclose(outfile);
    
    return 0;
}

*/

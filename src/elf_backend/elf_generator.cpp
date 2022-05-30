#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "elf_generator.h"
#include "symtable.h"
#include "encode.h"
#include "elf_wrap.h"
#include "relocation.h"
#include "../../include/logs/logs.h"
#include "../reserved_names.h"

static Symtable*    SYMTABLE    = nullptr;
static Localtable*  LOCALTABLE  = nullptr;
static Relocations* RELOCATIONS = nullptr;

static Section* DATA = nullptr;
// static Section* INIT = nullptr;
static Section* TEXT = nullptr;

static generator_err IS_ERROR = GENERATOR_NOERR;

#define semantic_error(MSG_, TOK_)                                                      \
do                                                                                      \
{                                                                                       \
    IS_ERROR = GENERATOR_SEMANTIC_ERROR;                                                \
    fprintf(stderr, "\x1b[31mSemantic error:\x1b[0m %s : %s\n", (MSG_), std_demangle(TOK_));\
    FILE* stream_ = logs_get();                                                         \
                                                                                        \
    if(stream_)                                                                         \
    {                                                                                   \
        fprintf(stream_, "<span class = \"error\">Semantic error: %s : %s\n</span>"     \
                         "\t\t\t\tat %s:%d:%s\n",                                       \
                         (MSG_), std_demangle(TOK_),                                        \
                         __FILE__, __LINE__, __PRETTY_FUNCTION__);                      \
                                                                                        \
        return GENERATOR_NOERR;                                                         \
    }                                                                                   \
} while(0)                                                                              \

#define format_error(MSG_, TOK_)                                                        \
do                                                                                      \
{                                                                                       \
    IS_ERROR = GENERATOR_FORMAT_ERROR;                                                  \
    fprintf(stderr, "\x1b[31mFormat error:\x1b[0m %s : %s\n", (MSG_), std_demangle(TOK_));  \
    FILE* stream_ = logs_get();                                                         \
                                                                                        \
    if(stream_)                                                                         \
    {                                                                                   \
        fprintf(stream_, "<span class = \"error\">Format error: %s : %s\n</span>"       \
                         "\t\t\t\tat %s : %d : %s\n",                                   \
                         (MSG_), std_demangle(TOK_),                                        \
                         __FILE__, __LINE__, __PRETTY_FUNCTION__);                      \
                                                                                        \
        return GENERATOR_NOERR;                                                         \
    }                                                                                   \
} while(0)                                                                              \

///////////////////////////////////////////////////////////////////////////////////////////////////

static generator_err expression(Section* sect, Node* node);
static generator_err statement(Node* node);

static generator_err number(Section* sect, Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_NUMBER);

    if(node->left || node->right)
        format_error("Number has descendants", &node->tok);

    encode(&sect->buffer, {PUSH, IMM32((int32_t) node->tok.val.num)});

    return GENERATOR_NOERR;
}

static generator_err variable(Section* sect, Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_ID);

    if(node->left)
        semantic_error("Variable has 'const' specifier in expression", &node->tok);
    
    Operand index_reg = {};
    uint8_t scale = 0;

    if(node->right)
    {
        PASS$(!expression(sect, node->right), return GENERATOR_PASS_ERROR; );
        
        encode(&sect->buffer, {POP, RCX, {}});
        index_reg = RCX;
        scale     = 8;
    }

    Symbol    sym = {};
    uint64_t  sym_index = 0;
    Local_var local = {};

    if(symtable_find(SYMTABLE, node->tok.val.name, &sym, &sym_index) == 0)
    {
        assert(sym.type == SYMBOL_TYPE_VARIABLE);

        encode(&sect->buffer, {LEA, RAX, MEM(0, {}, {}, 0x0)});

        Reloc reloc = {.dst_section_descriptor = sect->descriptor,
                       .dst_offset = sect->buffer.pos - sizeof(int32_t),
                       .dst_init_val = - (int32_t) sizeof(int32_t),
                       .src_nametable_index = sym_index
                      };
        
        relocations_insert(RELOCATIONS, reloc);

        encode(&sect->buffer, {PUSH, MEM(scale, index_reg, RAX, 0), {}});
    }
    else if(localtable_find(LOCALTABLE, node->tok.val.name, &local) == 0)
    {
        encode(&sect->buffer, {PUSH, MEM(scale, index_reg, RBP, local.offset), {}});
    }
    else
    {
        semantic_error("Variable wasn't declared", &node->tok);
    }

    return GENERATOR_NOERR;
}

#define DEF_OPER(MANGLE, OPCODE)                                    \
    case TOK_##MANGLE:                                              \
    {                                                               \
        encode(&sect->buffer, {OPCODE, RAX, RBX});                  \
        encode(&sect->buffer, {PUSH, RAX, {}});                     \
        break;                                                      \
    }                                                               \

#define DEF_LOGICAL(MANGLE, OPCODE)                                 \
    case TOK_##MANGLE:                                              \
    {                                                               \
        encode(&sect->buffer, {XOR, RDX, RDX});                     \
        encode(&sect->buffer, {CMP, RAX, RBX});                     \
        encode(&sect->buffer, {OPCODE, RDX, {}});                   \
        encode(&sect->buffer, {PUSH, RDX, {}});                     \
        break;                                                      \
    }                                                               \

#define DEF_UNIMPLEMENTED(MANGLE)    \
    case TOK_##MANGLE:               \

static generator_err oper(Section* sect, Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_OP);

    if(node->left && node->right)
    {
        encode(&sect->buffer, {POP, RBX, {}});
        encode(&sect->buffer, {POP, RAX, {}});

        switch(node->tok.val.op)
        {
            DEF_OPER(ADD, ADD)
            DEF_OPER(SUB, SUB)
            DEF_OPER(MUL, IMUL)
            case TOK_DIV:
            {
                encode(&sect->buffer, {CQO, {}, {}});
                encode(&sect->buffer, {IDIV, RBX, {}});
                encode(&sect->buffer, {PUSH, RAX, {}});
                break;
            }
            case TOK_OR:
            {
                encode(&sect->buffer, {XOR, RDX, RDX});
                encode(&sect->buffer, {OR, RAX, RBX});
                encode(&sect->buffer, {SETNE, RDX, {}});
                encode(&sect->buffer, {PUSH, RDX, {}});
                break;
            }
            case TOK_AND:
            {                
                encode(&sect->buffer, {TEST, RAX, RAX});
                encode(&sect->buffer, {SETNE, RAX, {}});
                encode(&sect->buffer, {XOR, RDX, RDX});
                encode(&sect->buffer, {TEST, RBX, RBX});
                encode(&sect->buffer, {SETNE, RDX});
                encode(&sect->buffer, {AND, RAX, RDX});
                encode(&sect->buffer, {PUSH, RAX, {}});
                break;
            }

            DEF_LOGICAL(EQ, SETE)
            DEF_LOGICAL(NEQ, SETNE)
            DEF_LOGICAL(GEQ, SETGE)
            DEF_LOGICAL(LEQ, SETLE)
            DEF_LOGICAL(GREAT, SETG)
            DEF_LOGICAL(LESS, SETL)

            DEF_UNIMPLEMENTED(POWER)
            default:
                format_error("Unknown or unimplemented operator", &node->tok);
        }
    }
    else if(!node->left && node->right && node->tok.val.op == TOK_NOT)
    {
        encode(&sect->buffer, {POP, RAX, {}});
        encode(&sect->buffer, {XOR, RDX, RDX});
        encode(&sect->buffer, {TEST, RAX, RAX});
        encode(&sect->buffer, {SETE, RDX, {}});
        encode(&sect->buffer, {PUSH, RDX, {}});
    }
    else
    {
        format_error("Invalid operator descendants combination", &node->tok);
    }

    return GENERATOR_NOERR;
}

#undef DEF_OPER
#undef DEF_LOGICAL
#undef DEF_UNIMPLEMENTED

static generator_err call_argument(Section* sect, Node* node, size_t n_args)
{
    assert(node);
    assert(node->tok.type == TYPE_AUX && node->tok.val.aux == TOK_PARAMETER);

    if((n_args > 1 && !node->left) || (n_args == 1 && node->left))
        semantic_error("Wrong amount of arguments", &node->tok);

    if(node->left)
        PASS$(!call_argument(sect, node->left, n_args - 1), return GENERATOR_PASS_ERROR; );

    if(!node->right)
        format_error("Missing argument", &node->right->tok);
    
    PASS$(!expression(sect, node->right), return GENERATOR_PASS_ERROR; );

    switch(n_args)
    {
        case 6:
            encode(&sect->buffer, {POP, R9, {}});
            break;
        case 5:
            encode(&sect->buffer, {POP, R8, {}});
            break;
        case 4:
            encode(&sect->buffer, {POP, RCX, {}});
            break;
        case 3:
            encode(&sect->buffer, {POP, RDX, {}});
            break;
        case 2:
            encode(&sect->buffer, {POP, RSI, {}});
            break;
        case 1:
            encode(&sect->buffer, {POP, RDI, {}});
            break;
        default:
            void(0);
    }

    return GENERATOR_NOERR;
}

static generator_err call(Section* sect, Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_AUX && node->tok.val.aux == TOK_CALL);

    if(node->left->tok.type != TYPE_ID)
        format_error("Left descendant of call is not function", &node->left->tok);

    Symbol   sym       = {};
    uint64_t sym_index = 0;
    if(symtable_find(SYMTABLE, node->left->tok.val.name, &sym, &sym_index) != 0)
        semantic_error("Function wasn't defined", &node->left->tok);

    if(sym.type != SYMBOL_TYPE_FUNCTION)
        semantic_error("Call of non-function object", &node->left->tok);

    if(strcmp(MAIN_STD_NAME, sym.id) == 0)
        semantic_error("'main' can't be called", &node->left->tok);
        
    if((sym.func.n_args == 0 && node->right) || (sym.func.n_args > 0 && !node->right))
        semantic_error("Wrong amount of arguments", &node->left->tok);
    
    if(node->right)
        PASS$(!call_argument(sect, node->right, sym.func.n_args), return GENERATOR_PASS_ERROR; );
    
    if(LOCALTABLE->offset_top % 16)
        encode(&sect->buffer, {SUB, RSP, IMM32(0x8)});
    
    encode(&sect->buffer, {XOR, RAX, RAX});
    encode(&sect->buffer, {CALL, IMM32(0x0), {}});

    Reloc reloc = {.dst_section_descriptor = sect->descriptor,
                   .dst_offset = sect->buffer.pos - sizeof(int32_t),
                   .dst_init_val = - (int32_t) sizeof(int32_t),
                   .src_nametable_index = sym_index,
                  };
    
    relocations_insert(RELOCATIONS, reloc);

    if(LOCALTABLE->offset_top % 16)
        encode(&sect->buffer, {ADD, RSP, IMM32(0x8)});

    if(sym.func.n_args > 6)
        encode(&sect->buffer, {ADD, RSP, IMM32(8 * (int32_t) (sym.func.n_args - 6))});

    encode(&sect->buffer, {PUSH, RAX, {}});
    
    return GENERATOR_NOERR;
}

static generator_err expression(Section* sect, Node* node)
{
    assert(node);

    if(node->tok.type == TYPE_NUMBER)
    {
        PASS$(!number(sect, node), return GENERATOR_PASS_ERROR; );
        return GENERATOR_NOERR;
    }
    
    if(node->tok.type == TYPE_ID)
    {
        PASS$(!variable(sect, node), return GENERATOR_PASS_ERROR; );
        return GENERATOR_NOERR;
    }

    if(node->tok.type == TYPE_EMBED)
    {
        format_error("Embedded functions are not supported in ELF compilator", &node->tok);
    }

    if(node->tok.type == TYPE_AUX && node->tok.val.aux == TOK_CALL)
    {
        PASS$(!call(sect, node), return GENERATOR_PASS_ERROR; );
        return GENERATOR_NOERR;
    }

    if(node->left)
    {
        PASS$(!expression(sect, node->left), return GENERATOR_PASS_ERROR; );
    }
    if(node->right)
    {
        PASS$(!expression(sect, node->right), return GENERATOR_PASS_ERROR; );
    }
    if(node->tok.type == TYPE_OP)
    {
        PASS$(!oper(sect, node), return GENERATOR_PASS_ERROR; );
        return GENERATOR_NOERR;
    }
        
    semantic_error("Token can't be used in expression", &node->tok);

    return GENERATOR_NOERR;
}

static generator_err conditional(Node* node)
{
    assert(node);
    if(!node->left || !node->right || node->right->tok.type != TYPE_AUX || node->right->tok.val.aux != TOK_DECISION)
        format_error("Conditional statement missing or wrong descendant", &node->tok);

    uint64_t false_offset     = 0;
    uint64_t end_offset       = 0;
    uint64_t jmp_false_offset = 0;
    uint64_t jmp_false_rip    = 0;
    uint64_t jmp_end_offset   = 0;
    uint64_t jmp_end_rip      = 0;

    PASS$(!expression(TEXT, node->left), return GENERATOR_PASS_ERROR; );
    
    encode(&TEXT->buffer, {POP, RAX, {}});
    encode(&TEXT->buffer, {TEST, RAX, RAX});

    jmp_false_offset = TEXT->buffer.pos;
    encode(&TEXT->buffer, {JE, IMM32(0xADDE), {}});
    jmp_false_rip = TEXT->buffer.pos;

    if(!node->right->left)
        semantic_error("Conditional statement missing positive branch (no body statements)", &node->tok);

    PASS$(!statement(node->right->left), return GENERATOR_PASS_ERROR; );

    jmp_end_offset = TEXT->buffer.pos;
    encode(&TEXT->buffer, {JMP, IMM32(0xADDE)});
    jmp_end_rip = TEXT->buffer.pos;

    false_offset = TEXT->buffer.pos;

    if(node->right->right)
        PASS$(!statement(node->right->right), return GENERATOR_PASS_ERROR; );

    end_offset = TEXT->buffer.pos;

    buffer_seek(&TEXT->buffer, jmp_false_offset);
    encode(&TEXT->buffer, {JE, IMM32((int32_t) (false_offset - jmp_false_rip)), {}});

    buffer_seek(&TEXT->buffer, jmp_end_offset);
    encode(&TEXT->buffer, {JMP, IMM32((int32_t) (end_offset - jmp_end_rip)), {}});

    buffer_rewind(&TEXT->buffer);

    return GENERATOR_NOERR;
}

static generator_err cycle(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_KEYWORD && node->tok.val.key == TOK_WHILE);

    if(!node->left || !node->right)
        format_error("Cycle statement missing or wrong descendant", &node->tok);

    uint64_t begin_offset     = 0;
    uint64_t cond_offset      = 0;
    uint64_t jmp_begin_offset = 0;
    uint64_t jmp_begin_rip    = 0;
    uint64_t jmp_cond_offset  = 0;
    uint64_t jmp_cond_rip     = 0;

    jmp_begin_offset = TEXT->buffer.pos;
    encode(&TEXT->buffer, {JMP, IMM32(0xADDE)});
    jmp_begin_rip    = TEXT->buffer.pos;

    begin_offset = TEXT->buffer.pos;

    PASS$(!statement(node->right), return GENERATOR_PASS_ERROR; );

    cond_offset = TEXT->buffer.pos;

    PASS$(!expression(TEXT, node->left), return GENERATOR_PASS_ERROR; );

    encode(&TEXT->buffer, {POP, RAX, {}});
    encode(&TEXT->buffer, {TEST, RAX, RAX});

    jmp_cond_offset = TEXT->buffer.pos;
    encode(&TEXT->buffer, {JNE, IMM32(0xADDE)});
    jmp_cond_rip    = TEXT->buffer.pos;

    buffer_seek(&TEXT->buffer, jmp_begin_offset);
    encode(&TEXT->buffer, {JMP, IMM32((int32_t) (cond_offset - jmp_begin_rip)), {}});

    buffer_seek(&TEXT->buffer, jmp_cond_offset);
    encode(&TEXT->buffer, {JNE, IMM32((int32_t) (begin_offset - jmp_cond_rip)), {}});

    buffer_rewind(&TEXT->buffer);

    return GENERATOR_NOERR;
}

static generator_err terminational(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_KEYWORD && node->tok.val.key == TOK_RETURN);

    if(node->left || !node->right)
        format_error("Terminational statement missing or wrong descendant", &node->tok);

    PASS$(!expression(TEXT, node->right), return GENERATOR_PASS_ERROR; );

    encode(&TEXT->buffer, {POP, RAX, {}});

    encode(&TEXT->buffer, {MOV, RSP, RBP});
    encode(&TEXT->buffer, {POP, RBP, {}});
    encode(&TEXT->buffer, {RET, {},  {}});

    return GENERATOR_NOERR;
}

static generator_err assignment(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_OP && node->tok.val.op == TOK_ASSIGN);

    LOG$("Entered function");
    
    if(!node->left)
        format_error("Assignment requires lvalue", &node->tok);

    if(node->left->tok.type != TYPE_ID)
        format_error("Assignment requires identifier as lvalue", &node->left->tok);

    bool is_const = false;
    if(node->left->left)
    {
        if(node->left->left->tok.type != TYPE_KEYWORD || node->left->left->tok.val.key != TOK_CONST)
            format_error("Variable has wrong left descendant ('const' expected)", &node->left->left->tok);

        is_const = true;          
    }
    LOG$("Passed checks");
    
    Symbol    sym = {};
    uint64_t  sym_index = 0;
    Local_var var = {};
    bool      is_global = false;

    if(symtable_find(SYMTABLE, node->left->tok.val.name, &sym, &sym_index) == 0 && sym.type == SYMBOL_TYPE_VARIABLE)
    {
        LOG$("Global variable found");
        is_global = true;
    }
    else if(localtable_find(LOCALTABLE, node->left->tok.val.name, &var) == 0)
    {
        LOG$("Local variable found");
        is_global = false;
    }
    else
    {
        LOG$("Declaring new local variable");

        if(is_global && sym.type == SYMBOL_TYPE_FUNCTION)
            semantic_error("Cannot declare variable with function name", &node->left->tok);
        
        int32_t shift = 0;
        if(node->left->right)
        {
            if(node->left->right->tok.type != TYPE_NUMBER)
                semantic_error("Size of variable is not compile-time evaluatable", &node->left->tok);

            shift = (int32_t) node->left->right->tok.val.num;
        }

        if(shift < 0)
            semantic_error("Size of variable is negative", &node->left->tok);

        var = {.id = node->left->tok.val.name,
               .size = (size_t) shift + 1,
               .is_const = is_const
              };
    
        encode(&TEXT->buffer, {SUB, RSP, IMM32((int32_t) var.size * 8)});
        localtable_allocate(LOCALTABLE, &var);

        PASS$(!expression(TEXT, node->right), return GENERATOR_PASS_ERROR; );
        encode(&TEXT->buffer, {POP, MEM(0x0, {}, RBP, var.offset + shift * 8), {}});

        return GENERATOR_NOERR;
    }

    if(is_const)
        semantic_error("'const' specifier in assignment to declared variable", &node->left->tok);

    if((is_global && sym.var.is_const) || (!is_global && var.is_const))
        semantic_error("Assignment to 'const' variable", &node->left->tok);

    PASS$(!expression(TEXT, node->right), return GENERATOR_PASS_ERROR; );

    Operand index_reg = {};
    uint8_t scale     = 0;

    if(node->left->right)
    {
        PASS$(!expression(TEXT, node->left->right), return GENERATOR_PASS_ERROR; );
        encode(&TEXT->buffer, {POP, RCX, {}});

        index_reg = RCX;
        scale     = 8;
    }

    if(is_global)
    {
        encode(&TEXT->buffer, {LEA, RAX, MEM(0, {}, {}, 0x0)});

        Reloc reloc = {.dst_section_descriptor = TEXT->descriptor,
                       .dst_offset = TEXT->buffer.pos - sizeof(int32_t),
                       .dst_init_val = - (int32_t) sizeof(int32_t),
                       .src_nametable_index = sym_index,
                      };
        
        relocations_insert(RELOCATIONS, reloc);

        encode(&TEXT->buffer, {POP, MEM(scale, index_reg, RAX, 0), {}});
    }
    else
    {
        encode(&TEXT->buffer, {POP, MEM(scale, index_reg, RBP, var.offset), {}}); 
    }

    return GENERATOR_NOERR;
}

static generator_err statement(Node* node)
{
    assert(node);

    if(node->tok.type != TYPE_AUX || node->tok.val.aux != TOK_STATEMENT)
        format_error("'statement' expected", &node->tok);

    if(node->left)
        PASS$(!statement(node->left), return GENERATOR_PASS_ERROR; );

    if(node->right->tok.type == TYPE_KEYWORD)
    {
        if(node->right->tok.val.key == TOK_IF)
        {
            PASS$(!conditional(node->right), return GENERATOR_PASS_ERROR; );
            return GENERATOR_NOERR;
        }
        else if(node->right->tok.val.key == TOK_WHILE)
        {
            PASS$(!cycle(node->right), return GENERATOR_PASS_ERROR; );
            return GENERATOR_NOERR;
        }
        else if(node->right->tok.val.key == TOK_RETURN)
        {
            PASS$(!terminational(node->right), return GENERATOR_PASS_ERROR; );
            return GENERATOR_NOERR;
        }
    }

    if(node->right->tok.type == TYPE_OP && node->right->tok.val.op == TOK_ASSIGN)
    {
        PASS$(!assignment(node->right), return GENERATOR_PASS_ERROR; );
        return GENERATOR_NOERR;
    }

    // plain expression, remove result from stack
    PASS$(!expression(TEXT, node->right), return GENERATOR_PASS_ERROR; );
    encode(&TEXT->buffer, {POP, RAX, {}});

    return GENERATOR_NOERR;
}

static generator_err parameter(Node* node, size_t n_params)
{
    assert(node);
    
    if(node->tok.type != TYPE_AUX || node->tok.val.aux != TOK_PARAMETER)
        format_error("'parameter' expected", &node->tok);

    if(node->left)
        PASS$(!parameter(node->left, n_params - 1), return GENERATOR_PASS_ERROR; );
    
    if(node->right->tok.type != TYPE_ID)
        format_error("Parameter is not id", &node->right->tok);

    Local_var param = {.id = node->right->tok.val.name};

    if(node->right->right)
        format_error("Parameter cannot be array", &node->right->tok);
    
    param.size = 1;

    if(node->right->left)
    {
        if(node->right->left->tok.type != TYPE_KEYWORD || node->right->left->tok.val.key != TOK_CONST)
            format_error("Variable has wrong left descendant ('const' expected)", &node->right->tok);
        
        param.is_const = true;
    }

    if(symtable_find(SYMTABLE, param.id) == 0 || localtable_find(LOCALTABLE, param.id) == 0)
        semantic_error("Variable redeclaration", &node->right->tok);

    switch(n_params)
    {
        case 6:
            encode(&TEXT->buffer, {PUSH, R9, {}});
            break;
        case 5:
            encode(&TEXT->buffer, {PUSH, R8, {}});
            break;
        case 4:
            encode(&TEXT->buffer, {PUSH, RCX, {}});
            break;
        case 3:
            encode(&TEXT->buffer, {PUSH, RDX, {}});
            break;
        case 2:
            encode(&TEXT->buffer, {PUSH, RSI, {}});
            break;
        case 1:
            encode(&TEXT->buffer, {PUSH, RDI, {}});
            break;
        default:
            PASS$(!localtable_set_parameter(LOCALTABLE, &param), return GENERATOR_PASS_ERROR; );
            return GENERATOR_NOERR;
    }

    PASS$(!localtable_allocate(LOCALTABLE, &param), return GENERATOR_PASS_ERROR; );

    return GENERATOR_NOERR;
}

static generator_err collect_stdlib_functions(Dependencies* deps)
{
    assert(deps);

    for(size_t iter = 0; iter < deps->buffer_sz; iter++)
    {
        Dep dep = deps->buffer[iter];

        Symbol sym = {.type = SYMBOL_TYPE_FUNCTION,
                      .id = dep.func.val.name,
                      .func = (Function) {.n_args = dep.n_args}
                     };

        ASSERT$(!symtable_insert(SYMTABLE, sym), GENERATOR_PASS_ERROR, return GENERATOR_PASS_ERROR; );
    }

    return GENERATOR_NOERR;
}

static generator_err collect_functions(Node* node)
{
    assert(node);

    if(node->left)
        PASS$(!collect_functions(node->left), return GENERATOR_PASS_ERROR; );
    
    if(node->tok.type != TYPE_AUX || node->tok.val.aux != TOK_STATEMENT)
        format_error("'statement' expected (first line)", &node->tok);
    
    if(!node->right)
        format_error("Missing 'statement' body (first line)", &node->tok);

    if(node->right->tok.type == TYPE_OP && node->right->tok.val.op == TOK_ASSIGN)
        return GENERATOR_NOERR;

    if(node->right->tok.type != TYPE_AUX || node->right->tok.val.aux != TOK_DEFINE)
        format_error("'define' expected", &node->right->tok);
    
    if(!node->right->left)
        format_error("'define' missing 'function'", &node->right->tok);

    if(node->right->left->tok.type != TYPE_AUX || node->right->left->tok.val.aux != TOK_FUNCTION)
        format_error("'function' expected", &node->right->left->tok);

    Symbol sym = {.type = SYMBOL_TYPE_FUNCTION};

    Node* ptr = node->right->left->right;

    while(ptr)
    {
        if(ptr->tok.type != TYPE_AUX || ptr->tok.val.aux != TOK_PARAMETER)
            format_error("'parameter' expected", &ptr->tok);
        
        sym.func.n_args++;

        ptr = ptr->left;
    }

    ptr = node->right->left->left;
    if(!ptr)
        format_error("Function name is missing", &node->right->left->tok);

    if(ptr->tok.type != TYPE_ID)
        format_error("Function name is not identifier", &ptr->tok);
    
    sym.id = ptr->tok.val.name;

    if(symtable_find(SYMTABLE, sym.id) == 0)
        semantic_error("Function redefinition", &ptr->tok);

    PASS$(!symtable_insert(SYMTABLE, sym), return GENERATOR_PASS_ERROR; );
        
    return GENERATOR_NOERR;
}

static generator_err generate_funtions(Node* node)
{
    assert(node);

    if(node->left)
        PASS$(!generate_funtions(node->left), return GENERATOR_PASS_ERROR; );
    
    if(node->right->tok.type != TYPE_AUX || node->right->tok.val.aux != TOK_DEFINE)
        return GENERATOR_NOERR;
    
    localtable_clean(LOCALTABLE);

    char* func_name = node->right->left->left->tok.val.name;

    Symbol   sym = {};
    uint64_t sym_index = {};
    assert(!symtable_find(SYMTABLE, func_name, &sym, &sym_index));

    Symbol* ptr = &SYMTABLE->buffer[sym_index];
    ptr->offset             = TEXT->buffer.pos;
    ptr->section_descriptor = TEXT->descriptor;

    encode(&TEXT->buffer, {PUSH, RBP, {}});
    encode(&TEXT->buffer, {MOV, RBP, RSP});

    size_t n_param = sym.func.n_args;
    Node* param = node->right->left->right;
    if(param)
        PASS$(!parameter(param, n_param), return GENERATOR_PASS_ERROR; );

    Node* stmnt = node->right->right;
    if(stmnt)
        PASS$(!statement(stmnt), return GENERATOR_PASS_ERROR; );

    ptr->s_size = TEXT->buffer.pos - ptr->offset;

    if(stmnt->right->tok.type != TYPE_KEYWORD || stmnt->right->tok.val.key != TOK_RETURN)
        semantic_error("Missing terminational", &node->right->left->left->tok);

    MSG$("Function `%s` local variables:", node->right->left->left->tok.val.name);
    localtable_dump(LOCALTABLE);

    return GENERATOR_NOERR;
}

static generator_err declare_global(Node* node)
{
    if(!node->left)
        format_error("Assignment requires lvalue", &node->tok);

    if(node->left->tok.type != TYPE_ID)
        format_error("Assignment requires identifier as lvalue", &node->left->tok);

    // PASS$(!expression(INIT, node->right), return GENERATOR_PASS_ERROR; );

    if(!node->right)
        semantic_error("Global variable is not initialized", &node->left->  tok);

    if(node->right->tok.type != TYPE_NUMBER)
        semantic_error("Global variable is not compile-time evaluatable", &node->left->tok);

    uint64_t value = (uint64_t) node->right->tok.val.num;

    bool is_const = false;
    if(node->left->left)
    {
        if(node->left->left->tok.type != TYPE_KEYWORD || node->left->left->tok.val.key != TOK_CONST)
            format_error("Variable has wrong left descendant ('const' expected)", &node->left->left->tok);

        is_const = true;          
    }

    Symbol sym = {};

    if(symtable_find(SYMTABLE, node->left->tok.val.name, &sym) == 0)
    {
        if(sym.type == SYMBOL_TYPE_FUNCTION)
            semantic_error("Cannot declare variable with function name", &node->left->tok);
        else
            semantic_error("Global variable redeclaration", &node->left->tok);
    }

    uint64_t shift = 0;
    if(node->left->right)
    {
        if(node->left->right->tok.type != TYPE_NUMBER)
            semantic_error("Size of variable is not compile-time evaluatable", &node->left->tok);

        if(node->left->right->tok.val.num < 0)
            semantic_error("Size of variable is negative", &node->left->tok);

        shift = (uint64_t) node->left->right->tok.val.num;
    }

    sym = {.type               = SYMBOL_TYPE_VARIABLE,
           .id                 = node->left->tok.val.name,
           .offset             = DATA->buffer.pos,
           .section_descriptor = DATA->descriptor,
           .var  = (Variable) {.is_const = is_const, .size = shift + 1}
          };
    
    uint64_t sym_index = 0;
    symtable_insert(SYMTABLE, sym, &sym_index);

    for(size_t iter = 0; iter < shift; iter++)
    {
        buffer_append_u64(&DATA->buffer, 0x0);
    }
    buffer_append_u64(&DATA->buffer, value);

    // encode(&INIT->buffer, {POP, MEM(0x0, {}, {}, 0x0), {}});
    
    // Reloc reloc = {.dst_section_descriptor = INIT->descriptor,
    //                .dst_offset = INIT->buffer.pos - sizeof(int32_t),
    //                .dst_init_val = shift - (int32_t) sizeof(int32_t),
    //                .src_nametable_index = sym_index,
    //               };
        
    // relocations_insert(RELOCATIONS, reloc);

    return GENERATOR_NOERR;
}

static generator_err generate_globals(Node* node)
{
    assert(node);

    if(node->left)
        PASS$(!generate_globals(node->left), return GENERATOR_PASS_ERROR; );
    
    if(node->tok.type != TYPE_AUX || node->tok.val.aux != TOK_STATEMENT)
        format_error("'statement' expected (first line)", &node->tok);

    if(!node->right)
        format_error("Missing 'statement' body (first line)", &node->tok);

    if(node->right->tok.type == TYPE_AUX && node->right->tok.val.aux == TOK_DEFINE)
        return GENERATOR_NOERR;

    if(node->right->tok.type != TYPE_OP || node->right->tok.val.op != TOK_ASSIGN)
        format_error("'=' expected", &node->right->tok);

    PASS$(!declare_global(node->right), return GENERATOR_PASS_ERROR; );

    return GENERATOR_NOERR;
}

generator_err generator(Tree* tree, Dependencies* deps, Binary* bin)
{
    assert(bin && tree);

    symtable_dump_init(logs_get());

    Section null = {(Elf64_Shdr)
                    {
                        .sh_type  = SHT_NULL,
                    },
                    .name = "",
                   };

    Section text = {(Elf64_Shdr)
                    {
                        .sh_type  = SHT_PROGBITS,
                        .sh_flags = SHF_EXECINSTR | SHF_ALLOC,
                        .sh_addralign = 16,
                    },
                    .name = ".text",
                   };

    Section relatbl = {(Elf64_Shdr)
                       {
                            .sh_type  = SHT_RELA,
                            .sh_flags = SHF_INFO_LINK,
                            .sh_addralign = 8,
                            .sh_entsize = sizeof(Elf64_Rela),
                       },
                       .name = ".rela.text",
                      };
    
    // Section init = {(Elf64_Shdr)
    //                 {
    //                     .sh_type  = SHT_PROGBITS,
    //                     .sh_flags = SHF_EXECINSTR | SHF_ALLOC,
    //                     .sh_addralign = 16,
    //                 },
    //                 .name = ".init",
    //                };

    Section data = {(Elf64_Shdr)
                    {
                        .sh_type  = SHT_PROGBITS,
                        .sh_flags = SHF_WRITE | SHF_ALLOC,
                        .sh_addralign = 8,
                    },
                    .name = ".data",
                   };

    Section symtab = {(Elf64_Shdr)
                      {
                          .sh_type  = SHT_SYMTAB,
                          .sh_flags = 0x0,
                          .sh_addralign = 8,
                          .sh_entsize = sizeof(Elf64_Sym),
                      },
                      .name = ".symtab",
                     };

    Section strtab = {(Elf64_Shdr)
                      {
                          .sh_type  = SHT_STRTAB,
                          .sh_flags = 0x0,
                          .sh_addralign = 1,
                      },
                      .name = ".strtab",
                     };

    binary_reserve_section(bin, &null);
    binary_reserve_section(bin, &text);
    binary_reserve_section(bin, &relatbl);
    // binary_reserve_section(bin, &init);
    binary_reserve_section(bin, &data);
    binary_reserve_section(bin, &symtab);
    binary_reserve_section(bin, &strtab);

    relatbl.shdr.sh_info = (Elf64_Word) text.descriptor;
    relatbl.shdr.sh_link = (Elf64_Word) symtab.descriptor;

    symtab.shdr.sh_info  = 0; // # of local symbols
    symtab.shdr.sh_link  = (Elf64_Word) strtab.descriptor;

    Symtable    symbols = {};
    Localtable  locals  = {};
    Relocations relocs  = {};
    symtable_ctor   (&symbols);
    localtable_ctor (&locals);
    relocations_ctor(&relocs);

    Symbol null_sym = {.id = ""};
    symtable_insert(&symbols, null_sym);

    RELOCATIONS = &relocs;
    SYMTABLE    = &symbols;
    LOCALTABLE  = &locals;

    TEXT = &text;
    DATA = &data;
    // INIT = &init;

    PASS$(!collect_stdlib_functions(deps), return GENERATOR_PASS_ERROR; );

    collect_functions(tree->root);

    generate_globals(tree->root);

    generate_funtions(tree->root);

    symtable_dump(&symbols);

    binary_store_section(bin, text);
    // binary_store_section(bin, init);
    binary_store_section(bin, data);

    binary_generate_rela(&relatbl, &symbols, &relocs);

    binary_generate_strtab(&strtab, &symbols);
    binary_generate_symtab(&symtab, &symbols);

    symtable_dump(&symbols);

    binary_store_section(bin, relatbl);
    binary_store_section(bin, strtab);
    binary_store_section(bin, symtab);

    binary_generate_shstrtab(bin);

    binary_reserve_hdrs(bin);

    binary_arrange_sections(bin);

    binary_generate_shdrs(bin);
    binary_generate_ehdr(bin);

    SYMTABLE   = nullptr;
    LOCALTABLE = nullptr;

    symtable_dtor   (&symbols);
    localtable_dtor (&locals);
    relocations_dtor(&relocs);

    return IS_ERROR;
}

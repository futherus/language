#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "generator_elf.h"
#include "symtable.h"
#include "encode.h"
#include "elf.h"
#include "relocation.h"
#include "../../include/logs/logs.h"
//#include "hash.h"
#include "../reserved_names.h"

static Symtable*    SYMTABLE    = nullptr;
static Localtable*  LOCALTABLE  = nullptr;
static Relocations* RELOCATIONS = nullptr;

static Section* DATA = nullptr;
static Section* INIT = nullptr;
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
                       .dst_init_val = 0,
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

/*
static generator_err embedded(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_EMBED);


    switch(node->tok.val.emb)
    {
        case TOK_SIN:
        {
            if(!node->right || node->left)
                format_error("Embedded 'sin' requires 1 argument", &node->tok);

            PASS$(!expression(node->right), return GENERATOR_PASS_ERROR; );

            print_tab("pop rex\n");
            print_tab("sin rex\n");
            break;
        }
        case TOK_COS:
        {
            if(!node->right || node->left)
                format_error("Embedded 'cos' requires 1 argument", &node->tok);

            PASS$(!expression(node->right), return GENERATOR_PASS_ERROR; );

            print_tab("pop rex\n");
            print_tab("cos rex\n");
            break;
        }
        case TOK_PRINT:
        {
            if(!node->right)
                format_error("Embedded 'print' requires 1 argument", &node->tok);

            PASS$(!expression(node->right), return GENERATOR_PASS_ERROR; );

            print_tab("out\n");

            print_tab("push 0\n");
            break;
        }
        case TOK_SCAN:
        {
            if(node->right || node->left)
                format_error("Embedded 'scan' cannot has arguments", &node->tok);

            print_tab("in\n");
            break;
        }
        case TOK_SHOW:
        {
            if(!node->right || !node->left)
                format_error("Embedded 'show' requires 2 arguments", &node->tok);
            
            print("\n");

            if(node->left->tok.type != TYPE_ID)
                format_error("Embedded 'show' requires variable as first argument", &node->left->tok);

            Variable* var = {};

            if(node->left->right)
            {
                PASS$(!expression(node->left->right), return GENERATOR_PASS_ERROR; );
                
                print_tab("pop rex\n");
                print_tab("push rex + ");
            }
            else
            {
                print_tab("push ");
            }

            if((var = vartable_find(GLOBALS, node->left->tok.val.name)) != nullptr)
            {
                print("rcx + %ld\n", var->offset);
            }
            else if((var = vartable_find(LOCALS, node->left->tok.val.name)) != nullptr)
            {
                print("rbx + %ld\n", var->offset);
            }
            else
            {
                semantic_error("Variable wasn't declared", &node->tok);
            }

            PASS$(!expression(node->right), return GENERATOR_PASS_ERROR; );

            print_tab("pop rfx\n");
            print_tab("pop rex\n");

            print_tab("show rex, rfx\n");

            print_tab("push 0\n\n");
            break;
        }
        case TOK_INT:
        {
            if(!node->right || node->left)
                format_error("Embedded 'int' requires 1 argument", &node->tok);

            PASS$(!expression(node->right), return GENERATOR_PASS_ERROR; );

            print_tab("pop rex\n");
            print_tab("int rex\n");
            break;
        }
        case TOK_SQRT:
        {
            if(!node->right || node->left)
                format_error("Embedded 'sqrt' requires 1 argument", &node->tok);

            PASS$(!expression(node->right), return GENERATOR_PASS_ERROR; );

            print_tab("pop rex\n");
            print_tab("sqrt rex\n");
            break;
        }
        default:
        {
            assert(0);
        }
    }

    return GENERATOR_NOERR;
}
*/

/*
#define PRINT_CMD(MANGLE, TXT)                                        \
    if(node->tok.val.op == TOK_##MANGLE)                              \
    {                                                                 \
        if(!node->left || !node->right)                               \
            format_error("Operator wrong descendants", &node->tok);   \
        print_tab("%s\n", (TXT));                                     \
    }                                                                 \
    else                                                              \
*/

static generator_err oper(Section* sect, Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_OP);

    encode(&sect->buffer, {POP, RBX, {}});
    encode(&sect->buffer, {POP, RAX, {}});

    if(node->tok.val.op == TOK_ADD)
    {
        if(!node->left || !node->right)
            format_error("Operator wrong descendants", &node->tok);
        
        encode(&sect->buffer, {ADD, RAX, RBX});
    }
    else if(node->tok.val.op == TOK_SUB)
    {
        if(!node->left || !node->right)
            format_error("Operator wrong descendants", &node->tok);

        encode(&sect->buffer, {SUB, RAX, RBX});
    }
    else
    {
        format_error("Unknown operator", &node->tok);
    }
    
    encode(&sect->buffer, {PUSH, RAX, {}});

    // if(node->tok.val.op == TOK_NOT)
    // {
    //     if(node->left || !node->right)
    //         format_error("Operator wrong descendants", &node->tok);
    //     print_tab("push 0\n");
    //     print_tab("eq\n");
    // }
    // else
    // PRINT_CMD(ADD,   "add")
    // PRINT_CMD(SUB,   "sub")
    // PRINT_CMD(MUL,   "mul")
    // PRINT_CMD(DIV,   "div")
    // PRINT_CMD(POWER, "pow")
    // PRINT_CMD(EQ,    "eq")
    // PRINT_CMD(NEQ,   "neq")
    // PRINT_CMD(GREAT, "gr")
    // PRINT_CMD(LESS,  "le")
    // PRINT_CMD(LEQ,   "leq")
    // PRINT_CMD(GEQ,   "geq")
    // PRINT_CMD(AND,   "and")
    // PRINT_CMD(OR,    "or")
    // /* else */
    //     format_error("Unknown operator", &node->tok);

    return GENERATOR_NOERR;
}
// #undef PRINT_CMD

static generator_err call_argument(Section* sect, Node* node, size_t n_args)
{
    assert(node);
    assert(node->tok.type == TYPE_AUX && node->tok.val.aux == TOK_PARAMETER);

    n_args--;

    if((n_args > 0 && !node->left) || (n_args == 0 && node->left))
        semantic_error("Wrong amount of arguments", &node->tok);

    if(node->left)
        PASS$(!call_argument(sect, node->left, n_args), return GENERATOR_PASS_ERROR; );

    if(!node->right)
        format_error("Missing argument", &node->right->tok);
    
    Local_var arg = {};
    localtable_allocate_argument(LOCALTABLE, &arg);

    PASS$(!expression(sect, node->right), return GENERATOR_PASS_ERROR; );

    encode(&sect->buffer, {POP, MEM(0, {}, RBP, arg.offset), {}});

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
    
    if(sym.func.n_args)
        encode(&sect->buffer, {SUB, RSP, IMM32(8 * (int32_t) sym.func.n_args)});

    if(node->right)
        PASS$(!call_argument(sect, node->right, sym.func.n_args), return GENERATOR_PASS_ERROR; );
    
    encode(&sect->buffer, {CALL, IMM32(0x0), {}});

    Reloc reloc = {.dst_section_descriptor = sect->descriptor,
                   .dst_offset = sect->buffer.pos - sizeof(int32_t),
                   .dst_init_val = 0,
                   .src_nametable_index = sym_index
                  };
    
    relocations_insert(RELOCATIONS, reloc);

    localtable_deallocate(LOCALTABLE, sym.func.n_args);
    encode(&sect->buffer, {ADD, RSP, IMM32(8 * (int32_t) sym.func.n_args)});

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

    // if(node->tok.type == TYPE_EMBED)
    // {
    //     PASS$(!embedded(node), return GENERATOR_PASS_ERROR; );
    //     return GENERATOR_NOERR;
    // }

    if(node->tok.type == TYPE_AUX && node->tok.val.aux == TOK_CALL)
    {
        PASS$(!call(sect, node), return GENERATOR_PASS_ERROR; );
        return GENERATOR_NOERR;
    }

    if(node->left)
        PASS$(!expression(sect, node->left), return GENERATOR_PASS_ERROR; );

    if(node->right)
        PASS$(!expression(sect, node->right), return GENERATOR_PASS_ERROR; );
        
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
    encode(&TEXT->buffer, {CMP, RAX, IMM32(0)});

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
    encode(&TEXT->buffer, {CMP, RAX, IMM32(0)});

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
                       .dst_init_val = 0,
                       .src_nametable_index = sym_index
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

static generator_err parameter(Node* node)
{
    assert(node);
    
    if(node->tok.type != TYPE_AUX || node->tok.val.aux != TOK_PARAMETER)
        format_error("'parameter' expected", &node->tok);

    if(node->left)
        PASS$(!parameter(node->left), return GENERATOR_PASS_ERROR; );
    
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

    PASS$(!localtable_set_parameter(LOCALTABLE, &param), return GENERATOR_PASS_ERROR; );

    return GENERATOR_NOERR;
}

static generator_err fill_funcs_table(Node* node)
{
    assert(node);

    if(node->left)
        PASS$(!fill_funcs_table(node->left), return GENERATOR_PASS_ERROR; );
    
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

static generator_err generate_funcs(Node* node)
{
    assert(node);

    if(node->left)
        PASS$(!generate_funcs(node->left), return GENERATOR_PASS_ERROR; );
    
    if(node->right->tok.type != TYPE_AUX || node->right->tok.val.aux != TOK_DEFINE)
        return GENERATOR_NOERR;
    
    localtable_clean(LOCALTABLE);

    char* func_name = node->right->left->left->tok.val.name;

    Symbol   sym = {};
    uint64_t sym_index = {};
    assert(!symtable_find(SYMTABLE, func_name, &sym, &sym_index));

    SYMTABLE->buffer[sym_index].offset             = TEXT->buffer.pos;
    SYMTABLE->buffer[sym_index].section_descriptor = TEXT->descriptor;

    encode(&TEXT->buffer, {PUSH, RBP, {}});
    encode(&TEXT->buffer, {MOV, RBP, RSP});

    Node* param = node->right->left->right;
    if(param)
        PASS$(!parameter(param), return GENERATOR_PASS_ERROR; );

    Node* stmnt = node->right->right;
    if(stmnt)
        PASS$(!statement(stmnt), return GENERATOR_PASS_ERROR; );

    if(stmnt->right->tok.type != TYPE_KEYWORD || stmnt->right->tok.val.key != TOK_RETURN)
        semantic_error("Missing terminational", &node->right->left->left->tok);

    MSG$("Function `%s` local variables:", node->right->left->left->tok.val.name);
    localtable_dump(LOCALTABLE);

    return GENERATOR_NOERR;
}

static generator_err declare_global(Node* node)
{
    PASS$(!expression(INIT, node->right), return GENERATOR_PASS_ERROR; );

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

    Symbol sym = {};

    if(symtable_find(SYMTABLE, node->left->tok.val.name, &sym) == 0)
    {
        if(sym.type == SYMBOL_TYPE_FUNCTION)
            semantic_error("Cannot declare variable with function name", &node->left->tok);
        else
            semantic_error("Global variable redeclaration", &node->left->tok);
    }

    int32_t shift = 0;
    if(node->left->right)
    {
        if(node->left->right->tok.type != TYPE_NUMBER)
            semantic_error("Size of variable is not compile-time evaluatable", &node->left->tok);

        shift = (int32_t) node->left->right->tok.val.num;
    }

    sym = {.type               = SYMBOL_TYPE_VARIABLE,
           .id                 = node->left->tok.val.name,
           .offset             = DATA->buffer.pos,
           .section_descriptor = DATA->descriptor,
           .var  = (Variable) {.is_const = is_const, .size = (size_t) shift + 1}
          };
    
    uint64_t sym_index = 0;
    symtable_insert(SYMTABLE, sym, &sym_index);
    buffer_append_u64(&DATA->buffer, 0x0);

    encode(&INIT->buffer, {POP, MEM(0x0, {}, {}, 0x0), {}});
    
    Reloc reloc = {.dst_section_descriptor = INIT->descriptor,
                   .dst_offset = INIT->buffer.pos - sizeof(int32_t),
                   .dst_init_val = shift,
                   .src_nametable_index = sym_index
                  };
        
    relocations_insert(RELOCATIONS, reloc);

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

generator_err generator(Tree* tree, Binary* bin)
{
    assert(bin && tree);

    symtable_dump_init(logs_get());

    Section null = {.needs_phdr = false,
                    .needs_shdr = true,
                    .stype  = SHT_NULL,
                    .name = ""
                   };

    Section text = {.needs_phdr = true,
                    .needs_shdr = true,
                    .ptype  = PT_LOAD,
                    .pflags = PF_EXECUTE | PF_READ,
                    .stype  = SHT_PROGBITS,
                    .sflags = SHF_EXECINSTR | SHF_ALLOC,
                    .salign = 16,
                    .name = ".text",
                   };

    Section init = {.needs_phdr = true,
                    .needs_shdr = true,
                    .ptype  = PT_LOAD,
                    .pflags = PF_EXECUTE | PF_READ,
                    .stype  = SHT_PROGBITS,
                    .sflags = SHF_EXECINSTR | SHF_ALLOC,
                    .salign = 16,
                    .name = ".init",
                   };

    Section data = {.needs_phdr = true,
                    .needs_shdr = true,
                    .ptype  = PT_LOAD,
                    .pflags = PF_WRITE | PF_READ,
                    .stype  = SHT_PROGBITS,
                    .sflags = SHF_WRITE | SHF_ALLOC,
                    .salign = 8,
                    .name = ".data",
                   };

    binary_reserve_section(bin, &null);
    binary_reserve_section(bin, &text);
    binary_reserve_section(bin, &init);
    binary_reserve_section(bin, &data);

    Symtable    symbols = {};
    Localtable  locals  = {};
    Relocations relocs  = {};
    symtable_ctor   (&symbols);
    localtable_ctor (&locals);
    relocations_ctor(&relocs);

    RELOCATIONS = &relocs;
    SYMTABLE    = &symbols;
    LOCALTABLE  = &locals;

    TEXT = &text;
    DATA = &data;
    INIT = &init;

    fill_funcs_table(tree->root);

    generate_globals(tree->root);

    generate_funcs(tree->root);

    // PASS$(!fill_funcs_table(tree->root), return GENERATOR_PASS_ERROR; );
    // MSG$("Functions:");
    // functable_dump(&funcs);

    // print_tab("push %ld\n"
    //           "pop rcx\n",
    //            MEMORY_GLOBAL);

    // PASS$(!generate_globals(tree->root), return GENERATOR_PASS_ERROR; );
    // MSG$("Global variables:");
    // vartable_dump(&globals);

    // print_tab("push %ld\n"
    //           "pop rbx\n",
    //           vartable_end(&globals));

    // print_tab("call func__%lx\n", fnv1_64(MAIN_STD_NAME, sizeof(MAIN_STD_NAME) - 1));

    // print_tab("hlt\n");

    // PASS$(!generate_funcs(tree->root), return GENERATOR_PASS_ERROR; );

    binary_store_section(bin, text);
    binary_store_section(bin, init);
    binary_store_section(bin, data);

    binary_generate_shstrtab(bin);

    binary_reserve_hdrs(bin);

    binary_arrange_sections(bin);

    binary_generate_phdrs(bin);
    binary_generate_shdrs(bin);
    binary_generate_ehdr(bin);

    LOG$("Before relocations");
    relocations_resolve(&relocs, &symbols, bin);
    LOG$("After relocations");

    SYMTABLE   = nullptr;
    LOCALTABLE = nullptr;

    symtable_dump(&symbols);

    symtable_dtor   (&symbols);
    localtable_dtor (&locals);
    relocations_dtor(&relocs);

    return IS_ERROR;
}

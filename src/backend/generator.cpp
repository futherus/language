#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "generator.h"
#include "nametable/nametable.h"
#include "../dumpsystem/dumpsystem.h"
#include "hash.h"
#include "../reserved_names.h"

/*
    rax -- return value
    rbx -- memory stack pointer
    rex -- temporary
    rkx -- trash register
*/

static Function_table* FUNCS   = nullptr;
static Variable_table* LOCALS  = nullptr;
static Variable_table* GLOBALS = nullptr;

static FILE* OSTREAM = nullptr;

static generator_err IS_ERROR = GENERATOR_NOERR;

#define semantic_error(MSG_, TOK_)                                                      \
do                                                                                      \
{                                                                                       \
    IS_ERROR = GENERATOR_SEMANTIC_ERROR;                                                \
    fprintf(stderr, "\x1b[31mSemantic error:\x1b[0m %s : %s\n", (MSG_), std_demangle(TOK_));\
    FILE* stream_ = dumpsystem_get_opened_stream();                                     \
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
    FILE* stream_ = dumpsystem_get_opened_stream();                                     \
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

static int INDENTATION = 0;

#define print_tab(fmt, ...) fprintf(OSTREAM, "%*s" fmt, INDENTATION * 4, "", ##__VA_ARGS__)
#define print(fmt, ...)     fprintf(OSTREAM, fmt, ##__VA_ARGS__)

static generator_err expression(Node* node);
static generator_err statement(Node* node);

static generator_err number(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_NUMBER);

    if(node->left || node->right)
        format_error("Number has descendants", &node->tok);

    print_tab("push %lg\n", node->tok.val.num);

    return GENERATOR_NOERR;
}

static generator_err variable(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_ID);

    if(node->left)
        semantic_error("Variable has 'const' specifier in expression", &node->tok);
    
    Variable* var = 0;

    if(node->right)
    {
        PASS$(!expression(node->right), return GENERATOR_PASS_ERROR; );
        
        print_tab("pop rex\n");
        print_tab("push [rex + ");
    }
    else
    {
        print_tab("push [");
    }

    if((var = vartable_find(GLOBALS, node->tok.val.name)) != nullptr)
    {
        print("rcx + %lld]\n", var->offset);
    }
    else if((var = vartable_find(LOCALS, node->tok.val.name)) != nullptr)
    {
        print("rbx + %lld]\n", var->offset);
    }
    else
    {
        semantic_error("Variable wasn't declared", &node->tok);
    }

    return GENERATOR_NOERR;
}

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
                print("rcx + %lld\n", var->offset);
            }
            else if((var = vartable_find(LOCALS, node->left->tok.val.name)) != nullptr)
            {
                print("rbx + %lld\n", var->offset);
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

#define PRINT_CMD(MANGLE, TXT)                                        \
    if(node->tok.val.op == TOK_##MANGLE)                              \
    {                                                                 \
        if(!node->left || !node->right)                               \
            format_error("Operator wrong descendants", &node->tok);   \
        print_tab("%s\n", (TXT));                                     \
    }                                                                 \
    else                                                              \
    
static generator_err oper(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_OP);

    if(node->tok.val.op == TOK_NOT)
    {
        if(node->left || !node->right)
            format_error("Operator wrong descendants", &node->tok);
        print_tab("push 0\n");
        print_tab("eq\n");
    }
    else
    PRINT_CMD(ADD,   "add")
    PRINT_CMD(SUB,   "sub")
    PRINT_CMD(MUL,   "mul")
    PRINT_CMD(DIV,   "div")
    PRINT_CMD(POWER, "pow")
    PRINT_CMD(EQ,    "eq")
    PRINT_CMD(NEQ,   "neq")
    PRINT_CMD(GREAT, "gr")
    PRINT_CMD(LESS,  "le")
    PRINT_CMD(LEQ,   "leq")
    PRINT_CMD(GEQ,   "geq")
    PRINT_CMD(AND,   "and")
    PRINT_CMD(OR,    "or")
    /* else */
        format_error("Unknown operator", &node->tok);

    return GENERATOR_NOERR;
}
#undef PRINT_CMD

static generator_err call_parameter(Node* node, ptrdiff_t n_args)
{
    assert(node);
    assert(node->tok.type == TYPE_AUX && node->tok.val.aux == TOK_PARAMETER);

    n_args--;

    if((n_args > 0 && !node->left) || (n_args == 0 && node->left))
        semantic_error("Wrong amount of arguments", &node->tok);

    if(node->left)
        PASS$(!call_parameter(node->left, n_args), return GENERATOR_PASS_ERROR; );

    if(!node->right)
        format_error("Missing argument", &node->right->tok);
    
    PASS$(!expression(node->right), return GENERATOR_PASS_ERROR; );

    print_tab("pop [rbx + %lld]\n", vartable_end(LOCALS));

    Variable call_var = {};
    call_var.id = (char*) CALL_VARIABLE;
    PASS$(!vartable_add(LOCALS, call_var), return GENERATOR_PASS_ERROR; );

    return GENERATOR_NOERR;
}

static generator_err call(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_AUX && node->tok.val.aux == TOK_CALL);

    if(node->left->tok.type != TYPE_ID)
        format_error("Left descendant of call is not function", &node->left->tok);

    Function* func = functable_find(FUNCS, node->left->tok.val.name);
    if(!func)
        semantic_error("Function wasn't defined", &node->left->tok);
    
    if(strcmp(MAIN_STD_NAME, func->id) == 0)
        semantic_error("'main' can't be called", &node->left->tok);
    
    print("\n");
    
    ptrdiff_t call_offset = vartable_end(LOCALS);
    if((func->n_args == 0 && node->right) || (func->n_args > 0 && !node->right))
        semantic_error("Wrong amount of arguments", &node->left->tok);
    
    INDENTATION++;

    if(node->right)
        PASS$(!call_parameter(node->right, func->n_args), return GENERATOR_PASS_ERROR; );
    
    INDENTATION--;

    print_tab("push rbx\n");
    print_tab("push %lld\n", call_offset);
    print_tab("add\n");
    print_tab("pop rbx\n");

    char* func_name = node->left->tok.val.name;
    print_tab("call func__%llx\n", fnv1_64(func_name, strlen(func_name)));

    print_tab("push rbx\n");
    print_tab("push %lld\n", call_offset);
    print_tab("sub\n");
    print_tab("pop rbx\n");

    print_tab("push rax\n\n");

    MSG$("Function call %s", node->left->tok.val.name);
    vartable_dump(LOCALS);
    LOCALS->size -= func->n_args;

    return GENERATOR_NOERR;
}

static generator_err expression(Node* node)
{
    assert(node);

    if(node->tok.type == TYPE_NUMBER)
    {
        PASS$(!number(node), return GENERATOR_PASS_ERROR; );
        return GENERATOR_NOERR;
    }
    
    if(node->tok.type == TYPE_ID)
    {
        PASS$(!variable(node), return GENERATOR_PASS_ERROR; );
        return GENERATOR_NOERR;
    }

    if(node->tok.type == TYPE_EMBED)
    {
        PASS$(!embedded(node), return GENERATOR_PASS_ERROR; );
        return GENERATOR_NOERR;
    }

    if(node->tok.type == TYPE_AUX && node->tok.val.aux == TOK_CALL)
    {
        PASS$(!call(node), return GENERATOR_PASS_ERROR; );
        return GENERATOR_NOERR;
    }

    if(node->left)
        PASS$(!expression(node->left), return GENERATOR_PASS_ERROR; );

    if(node->right)
        PASS$(!expression(node->right), return GENERATOR_PASS_ERROR; );
        
    if(node->tok.type == TYPE_OP)
    {
        PASS$(!oper(node), return GENERATOR_PASS_ERROR; );
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

    INDENTATION++;

    print("\n");
    PASS$(!expression(node->left), return GENERATOR_PASS_ERROR; );
    
    print_tab("push 0\n");
    print_tab("je if_false__0x%p\n", node);

    if(!node->right->left)
        semantic_error("Conditional statement missing positive branch (no body statements)", &node->tok);

    PASS$(!statement(node->right->left), return GENERATOR_PASS_ERROR; );

    print_tab("jmp if_end__0x%p\n", node);
    print("if_false__0x%p:\n", node);

    if(node->right->right)
        PASS$(!statement(node->right->right), return GENERATOR_PASS_ERROR; );

    print("if_end__0x%p:\n\n", node);

    INDENTATION--;

    return GENERATOR_NOERR;
}

static generator_err cycle(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_KEYWORD && node->tok.val.key == TOK_WHILE);

    if(!node->left || !node->right)
        format_error("Cycle statement missing or wrong descendant", &node->tok);

    INDENTATION++;

    print("\nwhile__0x%p:\n", node);
    
    PASS$(!expression(node->left), return GENERATOR_PASS_ERROR; );

    print_tab("push 0\n");
    print_tab("je while_end__0x%p\n\n", node);

    PASS$(!statement(node->right), return GENERATOR_PASS_ERROR; );

    print_tab("jmp while__0x%p\n", node);
    print("while_end__0x%p:\n\n", node);

    INDENTATION--;

    return GENERATOR_NOERR;
}

static generator_err terminational(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_KEYWORD && node->tok.val.key == TOK_RETURN);

    if(node->left || !node->right)
        format_error("Terminational statement missing or wrong descendant", &node->tok);

    PASS$(!expression(node->right), return GENERATOR_PASS_ERROR; );

    print_tab("pop rax\n");

    print_tab("ret\n");

    return GENERATOR_NOERR;
}

static generator_err assignment(Node* node, Variable_table* vartable)
{
    assert(node && vartable);
    assert(node->tok.type == TYPE_OP && node->tok.val.op == TOK_ASSIGN);

    PASS$(!expression(node->right), return GENERATOR_PASS_ERROR; );

    if(!node->left)
        format_error("Assignment requires lvalue", &node->tok);

    if(node->left->tok.type != TYPE_ID)
        format_error("Assignment requires identifier as lvalue", &node->left->tok);

    Variable var = {};
    ptrdiff_t shift = 0;
    bool is_global = false;

    if(node->left->left)
    {
        if(node->left->left->tok.type != TYPE_KEYWORD || node->left->left->tok.val.key != TOK_CONST)
            format_error("Variable has wrong left descendant ('const' expected)", &node->left->left->tok);

        var.is_const = true;          
    }
    
    Variable* ptr = nullptr;

    if((ptr = vartable_find(GLOBALS, node->left->tok.val.name)) != nullptr)
    {
        is_global = true;
    }
    else if((ptr = vartable_find(LOCALS, node->left->tok.val.name)) != nullptr)
    {
        is_global = false;
    }
    else
    {
        if(functable_find(FUNCS, node->left->tok.val.name) != nullptr)
            semantic_error("Cannot declare variable with function name", &node->left->tok);
        
        if(node->left->right)
        {
            if(node->left->right->tok.type != TYPE_NUMBER)
                semantic_error("Size of variable is not compile-time evaluatable", &node->left->tok);

            shift = (ptrdiff_t) node->left->right->tok.val.num;
        }

        if(shift < 0)
            semantic_error("Size of variable is negative", &node->left->tok);

        var.size   = shift + 1;
        var.id     = node->left->tok.val.name;
        var.offset = vartable_end(vartable);

        PASS$(!vartable_add(vartable, var), return GENERATOR_PASS_ERROR; );

        if(vartable == LOCALS)
            print_tab("pop [rbx + %lld]\n", var.offset + shift);
        else if(vartable == GLOBALS)
            print_tab("pop [rcx + %lld]\n", var.offset + shift);
        
        return GENERATOR_NOERR;
    }

    if(var.is_const)
        semantic_error("'const' specifier in assignment to declared variable", &node->left->tok);
    
    if(ptr->is_const)
        semantic_error("Assignment to 'const' variable", &node->left->tok);
        
    if(node->left->right)
    {
        PASS$(!expression(node->left->right), return GENERATOR_PASS_ERROR; );
        print_tab("pop rex\n");

        print_tab("pop [rex + ");
    }
    else
    {
        print_tab("pop [");
    }

    if(is_global)
    {
        print("rcx + %lld]\n", ptr->offset);
    }
    else
    {
        print("rbx + %lld]\n", ptr->offset);
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
        PASS$(!assignment(node->right, LOCALS), return GENERATOR_PASS_ERROR; );
        return GENERATOR_NOERR;
    }

    PASS$(!expression(node->right), return GENERATOR_PASS_ERROR; );
    print_tab("pop rkx\n");

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

    Variable var = {};
    var.id = node->right->tok.val.name;

    if(node->right->right)
        format_error("Parameter cannot be array", &node->right->tok);
    
    var.size = 1;

    if(node->right->left)
    {
        if(node->right->left->tok.type != TYPE_KEYWORD || node->right->left->tok.val.key != TOK_CONST)
            format_error("Variable has wrong left descendant ('const' expected)", &node->right->tok);
        
        var.is_const = true;
    }

    if(vartable_find(LOCALS, var.id) || vartable_find(GLOBALS, var.id) || functable_find(FUNCS, var.id))
        semantic_error("Variable redeclaration", &node->right->tok);

    PASS$(!vartable_add(LOCALS, var), return GENERATOR_PASS_ERROR; );

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

    Function func = {};
    
    Node* ptr = node->right->left->right;

    while(ptr)
    {
        if(ptr->tok.type != TYPE_AUX || ptr->tok.val.aux != TOK_PARAMETER)
            format_error("'parameter' expected", &ptr->tok);
        
        func.n_args++;

        ptr = ptr->left;
    }

    ptr = node->right->left->left;
    if(!ptr)
        format_error("Function name is missing", &node->right->left->tok);

    if(ptr->tok.type != TYPE_ID)
        format_error("Function name is not identifier", &ptr->tok);
    
    func.id = ptr->tok.val.name;
        
    if(functable_find(FUNCS, func.id) != nullptr)
        semantic_error("Function redefinition", &ptr->tok);

    PASS$(!functable_add(FUNCS, &func), return GENERATOR_PASS_ERROR; );

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

    PASS$(!assignment(node->right, GLOBALS), return GENERATOR_PASS_ERROR; );

    return GENERATOR_NOERR;
}

static generator_err generate_funcs(Node* node)
{
    assert(node);

    if(node->left)
        PASS$(!generate_funcs(node->left), return GENERATOR_PASS_ERROR; );
    
    if(node->right->tok.type != TYPE_AUX || node->right->tok.val.aux != TOK_DEFINE)
        return GENERATOR_NOERR;
    
    char* func_name = node->right->left->left->tok.val.name;
    print("\n\n\nfunc__%llx:\n", fnv1_64(func_name, strlen(func_name)));

    INDENTATION++;

    Node* param = node->right->left->right;
    if(param)
        PASS$(!parameter(param), return GENERATOR_PASS_ERROR; );

    Node* stmnt = node->right->right;
    if(stmnt)
        PASS$(!statement(stmnt), return GENERATOR_PASS_ERROR; );

    if(stmnt->right->tok.type != TYPE_KEYWORD || stmnt->right->tok.val.key != TOK_RETURN)
        semantic_error("Missing terminational", &node->right->left->left->tok);

    MSG$("End of function %s", node->right->left->left->tok.val.name);
    vartable_dump(LOCALS);
    LOCALS->size = 0;

    INDENTATION--;

    return GENERATOR_NOERR;
}

generator_err generator(Tree* tree, FILE* ostream)
{
    assert(ostream && tree);

    nametable_dump_init(dumpsystem_get_stream(backend_log));

    OSTREAM = ostream;

    Variable_table globals = {};
    Variable_table locals  = {};
    Function_table funcs   = {};
    GLOBALS = &globals;
    LOCALS  = &locals;
    FUNCS   = &funcs;

    PASS$(!fill_funcs_table(tree->root), return GENERATOR_PASS_ERROR; );
    functable_dump(&funcs);

    print_tab("push %lld\n"
              "pop rcx\n",
               MEMORY_GLOBAL);

    PASS$(!generate_globals(tree->root), return GENERATOR_PASS_ERROR; );
    vartable_dump(&globals);

    print_tab("push %lld\n"
              "pop rbx\n",
              vartable_end(&globals));

    print_tab("call func__%llx\n", fnv1_64(MAIN_STD_NAME, sizeof(MAIN_STD_NAME) - 1));

    print_tab("hlt\n");

    PASS$(!generate_funcs(tree->root), return GENERATOR_PASS_ERROR; );

    GLOBALS = nullptr;
    LOCALS  = nullptr;
    FUNCS   = nullptr;
    vartable_dstr(&locals);
    vartable_dstr(&globals);
    functable_dstr(&funcs);

    OSTREAM = nullptr;

    return IS_ERROR;
}

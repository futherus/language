#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "generator.h"
#include "nametable/nametable.h"
#include "../dumpsystem/dumpsystem.h"

/*
    rax -- return value
    rbx -- memory stack pointer
    rex -- embedded functions
    rkx -- trash register
*/

static Function_table* FUNCS   = nullptr;
static Variable_table* LOCALS  = nullptr;
static Variable_table* GLOBALS = nullptr;

static FILE* OSTREAM = nullptr;

#define semantic_error(TOK) ASSERT$(0, Semantic error, return GENERATOR_SEMANTIC_ERROR; );

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
        semantic_error();

    print_tab("push %lg\n", node->tok.val.num);

    return GENERATOR_NOERR;
}

static generator_err variable(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_ID);

    if(node->left)
        semantic_error();
    
    Variable* var = 0;

    if((var = vartable_find(GLOBALS, node->tok.val.name)) != nullptr)
    {
        print_tab("push [rcx + %lld", var->offset);
    }
    else if((var = vartable_find(LOCALS, node->tok.val.name)) != nullptr)
        print_tab("push [rbx + %lld", var->offset);
    else
        semantic_error();

    if(node->right)
    {
        if(node->right->tok.type != TYPE_NUMBER)
            semantic_error();
        
        if(node->right->tok.val.num > 0)
            print(" + %lg", node->right->tok.val.num);
        else if(node->right->tok.val.num < 0)
            print(" %lg", node->right->tok.val.num);
    }

    print("]\n");

    return GENERATOR_NOERR;
}

static generator_err embedded(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_EMBED);

    if(!node->right)
        semantic_error();

    switch(node->tok.val.aux)
    {
        case TOK_SIN:
        {
            expression(node->right);

            print_tab("pop rex\n");
            print_tab("sin rex\n");
            break;
        }
        case TOK_PRINT:
        {
            expression(node->right);

            print_tab("out\n");

            print_tab("push 0\n");
            break;
        }
        case TOK_SCAN:
        {
            if(node->right->tok.type != TYPE_ID)
                semantic_error();
            
            print_tab("in\n");

            Variable* var = {};
            if((var = vartable_find(LOCALS, node->right->tok.val.name)) != nullptr)
            {
                print_tab("pop [rbx + %lld]\n", var->offset);
            }
            else if((var = vartable_find(GLOBALS, node->right->tok.val.name)) != nullptr)
            {
                print_tab("pop [rcx + %lld]\n", var->offset);
            }
            else
            {
                semantic_error();
            }

            print_tab("push 0\n");

            break;
        }
        default:
        {
            assert(0);
        }
    }

    return GENERATOR_NOERR;
}

#define PRINT_CMD(MANGLE, TXT)     \
    case TOK_##MANGLE:             \
        print_tab("%s\n", (TXT));  \
        break;                     \
    
static generator_err oper(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_OP);

    switch(node->tok.val.op)
    {
        PRINT_CMD(ADD,   "add");
        PRINT_CMD(SUB,   "sub");
        PRINT_CMD(MUL,   "mul");
        PRINT_CMD(DIV,   "div");
        PRINT_CMD(EQ,    "eq");
        PRINT_CMD(NEQ,   "neq");
        PRINT_CMD(GREAT, "gr");
        PRINT_CMD(LESS,  "le");
        PRINT_CMD(LEQ,   "leq");
        PRINT_CMD(GEQ,   "geq");
        PRINT_CMD(AND,   "and");
        PRINT_CMD(OR,    "or");
        
        default:
            assert(0);
    }

    return GENERATOR_NOERR;
}
#undef PRINT_CMD

static generator_err call_parameter(Node* node, ptrdiff_t n_args)
{
    assert(node);
    assert(node->tok.type == TYPE_AUX && node->tok.val.aux == TOK_PARAMETER);

    n_args--;

    if((n_args > 0 && !node->left) || (n_args == 0 && node->left))
        semantic_error();

    if(node->left)
        call_parameter(node->left, n_args);

    if(!node->right)
        semantic_error();
    
    expression(node->right);

    print_tab("pop [rbx + %lld]\n", vartable_end(LOCALS));

    Variable call_var = {};
    call_var.id = (char*) CALL_VARIABLE;
    vartable_add(LOCALS, call_var);

    return GENERATOR_NOERR;
}

static generator_err call(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_AUX && node->tok.val.aux == TOK_CALL);

    if(node->left->tok.type != TYPE_ID)
        semantic_error();

    Function* func = functable_find(FUNCS, node->left->tok.val.name);
    if(!func)
        semantic_error();
    
    if(strcmp("main", func->id) == 0)
        semantic_error();
    
    print("\n");
    
    ptrdiff_t call_offset = vartable_end(LOCALS);
    if((func->n_args == 0 && node->right) || (func->n_args > 0 && !node->right))
        semantic_error();
    
    INDENTATION++;

    if(node->right)
        call_parameter(node->right, func->n_args);
    
    INDENTATION--;

    print_tab("push rbx\n");
    print_tab("push %lld\n", call_offset);
    print_tab("add\n");
    print_tab("pop rbx\n");

    print_tab("call %s\n", node->left->tok.val.name);

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
        number(node);
        return GENERATOR_NOERR;
    }
    
    if(node->tok.type == TYPE_ID)
    {
        variable(node);
        return GENERATOR_NOERR;
    }

    if(node->tok.type == TYPE_EMBED)
    {
        embedded(node);
        return GENERATOR_NOERR;
    }

    if(node->tok.type == TYPE_AUX && node->tok.val.aux == TOK_CALL)
    {
        call(node);
        return GENERATOR_NOERR;
    }

    if(node->left)
        expression(node->left);

    if(node->right)
        expression(node->right);
        
    if(node->tok.type == TYPE_OP)
    {
        oper(node);
        return GENERATOR_NOERR;
    }
        
    semantic_error();

    return GENERATOR_NOERR;
}

static generator_err conditional(Node* node)
{
    assert(node);
    if(!node->left || !node->right || node->right->tok.type != TYPE_AUX || node->right->tok.val.aux != TOK_DECISION)
        semantic_error();

    INDENTATION++;

    print("\n");
    expression(node->left);
    
    print_tab("push 0\n");
    print_tab("je if_false__0x%p\n", node);

    if(!node->right->left)
        semantic_error();

    statement(node->right->left);

    print_tab("jmp if_end__0x%p\n", node);
    print("if_false__0x%p:\n", node);

    if(node->right->right)
        statement(node->right->right);

    print("if_end__0x%p:\n\n", node);

    INDENTATION--;

    return GENERATOR_NOERR;
}

static generator_err cycle(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_KEYWORD && node->tok.val.key == TOK_WHILE);

    if(!node->left || !node->right)
        semantic_error();

    INDENTATION++;

    print("\nwhile__0x%p:\n", node);
    
    expression(node->left);

    print_tab("push 0\n");
    print_tab("je while_end__0x%p\n\n", node);

    statement(node->right);

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
        semantic_error();

    expression(node->right);

    print_tab("pop rax\n");

    print_tab("ret\n");

    return GENERATOR_NOERR;
}

static generator_err assignment(Node* node, Variable_table* vartable)
{
    assert(node && vartable);
    assert(node->tok.type == TYPE_OP && node->tok.val.op == TOK_ASSIGN);

    expression(node->right);

    if(!node->left)
        semantic_error();

    if(node->left->tok.type != TYPE_ID)
        semantic_error();

    Variable var = {};
    ptrdiff_t shift = 0;

    if(node->left->left)
    {
        if(node->left->left->tok.type != TYPE_KEYWORD || node->left->left->tok.val.key != TOK_CONST)
            semantic_error();

        var.is_const = true;          
    }
    
    if(node->left->right)
    {
        if(node->left->right->tok.type != TYPE_NUMBER)
            semantic_error();
        
        shift = (ptrdiff_t) node->left->right->tok.val.num;
    }
    
    Variable* ptr = nullptr;
    if((ptr = vartable_find(GLOBALS, node->left->tok.val.name)) != nullptr)
    {
        if(var.is_const || ptr->is_const)
            semantic_error();
            
        print_tab("pop [rcx + %lld", ptr->offset);
    }
    else if((ptr = vartable_find(LOCALS, node->left->tok.val.name)) != nullptr)
    {
        if(var.is_const || ptr->is_const)
            semantic_error();
        
        print_tab("pop [rbx + %lld", ptr->offset);
    }
    else
    {
        if(functable_find(FUNCS, node->left->tok.val.name) != nullptr)
            semantic_error();
        
        if(shift < 0)
            semantic_error();

        var.size   = shift + 1;
        var.id     = node->left->tok.val.name;
        var.offset = vartable_end(vartable);

        vartable_add(vartable, var);

        if(vartable == LOCALS)
            print_tab("pop [rbx + %lld", var.offset);
        else if(vartable == GLOBALS)
            print_tab("pop [rcx + %lld", var.offset);
    }

    if(shift > 0)
        print(" + %lld", shift);
    else if(shift < 0)
        print("  %lld", shift);

    print("]\n");

    return GENERATOR_NOERR;
}

static generator_err statement(Node* node)
{
    assert(node);

    if(node->tok.type != TYPE_AUX || node->tok.val.aux != TOK_STATEMENT)
        semantic_error();

    if(node->left)
        statement(node->left);

    if(node->right->tok.type == TYPE_KEYWORD)
    {
        if(node->right->tok.val.key == TOK_IF)
        {
            conditional(node->right);
            return GENERATOR_NOERR;
        }
        
        if(node->right->tok.val.key == TOK_WHILE)
        {
            cycle(node->right);
            return GENERATOR_NOERR;
        }

        if(node->right->tok.val.key == TOK_RETURN)
        {
            terminational(node->right);
            return GENERATOR_NOERR;
        }
    }

    if(node->right->tok.type == TYPE_OP && node->right->tok.val.op == TOK_ASSIGN)
    {
        assignment(node->right, LOCALS);
        return GENERATOR_NOERR;
    }

    expression(node->right);
    print_tab("pop rkx\n");

    return GENERATOR_NOERR;
}

static generator_err parameter(Node* node)
{
    assert(node);
    
    if(node->tok.type != TYPE_AUX || node->tok.val.aux != TOK_PARAMETER)
        semantic_error();

    if(node->left)
        parameter(node->left);
    
    if(node->right->tok.type != TYPE_ID)
        semantic_error();

    Variable var = {};
    var.id = node->right->tok.val.name;

    if(node->right->right)
        semantic_error();
    
    var.size = 1;

    if(node->right->left)
    {
        if(node->right->left->tok.type != TYPE_KEYWORD || node->right->left->tok.val.key != TOK_CONST)
            semantic_error();
        
        var.is_const = true;
    }

    if(vartable_find(LOCALS, var.id) || vartable_find(GLOBALS, var.id) || functable_find(FUNCS, var.id))
        semantic_error();

    vartable_add(LOCALS, var);

    return GENERATOR_NOERR;
}

static generator_err fill_funcs_table(Node* node)
{
    assert(node);

    if(node->left)
        fill_funcs_table(node->left);
    
    if(node->tok.type != TYPE_AUX || node->tok.val.aux != TOK_STATEMENT)
        semantic_error();
    
    assert(node->right);

    if(node->right->tok.type == TYPE_OP && node->right->tok.val.op == TOK_ASSIGN)
        return GENERATOR_NOERR;

    if(node->right->tok.type != TYPE_AUX || node->right->tok.val.aux != TOK_DEFINE)
        semantic_error();
    
    assert(node->right->left);

    if(node->right->left->tok.type != TYPE_AUX || node->right->left->tok.val.aux != TOK_FUNCTION)
        semantic_error();

    Node* ptr = node->right->left->left;
    Function func = {};
    
    assert(ptr);

    if(ptr->tok.type != TYPE_ID)
        semantic_error();
    
    func.id = ptr->tok.val.name;
        
    ptr = node->right->left->right;

    while(ptr)
    {
        if(ptr->tok.type != TYPE_AUX || ptr->tok.val.aux != TOK_PARAMETER)
            semantic_error();
        
        func.n_args++;

        ptr = ptr->left;
    }

    if(functable_find(FUNCS, func.id) != nullptr)
        semantic_error();

    PASS$(!functable_add(FUNCS, &func), return GENERATOR_BAD_ALLOC; );

    return GENERATOR_NOERR;
}

static generator_err generate_globals(Node* node)
{
    assert(node);

    if(node->left)
        generate_globals(node->left);
    
    if(node->tok.type != TYPE_AUX || node->tok.val.aux != TOK_STATEMENT)
        semantic_error();

    assert(node->right);

    if(node->right->tok.type == TYPE_AUX && node->right->tok.val.aux == TOK_DEFINE)
        return GENERATOR_NOERR;

    if(node->right->tok.type != TYPE_OP || node->right->tok.val.op != TOK_ASSIGN)
        semantic_error();

    assignment(node->right, GLOBALS);

    return GENERATOR_NOERR;
}

static generator_err generate_funcs(Node* node)
{
    assert(node);

    if(node->left)
        generate_funcs(node->left);
    
    if(node->right->tok.type != TYPE_AUX || node->right->tok.val.aux != TOK_DEFINE)
        return GENERATOR_NOERR;
    
    print("\n\n\n%s:\n", node->right->left->left->tok.val.name);

    INDENTATION++;

    Node* param = node->right->left->right;
    if(param)
        parameter(param);

    Node* stmnt = node->right->right;
    if(stmnt)
        statement(stmnt);

    if(stmnt->right->tok.type != TYPE_KEYWORD || stmnt->right->tok.val.key != TOK_RETURN)
        semantic_error();

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

    fill_funcs_table(tree->root);
    functable_dump(&funcs);

    print_tab("push %lld\n"
              "pop rcx\n",
               MEMORY_GLOBAL);

    print_tab("push %lld\n"
              "pop rbx\n",
               MEMORY_LOCAL);

    generate_globals(tree->root);
    vartable_dump(&globals);

    print_tab("call main\n");

    print_tab("hlt\n");

    generate_funcs(tree->root);

    GLOBALS = nullptr;
    LOCALS  = nullptr;
    FUNCS   = nullptr;
    vartable_dstr(&locals);
    vartable_dstr(&globals);
    functable_dstr(&funcs);

    OSTREAM = nullptr;

    return GENERATOR_NOERR;
}

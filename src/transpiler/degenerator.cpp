#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "transpiler.h"
#include "../common/dumpsystem.h"

static FILE* OSTREAM = nullptr;

static degenerator_err IS_ERROR = DEGENERATOR_NOERR;

#define semantic_error(MSG_, TOK_)                                                      \
do                                                                                      \
{                                                                                       \
    IS_ERROR = DEGENERATOR_SEMANTIC_ERROR;                                                \
    fprintf(stderr, "\x1b[31mSemantic error:\x1b[0m %s : %s\n", (MSG_), std_demangle(TOK_));\
    FILE* stream_ = dumpsystem_get_opened_stream();                                     \
                                                                                        \
    if(stream_)                                                                         \
    {                                                                                   \
        fprintf(stream_, "<span class = \"error\">Semantic error: %s : %s\n</span>"     \
                         "\t\t\t\tat %s:%d:%s\n",                                       \
                         (MSG_), std_demangle(TOK_),                                    \
                         __FILE__, __LINE__, __PRETTY_FUNCTION__);                      \
                                                                                        \
        return DEGENERATOR_PASS_ERROR;                                                         \
    }                                                                                   \
} while(0)                                                                              \

#define format_error(MSG_, TOK_)                                                        \
do                                                                                      \
{                                                                                       \
    IS_ERROR = DEGENERATOR_FORMAT_ERROR;                                                  \
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
        return DEGENERATOR_NOERR;                                                         \
    }                                                                                   \
} while(0)                                                                              \

///////////////////////////////////////////////////////////////////////////////////////////////////

static int INDENTATION = 0;

#define print_tab(fmt, ...) fprintf(OSTREAM, "%*s" fmt, INDENTATION * 4, "", ##__VA_ARGS__)
#define print(fmt, ...)     fprintf(OSTREAM, fmt, ##__VA_ARGS__)

static degenerator_err expression(Node* node);
static degenerator_err statement(Node* node);

static degenerator_err number(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_NUMBER);

    if(node->left || node->right)
        format_error("Number has descendants", &node->tok);

    print("%lg", node->tok.val.num);

    return DEGENERATOR_NOERR;
}

static degenerator_err variable(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_ID);

    Token tok = {};
    if(node->left)
    {
        tok.type = TYPE_KEYWORD;
        tok.val.key = TOK_CONST;
        print("%s ", demangle(&tok));

        semantic_error("Variable has 'const' specifier in expression", &node->tok);
    }
    
    print("%s", demangle(&node->tok));

    if(node->right)
    {
        tok.type = TYPE_OP;
        tok.val.op = TOK_SHIFT;
        print(" %s (", demangle(&tok));

        PASS$(!expression(node->right), return DEGENERATOR_PASS_ERROR; );
        
        print(")");
    }

    return DEGENERATOR_NOERR;
}

static degenerator_err embedded(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_EMBED);

    switch(node->tok.val.emb)
    {
        case TOK_SIN : case TOK_COS : case TOK_PRINT : case TOK_INT : case TOK_SQRT :
        {
            if(!node->right || node->left)
                format_error("Embedded '' requires 1 argument", &node->tok);

            print("%s(", demangle(&node->tok));

            PASS$(!expression(node->right), return DEGENERATOR_PASS_ERROR; );

            print(")");
            break;
        }
        case TOK_SCAN:
        {
            if(node->right || node->left)
                format_error("Embedded 'scan' cannot has arguments", &node->tok);

            print("%s()", demangle(&node->tok));
            break;
        }
        case TOK_SHOW:
        {
            if(!node->right || !node->left)
                format_error("Embedded 'show' requires 2 arguments", &node->tok);
            
            if(node->left->tok.type != TYPE_ID)
                format_error("Embedded 'show' requires variable as first argument", &node->left->tok);

            print("%s(", demangle(&node->tok));

            variable(node->left);
        
            print(", ");

            PASS$(!expression(node->right), return DEGENERATOR_PASS_ERROR; );

            print(")");
            break;
        }
        default:
        {
            assert(0);
        }
    }

    return DEGENERATOR_NOERR;
}
    
static degenerator_err call_parameter(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_AUX && node->tok.val.aux == TOK_PARAMETER);

    if(node->left)
    {
        PASS$(!call_parameter(node->left), return DEGENERATOR_PASS_ERROR; );

        Token tok = {.type = TYPE_OP};
        tok.val.op = TOK_COMMA;
        print("%s ", demangle(&tok));
    }

    if(!node->right)
        format_error("Missing argument", &node->right->tok);
    
    PASS$(!expression(node->right), return DEGENERATOR_PASS_ERROR; );

    return DEGENERATOR_NOERR;
}

static degenerator_err call(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_AUX && node->tok.val.aux == TOK_CALL);

    if(node->left->tok.type != TYPE_ID)
        format_error("Left descendant of call is not function", &node->left->tok);
    
    print("%s(", demangle(&node->left->tok));
    if(node->right)
        PASS$(!call_parameter(node->right), return DEGENERATOR_PASS_ERROR; );
    
    print(")");

    return DEGENERATOR_NOERR;
}

static ptrdiff_t get_priority(Token* tok)
{
    assert(tok);

    switch(tok->type)
    {
        case TYPE_EMBED : case TYPE_ID : case TYPE_NUMBER :
        {
            return 10;
        }
        case TYPE_OP:
        {
            if(tok->val.op == TOK_NOT)
                return 9;
            else if(tok->val.op == TOK_POWER)
                return 8;
            else if(tok->val.op == TOK_MUL || tok->val.op == TOK_DIV)
                return 7;
            else if(tok->val.op == TOK_ADD || tok->val.op == TOK_SUB)
                return 6;
            else if(tok->val.op == TOK_EQ  || tok->val.op == TOK_NEQ   || tok->val.op == TOK_LEQ ||
                    tok->val.op == TOK_GEQ || tok->val.op == TOK_GREAT || tok->val.op == TOK_LESS)
                return 5;
            else if(tok->val.op == TOK_AND || tok->val.op == TOK_OR)
                return 4;
            else
                return -777;
        }
        default : case TYPE_EOF : case TYPE_NOTYPE : case TYPE_KEYWORD : case TYPE_AUX :
                  case TYPE_DIRECTIVE_BEGIN : case TYPE_DIRECTIVE_END :
        {
            return -777;
        }
    }
}

static degenerator_err expression(Node* node)
{
    assert(node);

    if(node->tok.type == TYPE_NUMBER)
    {
        PASS$(!number(node), return DEGENERATOR_PASS_ERROR; );
        return DEGENERATOR_NOERR;
    }
    
    if(node->tok.type == TYPE_ID)
    {
        PASS$(!variable(node), return DEGENERATOR_PASS_ERROR; );
        return DEGENERATOR_NOERR;
    }

    if(node->tok.type == TYPE_EMBED)
    {
        PASS$(!embedded(node), return DEGENERATOR_PASS_ERROR; );
        return DEGENERATOR_NOERR;
    }

    if(node->tok.type == TYPE_AUX && node->tok.val.aux == TOK_CALL)
    {
        PASS$(!call(node), return DEGENERATOR_PASS_ERROR; );
        return DEGENERATOR_NOERR;
    }

    if(node->tok.type != TYPE_OP)
        format_error("Unexpected token in expression", &node->tok);

    ptrdiff_t op_priority = get_priority(&node->tok);
    ptrdiff_t priority    = 0;

    if(node->left)
    {
        priority = get_priority(&node->left->tok);
        if(op_priority >= priority)
            print("(");

        PASS$(!expression(node->left), return DEGENERATOR_PASS_ERROR; );
    
        if(op_priority >= priority)
            print(")");
    }

    print(" %s ", demangle(&node->tok));

    if(node->right)
    {
        priority = get_priority(&node->right->tok);
        if(op_priority >= priority)
            print("(");

        PASS$(!expression(node->right), return DEGENERATOR_PASS_ERROR; );
    
        if(op_priority >= priority)
            print(")");
    }

    return DEGENERATOR_NOERR;
}

static degenerator_err conditional(Node* node)
{
    assert(node);
    if(!node->left || !node->right || node->right->tok.type != TYPE_AUX || node->right->tok.val.aux != TOK_DECISION)
        format_error("Conditional statement missing or wrong descendant", &node->tok);

    print_tab("%s(", demangle(&node->tok));

    PASS$(!expression(node->left), return DEGENERATOR_PASS_ERROR; );
    print(")\n");

    Token tok = {.type = TYPE_OP};
    tok.val.op = TOK_LFPAR;
    print_tab("%s\n", demangle(&tok));
    INDENTATION++;

    if(!node->right->left)
        semantic_error("Conditional statement missing positive branch (no body statements)", &node->tok);

    PASS$(!statement(node->right->left), return DEGENERATOR_PASS_ERROR; );

    INDENTATION--;
    tok = {.type = TYPE_OP};
    tok.val.op = TOK_RFPAR;
    print_tab("%s\n", demangle(&tok));

    if(!node->right->right)
    {
        print("\n");
        return DEGENERATOR_NOERR;
    }

    tok.type    = TYPE_KEYWORD;
    tok.val.key = TOK_ELSE;
    print_tab("%s\n", demangle(&tok));

    tok.type = TYPE_OP;
    tok.val.op = TOK_LFPAR;
    print_tab("%s\n", demangle(&tok));
    INDENTATION++;

    PASS$(!statement(node->right->right), return DEGENERATOR_PASS_ERROR; );

    INDENTATION--;
    tok = {.type = TYPE_OP};
    tok.val.op = TOK_RFPAR;
    print_tab("%s\n\n", demangle(&tok));

    return DEGENERATOR_NOERR;
}

static degenerator_err cycle(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_KEYWORD && node->tok.val.key == TOK_WHILE);

    if(!node->left || !node->right)
        format_error("Cycle statement missing descendant", &node->tok);

    print("\n");
    print_tab("%s(", demangle(&node->tok));

    PASS$(!expression(node->left), return DEGENERATOR_PASS_ERROR; );

    print(")\n");

    Token tok = {.type = TYPE_OP};
    tok.val.op = TOK_LFPAR;
    print_tab("%s\n", demangle(&tok));
    INDENTATION++;

    PASS$(!statement(node->right), return DEGENERATOR_PASS_ERROR; );

    INDENTATION--;
    tok = {.type = TYPE_OP};
    tok.val.op = TOK_RFPAR;
    print_tab("%s\n", demangle(&tok));

    return DEGENERATOR_NOERR;
}

static degenerator_err terminational(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_KEYWORD && node->tok.val.key == TOK_RETURN);

    if(node->left || !node->right)
        format_error("Terminational statement missing or wrong descendant", &node->tok);

    print_tab("%s ", demangle(&node->tok));
    PASS$(!expression(node->right), return DEGENERATOR_PASS_ERROR; );

    Token tok = {.type = TYPE_OP};
    tok.val.op = TOK_SEMICOLON;
    print(" %s\n", demangle(&tok));

    return DEGENERATOR_NOERR;
}

static degenerator_err assignment(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_OP && node->tok.val.op == TOK_ASSIGN);

    if(!node->left)
        format_error("Assignment requires lvalue", &node->tok);

    if(node->left->tok.type != TYPE_ID)
        format_error("Assignment requires identifier as lvalue", &node->left->tok);

    print_tab("");

    if(node->left->left)
    {
        if(node->left->left->tok.type != TYPE_KEYWORD || node->left->left->tok.val.key != TOK_CONST)
            format_error("Variable has wrong left descendant ('const' expected)", &node->left->left->tok);

        print("%s ", demangle(&node->left->left->tok));
    }
    
    print("%s", demangle(&node->left->tok));

    Token tok = {};
    
    if(node->left->right)
    {
        tok = {.type = TYPE_OP};
        tok.val.op = TOK_LQPAR;
        print("%s", demangle(&tok));

        PASS$(!expression(node->left->right), return DEGENERATOR_PASS_ERROR; );

        tok = {.type = TYPE_OP};
        tok.val.op = TOK_RQPAR;
        print("%s", demangle(&tok));
    }

    print(" %s ", demangle(&node->tok));

    PASS$(!expression(node->right), return DEGENERATOR_PASS_ERROR; );

    tok = {.type = TYPE_OP};
    tok.val.op = TOK_SEMICOLON;
    print(" %s\n", demangle(&tok));
    
    return DEGENERATOR_NOERR;
}

static degenerator_err statement(Node* node)
{
    assert(node);

    if(node->tok.type != TYPE_AUX || node->tok.val.aux != TOK_STATEMENT)
        format_error("'statement' expected", &node->tok);

    if(node->left)
        PASS$(!statement(node->left), return DEGENERATOR_PASS_ERROR; );

    if(node->right->tok.type == TYPE_KEYWORD)
    {
        if(node->right->tok.val.key == TOK_IF)
        {
            PASS$(!conditional(node->right), return DEGENERATOR_PASS_ERROR; );
            return DEGENERATOR_NOERR;
        }
        else if(node->right->tok.val.key == TOK_WHILE)
        {
            PASS$(!cycle(node->right), return DEGENERATOR_PASS_ERROR; );
            return DEGENERATOR_NOERR;
        }
        else if(node->right->tok.val.key == TOK_RETURN)
        {
            PASS$(!terminational(node->right), return DEGENERATOR_PASS_ERROR; );
            return DEGENERATOR_NOERR;
        }
    }

    if(node->right->tok.type == TYPE_OP && node->right->tok.val.op == TOK_ASSIGN)
    {
        PASS$(!assignment(node->right), return DEGENERATOR_PASS_ERROR; );
        return DEGENERATOR_NOERR;
    }

    print_tab("");
    PASS$(!expression(node->right), return DEGENERATOR_PASS_ERROR; );

    Token tok = {.type = TYPE_OP};
    tok.val.op = TOK_SEMICOLON;
    print(" %s\n", demangle(&tok));

    return DEGENERATOR_NOERR;
}

static degenerator_err parameter(Node* node)
{
    assert(node);
    
    if(node->tok.type != TYPE_AUX || node->tok.val.aux != TOK_PARAMETER)
        format_error("'parameter' expected", &node->tok);

    if(node->left)
    {
        PASS$(!parameter(node->left), return DEGENERATOR_PASS_ERROR; );

        Token tok = {.type = TYPE_OP};
        tok.val.op = TOK_COMMA;
        print("%s ", demangle(&tok));
    }

    if(node->right->tok.type != TYPE_ID)
        format_error("Parameter is not id", &node->right->tok);

    if(node->right->right)
        format_error("Parameter cannot be array", &node->right->tok);
    
    if(node->right->left)
    {
        if(node->right->left->tok.type != TYPE_KEYWORD || node->right->left->tok.val.key != TOK_CONST)
            format_error("Variable has wrong left descendant ('const' expected)", &node->right->tok);
        
        print("%s ", demangle(&node->right->left->tok));
    }

    print("%s", demangle(&node->right->tok));

    return DEGENERATOR_NOERR;
}

static degenerator_err function(Node* node)
{
    assert(node);
    assert(node->tok.type == TYPE_AUX && node->tok.val.aux == TOK_DEFINE);
        
    print("\n\n");
    print_tab("%s(", demangle(&node->left->left->tok));

    Node* param = node->left->right;
    if(param)
        PASS$(!parameter(param), return DEGENERATOR_PASS_ERROR; );

    print(")\n");

    Token tok = {.type = TYPE_OP};
    tok.val.op = TOK_LFPAR;
    print_tab("%s\n", demangle(&tok));
    INDENTATION++;

    Node* stmnt = node->right;
    if(stmnt)
        PASS$(!statement(stmnt), return DEGENERATOR_PASS_ERROR; );

    if(stmnt->right->tok.type != TYPE_KEYWORD || stmnt->right->tok.val.key != TOK_RETURN)
        semantic_error("Missing terminational", &node->right->left->left->tok);

    INDENTATION--;
    tok = {.type = TYPE_OP};
    tok.val.op = TOK_RFPAR;
    print_tab("%s\n", demangle(&tok));

    return DEGENERATOR_NOERR;
}

static degenerator_err generate_first_line(Node* node)
{
    assert(node);
    
    if(node->left)
        PASS$(!generate_first_line(node->left), return DEGENERATOR_PASS_ERROR; );

    if(node->tok.type != TYPE_AUX || node->tok.val.aux != TOK_STATEMENT)
        format_error("'statement' expected (first line)", &node->tok);

    if(!node->right)
        format_error("Missing 'statement' body (first line)", &node->tok);

    if(node->right->tok.type == TYPE_OP && node->right->tok.val.op == TOK_ASSIGN)
    {
        PASS$(!assignment(node->right), return DEGENERATOR_PASS_ERROR; );
    }
    else if(node->right->tok.type == TYPE_AUX && node->right->tok.val.aux == TOK_DEFINE)
    {
        PASS$(!function(node->right), return DEGENERATOR_PASS_ERROR; );
    }
    else
    {
        format_error("Assignment or function definition expected (first line)", &node->right->tok);
    }

    return DEGENERATOR_NOERR;
}

degenerator_err degenerator(Tree* tree, FILE* ostream)
{
    assert(ostream && tree);

    OSTREAM = ostream;

    PASS$(!generate_first_line(tree->root), return DEGENERATOR_PASS_ERROR; );

    OSTREAM = nullptr;

    return IS_ERROR;
}

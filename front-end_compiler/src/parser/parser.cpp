#include <assert.h>

#include "parser.h"
#include "../lexer/lexer.h"
#include "../dumpsystem/dumpsystem.h"

static void syntax_error()
{
    printf("SYNTAX ERROR");
    exit(1);
}

/*
    Grammar:
        Primary    ::= '(' Expression ')' | TYPE_VAR | TYPE_NUMBER |
                       TYPE_FUNC '(' Expression {',' Expression}* ')' |
                       TYPE_EMB  '(' Expression ')'
        Term       ::= Primary {[/*] Primary}*
        Expression ::= Term {[+-] Term}*
        Statement  ::= 'return' Expression ';' |
                       'const'? TYPE_VAR '=' Expression ';' |
                       Expression ';' |
                       'if'    '(' Expression ')' '{' Statement '}'
                       'while' '(' Expression ')' '{' Statement '}'
        General    ::= {Statement}*
*/
 
static parser_err primary(Node** base_ptr);
static parser_err term(Node** base_ptr);
static parser_err expression(Node** base_ptr);
static parser_err general(Node** base_ptr);

#define MK_OP(BASE, OP)    tree_add(TREE, (BASE), {.type = TYPE_OP, .val.op = (OP)})
#define MK_NODE(BASE, TOK) tree_add(TREE, (BASE), (TOK))

static Tree* TREE = nullptr;

static parser_err primary(Node** base_ptr)
{
    Token tok = {};
    consume(&tok);

    if(tok.type == TYPE_OP && tok.val.op == TOK_LRPAR)
    {
        expression(base_ptr);

        consume(&tok);
        if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
            syntax_error();
    }
    else if(tok.type == TYPE_ID || tok.type == TYPE_NUMBER)
    {
        MK_NODE(base_ptr, &tok);
    }
    else
    {
        syntax_error();
    }

    return PARSER_NOERR;
}

static parser_err term(Node** base_ptr)
{
    primary(base_ptr);

    Token tok = {};
    peek(&tok);

    while(tok.type == TYPE_OP && (tok.val.op == TOK_MUL || tok.val.op == TOK_DIV))
    {
        consume(&tok);

        Node* tmp = *base_ptr;

        MK_NODE(base_ptr, &tok);
        
        (*base_ptr)->left = tmp;
        primary(&(*base_ptr)->right);
        
        peek(&tok);
    }
    
    return PARSER_NOERR;
}

static parser_err expression(Node** base_ptr)
{
    term(base_ptr);

    Token tok = {};
    peek(&tok);

    while(tok.type == TYPE_OP && (tok.val.op == TOK_ADD || tok.val.op == TOK_SUB))
    {
        consume(&tok);

        Node* tmp = *base_ptr;

        MK_NODE(base_ptr, &tok);

        (*base_ptr)->left = tmp;
        term(&(*base_ptr)->right);

        peek(&tok);
    }

    return PARSER_NOERR;
}

static parser_err general(Node** base_ptr)
{
    expression(base_ptr);

    Token tok = {};
    consume(&tok);

    if(tok.type != TYPE_OP || tok.val.op != TOK_SEMICOLON)
        syntax_error();

    return PARSER_NOERR;
}

parser_err parse(Tree* tree, char* data, size_t data_sz)
{
    assert(tree && data);

    lexer_init(data, data_sz);
    TREE = tree;

    general(&TREE->root);

    tree_dump(TREE, "DUMP");

    return PARSER_NOERR;
}
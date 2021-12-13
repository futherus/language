#include <assert.h>

#include "parser.h"
#include "../lexer/lexer.h"
#include "../dumpsystem/dumpsystem.h"

static Tree* TREE = nullptr;

#define syntax_error(TOK)                               \
{                                                       \
    printf("SYNTAX ERROR: %s\n", demangle(TOK));        \
    printf("in %s (%d)\n", __FUNCTION__, __LINE__);     \
                                                        \
    tree_dump(TREE, "SYNTAX_ERROR\n");                  \
    exit(1);                                            \
}                                                       \

/*
    Grammar:
        Primary    ::= '!' Primary |
                       '(' Expression ')' |
                       TYPE_NUMBER |
                       TYPE_ID |                                                  //Variable                   //1)TYPE_ID->TYPE_VAR
                       TYPE_ID '>>' Primary |                                     //Array access               //1)TYPE_ID->TYPE_VAR
                       TYPE_ID '(' {Expression {',' Expression}*}? ')' |             //Function call              //1)TYPE_ID->TYPE_FUNC
                       TYPE_EMB  '(' Expression ')'                               //Embedded-function call

        Term       ::= Primary {['/' '*'] Primary}*
        Ariphmetic ::= Term {['+' '-'] Term}*
        Boolean    ::= Ariphmetic {['==' '>' '<' '<=' '>=', '!='] Ariphmetic}*
        Expression ::= Boolean {['||' '&&'] Boolean}*

        Conditional     ::= 'if'    '(' Expression ')' '{' {Statement}+ '}' {'else' '{' {Statement}+ '}'}?
        Cycle           ::= 'while' '(' Expression ')' '{' {Statement}+ '}'
        Terminational   ::= 'return' Expression ';'
        Assign          ::= {'const'}? TYPE_ID {'[' Expression ']'}? '=' '{' Expression {',' Expression}* '}' ';' |
                                                                         Expression ';'
        
        Statement  ::= Conditional |
                       Cycle  |
                       Terminational |
                       Assign |
                       Expression ';'

        Define     ::= TYPE_ID '(' {TYPE_ID {',' TYPE_ID}*}? ')' '{' {Statement}+ '}'  //1)TYPE_ID->TYPE_FUNC, 2+)TYPE_ID->TYPE_VAR

        General    ::= {Define | Assign}+
*/
 
static parser_err primary(Node** base);
static parser_err term(Node** base);
static parser_err expression(Node** base);
static parser_err conditional(Node** base);
static parser_err cycle(Node** base);
static parser_err terminational(Node** base);
static parser_err assign(Node** base);
static parser_err statement(Node** base);
static parser_err general(Node** base);

#define MK_AUX(BASE, AUX) ASSERT_RET$(!tree_add_wrap((BASE), TYPE_AUX,     (AUX)), PARSER_TREE_FAIL)
#define MK_KEY(BASE, KEY) ASSERT_RET$(!tree_add_wrap((BASE), TYPE_KEYWORD, (KEY)), PARSER_TREE_FAIL)
static inline tree_err tree_add_wrap(Node** base, token_type type, int val)
{
    Token tok    = {};
    tok.type     = type;
    tok.val.aux  = (token_auxiliary) val;

    return tree_add(TREE, base, &tok);
}

#define MK_NODE(BASE, TOK) ASSERT_RET$(!tree_add(TREE, (BASE), (TOK)), PARSER_TREE_FAIL)

static parser_err primary(Node** base)
{
    Token tok = {};
    consume(&tok);

    if(tok.type == TYPE_OP && tok.val.op == TOK_NOT)
    {
        MK_NODE(base, &tok);
        primary(&(*base)->right);
    }
    else if(tok.type == TYPE_OP && tok.val.op == TOK_LRPAR)
    {
        expression(base);

        consume(&tok);
        if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
            syntax_error(&tok);
    }
    else if(tok.type == TYPE_NUMBER)
    {
        MK_NODE(base, &tok);
    }
    else if(tok.type == TYPE_ID)
    {
        Token probe = {};
        peek(&probe, 0);
        
        if(probe.type == TYPE_OP && probe.val.op == TOK_LRPAR)
        {
            MK_AUX(base, TOK_CALL);
            MK_AUX(&(*base)->right, TOK_FUNCTION);

            tok.type = TYPE_FUNC;
            MK_NODE(&(*base)->right->left, &tok);
            
            base = &(*base)->right->right;

            consume(&tok);

            peek(&tok, 0);
            if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
            {
                MK_AUX(base, TOK_PARAMETER);
                expression(&(*base)->right);

                peek(&tok, 0);
                while(tok.type == TYPE_OP && tok.val.op == TOK_COMMA)
                {
                    consume(&tok);

                    Node* tmp = *base;
                    MK_AUX(base, TOK_PARAMETER);
                    expression(&(*base)->right);
                    (*base)->left = tmp;

                    peek(&tok, 0);
                }
            }

            consume(&tok);
            if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
                syntax_error(&tok);
        }
        else if(probe.type == TYPE_OP && probe.val.op == TOK_SHIFT)
        {
            tok.type = TYPE_VAR;
            MK_NODE(base, &tok);

            consume(&tok);
            primary(&(*base)->right);
        }
        else
        {
            tok.type = TYPE_VAR;
            MK_NODE(base, &tok);
        }
    }
    else if(tok.type == TYPE_EMBED)
    {
        MK_NODE(base, &tok);

        consume(&tok);
        if(tok.type != TYPE_OP || tok.val.op != TOK_LRPAR)
            syntax_error(&tok);
        
        expression(&(*base)->right);

        consume(&tok);
        if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
            syntax_error(&tok);
    }
    else
    {
        syntax_error(&tok);
    }

    return PARSER_NOERR;
}

static parser_err term(Node** base)
{
    primary(base);

    Token tok = {};
    peek(&tok, 0);

    while(tok.type == TYPE_OP && (tok.val.op == TOK_MUL || tok.val.op == TOK_DIV))
    {
        consume(&tok);

        Node* tmp = *base;

        MK_NODE(base, &tok);
        
        (*base)->left = tmp;
        primary(&(*base)->right);
        
        peek(&tok, 0);
    }
    
    return PARSER_NOERR;
}

static parser_err ariphmetic(Node** base)
{
    term(base);

    Token tok = {};
    peek(&tok, 0);

    while(tok.type == TYPE_OP && (tok.val.op == TOK_ADD || tok.val.op == TOK_SUB))
    {
        consume(&tok);

        Node* tmp = *base;

        MK_NODE(base, &tok);

        (*base)->left = tmp;
        term(&(*base)->right);

        peek(&tok, 0);
    }

    return PARSER_NOERR;
}

static parser_err boolean(Node** base)
{
    ariphmetic(base);

    Token tok = {};
    peek(&tok, 0);

    while(tok.type == TYPE_OP && (tok.val.op == TOK_EQ    || tok.val.op == TOK_NEQ ||
                                  tok.val.op == TOK_LESS  || tok.val.op == TOK_LEQ ||
                                  tok.val.op == TOK_GREAT || tok.val.op == TOK_GEQ ))
    {
        consume(&tok);

        Node* tmp = *base;

        MK_NODE(base, &tok);

        (*base)->left = tmp;
        ariphmetic(&(*base)->right);

        peek(&tok, 0);
    }

    return PARSER_NOERR;
}

static parser_err expression(Node** base)
{
    boolean(base);

    Token tok = {};
    peek(&tok, 0);

    while(tok.type == TYPE_OP && (tok.val.op == TOK_AND || tok.val.op == TOK_OR))
    {
        consume(&tok);

        Node* tmp = *base;

        MK_NODE(base, &tok);

        (*base)->left = tmp;
        boolean(&(*base)->right);

        peek(&tok, 0);
    }

    return PARSER_NOERR;
}

static parser_err assign(Node** base)
{
    Token tok = {};

    consume(&tok);
    if(tok.type == TYPE_KEYWORD && tok.val.key == TOK_CONST)
    {
        MK_NODE(base, &tok);
        consume(&tok);
    }

    if(tok.type != TYPE_ID)
        syntax_error(&tok);
    
    Node* tmp = *base;
    tok.type = TYPE_VAR;
    MK_NODE(base, &tok);
    (*base)->left = tmp;

    consume(&tok);
    if(tok.type == TYPE_OP && tok.val.op == TOK_LQPAR)
    {
        expression(&(*base)->right);

        consume(&tok);
        if(tok.type != TYPE_OP && tok.val.op != TOK_RQPAR)
            syntax_error(&tok);
        
        consume(&tok);
    }

    if(tok.type != TYPE_OP || tok.val.op != TOK_ASSIGN)
        syntax_error(&tok);
    
    tmp = *base;
    MK_NODE(base, &tok);
    (*base)->left = tmp;

    MK_AUX(&(*base)->right, TOK_INITIALIZER);

    peek(&tok, 0);
    if(tok.type == TYPE_OP && tok.val.op == TOK_LFPAR)
    {
        consume(&tok);

        expression(&(*base)->right->right);

        consume(&tok);
        while(tok.type == TYPE_OP && tok.val.op == TOK_COMMA)
        {
            tmp = (*base)->right;
            MK_AUX(&(*base)->right, TOK_INITIALIZER);

            expression(&(*base)->right->right);
            (*base)->right->left = tmp;

            consume(&tok);
        }

        if(tok.type != TYPE_OP || tok.val.op != TOK_RFPAR)
            syntax_error(&tok);
    }
    else
    {
        expression(&(*base)->right->right);
    }

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_SEMICOLON)
        syntax_error(&tok);
    
    return PARSER_NOERR;
}

static parser_err conditional(Node** base)
{
    assert(base);

    Token tok = {};
    consume(&tok);
    if(tok.type != TYPE_KEYWORD || tok.val.key != TOK_IF)
        syntax_error(&tok);
    
    MK_NODE(base, &tok);

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LRPAR)
        syntax_error(&tok);

    expression(&(*base)->left);

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
        syntax_error(&tok);
    
    MK_AUX(&(*base)->right, TOK_DECISION);
    base = &(*base)->right;

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LFPAR)
        syntax_error(&tok);
    
    peek(&tok, 0);
    while(tok.type != TYPE_OP && tok.val.op != TOK_RFPAR)
    {
        Node* tmp = (*base)->left;
        MK_AUX(&(*base)->left, TOK_STATEMENT);
        statement(&(*base)->left->right);
        (*base)->left->left = tmp;

        peek(&tok, 0);
    }
    
    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RFPAR)
        syntax_error(&tok);
    
    peek(&tok, 0);
    if(tok.type != TYPE_KEYWORD || tok.val.key != TOK_ELSE)
        return PARSER_NOERR;
    
    consume(&tok);
    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LFPAR)
        syntax_error(&tok);
    
    peek(&tok, 0);
    while(tok.type != TYPE_OP && tok.val.op != TOK_RFPAR)
    {
        Node* tmp = (*base)->right;
        MK_AUX(&(*base)->right, TOK_STATEMENT);
        statement(&(*base)->right->right);
        (*base)->right->left = tmp;

        peek(&tok, 0);
    }

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RFPAR)
        syntax_error(&tok);
    
    return PARSER_NOERR;
}

static parser_err cycle(Node** base)
{
    assert(base);

    Token tok = {};
    consume(&tok);
    if(tok.type != TYPE_KEYWORD || tok.val.key != TOK_WHILE)
        syntax_error(&tok);

    MK_NODE(base, &tok);

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LRPAR)
        syntax_error(&tok);
    
    expression(&(*base)->left);

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
        syntax_error(&tok);

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LFPAR)
        syntax_error(&tok);
        
    base = &(*base)->right;

    peek(&tok, 0);
    while(tok.type != TYPE_OP && tok.val.op != TOK_RFPAR)
    {
        Node* tmp = (*base);
        MK_AUX(base, TOK_STATEMENT);
        statement(&(*base)->right);
        (*base)->left = tmp;

        peek(&tok, 0);
    }

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RFPAR)
        syntax_error(&tok);

    return PARSER_NOERR;
}

static parser_err terminational(Node** base)
{
    assert(base);

    Token tok = {};
    consume(&tok);

    if(tok.type != TYPE_KEYWORD || tok.val.key != TOK_RETURN)
        syntax_error(&tok);
    
    MK_NODE(base, &tok);

    expression(&(*base)->right);

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_SEMICOLON)
        syntax_error(&tok);

    return PARSER_NOERR;
}

static parser_err statement(Node** base)
{
    assert(base);

    Token tok = {};
    peek(&tok, 0);

    if(tok.type == TYPE_KEYWORD)
    {
        if(tok.val.key == TOK_IF)
        {
            conditional(base);
            return PARSER_NOERR;
        }
        else if(tok.val.key == TOK_WHILE)
        {
            cycle(base);
            return PARSER_NOERR;
        }
        else if(tok.val.key == TOK_RETURN)
        {
            terminational(base);
            return PARSER_NOERR;
        }
        else if(tok.val.key == TOK_CONST)
        {
            assign(base);
            return PARSER_NOERR;
        }
    }
    
    if(tok.type == TYPE_ID)
    {
        peek(&tok, 1);
        if(tok.type == TYPE_OP && (tok.val.op == TOK_ASSIGN || tok.val.op == TOK_LQPAR))
        {
            assign(base);
            return PARSER_NOERR;
        }
    }
    
    expression(base);
    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_SEMICOLON)
        syntax_error(&tok);
    
    return PARSER_NOERR;
}

static parser_err define(Node** base)
{
    assert(base);

    MK_AUX(base, TOK_DEFINE);
    MK_AUX(&(*base)->left, TOK_FUNCTION);

    Token tok = {};
    consume(&tok);
    if(tok.type != TYPE_ID)
        syntax_error(&tok);
    
    tok.type = TYPE_FUNC;
    MK_NODE(&(*base)->left->left, &tok);
    
    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LRPAR)
        syntax_error(&tok);
    
    peek(&tok, 0);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
    {
        MK_AUX(&(*base)->left->right, TOK_PARAMETER);

        expression(&(*base)->left->right->right);
    }

    peek(&tok, 0);
    while(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
    {
        Node* tmp = (*base)->left->right;
        MK_AUX(&(*base)->left->right, TOK_PARAMETER);

        consume(&tok);
        if(tok.type != TYPE_OP || tok.val.op != TOK_COMMA)
            syntax_error(&tok);

        expression(&(*base)->left->right->right);
        (*base)->left->right->left = tmp;

        peek(&tok, 0);
    }

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
        syntax_error(&tok);

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LFPAR)
        syntax_error(&tok);

    peek(&tok, 0);
    while(tok.type != TYPE_OP || tok.val.op != TOK_RFPAR)
    {
        Node* tmp = (*base)->right;
        MK_AUX(&(*base)->right, TOK_STATEMENT);

        statement(&(*base)->right->right);
        (*base)->right->left = tmp;

        peek(&tok, 0);
    }

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RFPAR)
        syntax_error(&tok);

    return PARSER_NOERR;

}

static parser_err general(Node** base)
{
    assert(base);

    Token tok = {};
    Node* tmp = nullptr;

    peek(&tok, 0);
    while(tok.type != TYPE_EOF)
    {
        tmp = *base;
        MK_AUX(base, TOK_STATEMENT);
        (*base)->left = tmp;

        if(tok.type == TYPE_KEYWORD && tok.val.key == TOK_CONST)
        {
            assign(&(*base)->right);
            peek(&tok, 0);
            continue;
        }

        peek(&tok, 1);
        if(tok.type == TYPE_OP && tok.val.op == TOK_ASSIGN)
        {
            assign(&(*base)->right);
            peek(&tok, 0);
            continue;
        }

        define(&(*base)->right);
        peek(&tok, 0);
    }
    
    return PARSER_NOERR;
}

parser_err parse(Tree* tree, char* data, size_t data_sz)
{
    assert(tree && data);

    lexer_init(data, data_sz);
    TREE = tree;

    general(&TREE->root);

    lexer_dstr();
    tree_dump(TREE, "DUMP");

    return PARSER_NOERR;
}

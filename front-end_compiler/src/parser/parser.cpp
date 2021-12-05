#include <assert.h>

#include "parser.h"
#include "../lexer/lexer.h"
#include "../dumpsystem/dumpsystem.h"

static Tree* TREE = nullptr;

static void syntax_error()
{
    printf("SYNTAX ERROR");
    tree_dump(TREE, "SYNTAX_ERROR");
    exit(1);
}

/*
    Grammar:
        Primary    ::= '(' Expression ')' |
                       TYPE_NUMBER |
                       TYPE_ID |                                                  //Variable                   //1)TYPE_ID->TYPE_VAR
                       TYPE_ID '(' Expression {',' Expression}* ')' |             //Function call              //1)TYPE_ID->TYPE_FUNC
                       TYPE_EMB  '(' Expression ')'                               //Embedded-function call

        Term       ::= Primary {[/*] Primary}*

        Expression ::= Term {[+-] Term}*

        Conditional     ::= 'if'    '(' Expression ')' '{' {Statement}+ '}' {'else' '{' {Statement}+ '}'}?
        Cycle           ::= 'while' '(' Expression ')' '{' {Statement}+ '}'
        Terminational   ::= 'return' Expression ';'
        Assign          ::= {'const'}? TYPE_ID '=' Expression ';'                              //1)TYPE_ID->TYPE_VAR
        
        Statement  ::= Conditional |
                       Cycle  |
                       Terminational |
                       Assign |
                       // Expression ';' |

        //Define     ::= 'def' TYPE_ID '(' TYPE_ID {',' TYPE_ID}* ')' '{' {Statement}+ '}'  //1)TYPE_ID->TYPE_FUNC, 2+)TYPE_ID->TYPE_VAR
        //General    ::= {Define | Assign}+
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


#define MK_AUX(BASE, AUX) tree_add_wrap((BASE), TYPE_AUX, (AUX))
#define MK_KEY(BASE, KEY) tree_add_wrap((BASE), TYPE_KEYWORD, (KEY))
static inline tree_err tree_add_wrap(Node** base, token_type type, int val)
{
    Token tok    = {};
    tok.type     = type;
    tok.val.aux  = (token_auxiliary) val;

    return tree_add(TREE, base, &tok);
}

#define MK_NODE(BASE, TOK) tree_add(TREE, (BASE), (TOK))

static parser_err primary(Node** base)
{
    Token tok = {};
    consume(&tok);

    if(tok.type == TYPE_OP && tok.val.op == TOK_LRPAR)
    {
        expression(base);

        consume(&tok);
        if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
            syntax_error();
    }
    else if(tok.type == TYPE_NUMBER)
    {
        MK_NODE(base, &tok);
    }
    else if(tok.type == TYPE_ID)
    {
        Token probe = {};
        peek(&probe);
        
        if(probe.type == TYPE_OP && probe.val.op == TOK_LRPAR)
        {
            MK_AUX(base, TOK_CALL);
            MK_AUX(&(*base)->right, TOK_FUNCTION);

            tok.type = TYPE_FUNC;
            MK_NODE(&(*base)->right->left, &tok);
            
            base = &(*base)->right->right;

            consume(&tok);

            MK_AUX(base, TOK_PARAMETER);
            expression(&(*base)->right);

            consume(&tok);
            while(tok.type == TYPE_OP && tok.val.op == TOK_COMMA)
            {
                Node* tmp = *base;
                MK_AUX(base, TOK_PARAMETER);
                expression(&(*base)->right);
                (*base)->left = tmp;

                consume(&tok);
            }

            if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
                syntax_error();
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
            syntax_error();
        
        expression(&(*base)->right);

        consume(&tok);
        if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
            syntax_error();
    }
    else
    {
        syntax_error();
    }

    return PARSER_NOERR;
}

static parser_err term(Node** base)
{
    primary(base);

    Token tok = {};
    peek(&tok);

    while(tok.type == TYPE_OP && (tok.val.op == TOK_MUL || tok.val.op == TOK_DIV))
    {
        consume(&tok);

        Node* tmp = *base;

        MK_NODE(base, &tok);
        
        (*base)->left = tmp;
        primary(&(*base)->right);
        
        peek(&tok);
    }
    
    return PARSER_NOERR;
}

static parser_err expression(Node** base)
{
    term(base);

    Token tok = {};
    peek(&tok);

    while(tok.type == TYPE_OP && (tok.val.op == TOK_ADD || tok.val.op == TOK_SUB))
    {
        consume(&tok);

        Node* tmp = *base;

        MK_NODE(base, &tok);

        (*base)->left = tmp;
        term(&(*base)->right);

        peek(&tok);
    }

    return PARSER_NOERR;
}

static parser_err conditional(Node** base)
{
    assert(base);

    Token tok = {};
    consume(&tok);
    if(tok.type != TYPE_KEYWORD || tok.val.key != TOK_IF)
        syntax_error();
    
    MK_NODE(base, &tok);

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LRPAR)
        syntax_error();

    expression(&(*base)->right);

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
        syntax_error();
    
    MK_AUX(&(*base)->left, TOK_DECISION);
    base = &(*base)->left;

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LFPAR)
        syntax_error();
    
    peek(&tok);
    while(tok.type != TYPE_OP && tok.val.op != TOK_RFPAR)
    {
        Node* tmp = (*base)->right;
        MK_AUX(&(*base)->right, TOK_STATEMENT);
        statement(&(*base)->right->right);
        (*base)->right->left = tmp;

        peek(&tok);
    }
    
    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RFPAR)
        syntax_error();
    
    peek(&tok);
    if(tok.type != TYPE_KEYWORD || tok.val.key != TOK_ELSE)
        return PARSER_NOERR;
    
    consume(&tok);
    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LFPAR)
        syntax_error();
    
    peek(&tok);
    while(tok.type != TYPE_OP && tok.val.op != TOK_RFPAR)
    {
        Node* tmp = (*base)->left;
        MK_AUX(&(*base)->left, TOK_STATEMENT);
        statement(&(*base)->left->right);
        (*base)->left->left = tmp;

        peek(&tok);
    }

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RFPAR)
        syntax_error();
    
    return PARSER_NOERR;
}

static parser_err cycle(Node** base)
{
    assert(base);

    Token tok = {};
    consume(&tok);
    if(tok.type != TYPE_KEYWORD || tok.val.key != TOK_WHILE)
        syntax_error();

    MK_NODE(base, &tok);

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LRPAR)
        syntax_error();
    
    expression(&(*base)->right);

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
        syntax_error();

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LFPAR)
        syntax_error();
        
    base = &(*base)->left;

    peek(&tok);
    while(tok.type != TYPE_OP && tok.val.op != TOK_RFPAR)
    {
        Node* tmp = (*base);
        MK_AUX(base, TOK_STATEMENT);
        statement(&(*base)->right);
        (*base)->left = tmp;

        peek(&tok);
    }

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RFPAR)
        syntax_error();

    return PARSER_NOERR;
}

static parser_err terminational(Node** base)
{
    assert(base);

    Token tok = {};
    consume(&tok);

    if(tok.type != TYPE_KEYWORD || tok.val.key != TOK_RETURN)
        syntax_error();
    
    MK_NODE(base, &tok);

    expression(&(*base)->right);

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_SEMICOLON)
        syntax_error();

    return PARSER_NOERR;
}

static parser_err assign(Node** base)
{
    assert(base);

    Token tok = {};
    Token tmp = {};
    tmp.type = TYPE_VAR;

    consume(&tok);

    if(tok.type == TYPE_KEYWORD && tok.val.key == TOK_CONST)
    {
        tmp.type = TYPE_CONSTANT;
        consume(&tok);
    }
    
    if(tok.type != TYPE_ID)
        syntax_error();
    
    tmp.val.name = tok.val.name;
    tmp.name_len = tok.name_len;

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_ASSIGN)
        syntax_error();
    
    MK_NODE(base, &tok);
    MK_NODE(&(*base)->left, &tmp);

    expression(&(*base)->right);

    consume(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_SEMICOLON)
        syntax_error();

    return PARSER_NOERR;
}

static parser_err statement(Node** base)
{
    assert(base);

    Token tok = {};
    peek(&tok);

    if(tok.type == TYPE_KEYWORD)
    {
        if(tok.val.key == TOK_IF)
            conditional(base);
        else if(tok.val.key == TOK_WHILE)
            cycle(base);
        else if(tok.val.key == TOK_RETURN)
            terminational(base);
    }
    else
        assign(base);
    
    return PARSER_NOERR;
}

static parser_err general(Node** base)
{
    assert(base);

    statement(base);

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
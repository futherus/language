#include <assert.h>

#include "parser.h"
#include "lexer.h"
#include "../common/dumpsystem.h"

static Tree*         TREE_         = nullptr;
static Token_array*  TOKEN_ARRAY_  = nullptr;
static Dependencies* DEPENDENCIES_ = nullptr;

static void syntax_error_tokens()
{
        Token error_token = {};

        for(int iter = -2; iter < 3; iter++)
        {
            peek(&error_token, iter, TOKEN_ARRAY_); 
            fprintf(stderr, "%s ", demangle(&error_token));
        }

        fprintf(stderr, "\n");
}

#define syntax_error(MSG_, TOK_)                                                        \
do                                                                                      \
{                                                                                       \
    fprintf(stderr, "\x1b[31mSyntax error:\x1b[0m %s : %s\n", (MSG_), demangle(TOK_));  \
    FILE* stream_ = dumpsystem_get_opened_stream();                                     \
                                                                                        \
    if(stream_)                                                                         \
    {                                                                                   \
        fprintf(stream_, "<span class = \"error\">Syntax error: %s : %s\n</span>"       \
                         "\t\t\t\tat %s:%d:%s\n",                                       \
                         (MSG_), demangle(TOK_),                                        \
                         __FILE__, __LINE__, __PRETTY_FUNCTION__);                      \
                                                                                        \
        syntax_error_tokens();                                                          \
        return PARSER_SYNTAX_ERR;                                                       \
    }                                                                                   \
} while(0)                                                                              \

/*
    New Grammar:
        Primary    ::= 'дадаць' Primary       |
                       'за_недахопам' Primary |
                       'не' Primary           |
                       '(' Expression ')'     |
                       TYPE_NUMBER            |
                       TYPE_ID                |                                   //Variable
                       TYPE_ID 'зрушаны_на' Primary                    |          //Array access
                       TYPE_ID '(' {Expression {',' Expression}*}? ')' |          //Function call
                       TYPE_EMB  '(' Expression ')'                               //Embedded-function call

        Term       ::= Primary {['падзелены_на' 'памножаны_на'] Primary}*
        Ariphmetic ::= Term {['дадаць' 'за_недахопам'] Term}*
        Boolean    ::= Ariphmetic {['аднолькавы_з' 'драбнейшы_за' 'больш_чым' 'драбнейшы_ці_аднолькавы_з' 'большы_ці_аднолькавы_з', 'розьніцца_з'] Ariphmetic}*
        Expression ::= Boolean {['ці' 'і'] Boolean}*

        Conditional     ::= 'калі'    '(' Expression ')' '\\\\' {Statement}+ '////' {'інакш' '\\\\' {Statement}+ '////'}?
        Cycle           ::= 'пакуль' '(' Expression ')' '\\\\' {Statement}+ '////'
        Terminational   ::= 'вышпурнуць' Expression  'нарэшце'
        Show            ::= 'надрукаваць' 'нарэшце'
        Pixel           ::= 'пафарбаваць' '(' Expression ',' Expression ')' 'нарэшце'
        Assign          ::= {'непахісны'}? TYPE_ID {'[' Expression ']'}? 'апыняецца' Expression 'нарэшце' |                                                      

        Statement  ::= Conditional   |
                       Cycle         |
                       Terminational |
                       Assign        |
                       Show          |
                       Expression 'нарэшце'

        Define     ::= TYPE_ID '(' {{'непахісны'}? TYPE_ID {',' TYPE_ID}*}? ')' '{' {Statement}+ '}'

        General    ::= {Define | Assign}+
*/

/*
    Grammar:
        Primary    ::= '!' Primary |
                       '(' Expression ')' |
                       TYPE_NUMBER |
                       TYPE_ID |                                                  //Variable     
                       TYPE_ID '>>' Primary |                                     //Array access 
                       TYPE_ID '(' {Expression {',' Expression}*}? ')' |          //Function call
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

        Define     ::= TYPE_ID '(' {TYPE_ID {',' TYPE_ID}*}? ')' '{' {Statement}+ '}'

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

#define CONSUME(TOK)      consume((TOK), TOKEN_ARRAY_)
#define PEEK(TOK, OFFSET) peek((TOK), (OFFSET), TOKEN_ARRAY_)

#define MK_NODE(BASE, TOK) ASSERT_RET$(!tree_add(TREE_, (BASE), (TOK)), PARSER_TREE_FAIL)

#define MK_AUX(BASE, AUX) ASSERT_RET$(!tree_add_wrap((BASE), TYPE_AUX,     (AUX)), PARSER_TREE_FAIL)
#define MK_KEY(BASE, KEY) ASSERT_RET$(!tree_add_wrap((BASE), TYPE_KEYWORD, (KEY)), PARSER_TREE_FAIL)
static inline tree_err tree_add_wrap(Node** base, token_type type, int val)
{
    Token tok    = {};
    tok.type     = type;
    tok.val.aux  = (token_auxiliary) val;

    return tree_add(TREE_, base, &tok);
}

static parser_err primary(Node** base)
{
    LOG$("Entering");

    Token tok = {};

    CONSUME(&tok);

    if(tok.type == TYPE_OP && tok.val.op == TOK_NOT)
    {
        MK_NODE(base, &tok);
        PASS$(!primary(&(*base)->right), return PARSER_PASS_ERR; );
    }
    else if(tok.type == TYPE_OP && tok.val.op == TOK_ADD)
    {
        PASS$(!primary(base), return PARSER_PASS_ERR; );        
    }
    else if(tok.type == TYPE_OP && tok.val.op == TOK_SUB)
    {
        MK_NODE(base, &tok);

        tok.type = TYPE_NUMBER;
        tok.val.num = 0;
        MK_NODE(&(*base)->left, &tok);

        PASS$(!primary(&(*base)->right), return PARSER_PASS_ERR; );
    }
    else if(tok.type == TYPE_OP && tok.val.op == TOK_LRPAR)
    {
        PASS$(!expression(base), return PARSER_PASS_ERR; );

        CONSUME(&tok);
        if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
            syntax_error("Closing parenthesis expected (primary expression)",&tok);
    }
    else if(tok.type == TYPE_NUMBER)
    {
        MK_NODE(base, &tok);
    }
    else if(tok.type == TYPE_ID)
    {
        Token probe = {};

        PEEK(&probe, 0);
        
        if(probe.type == TYPE_OP && probe.val.op == TOK_LRPAR)
        {
            MK_AUX(base, TOK_CALL);

            MK_NODE(&(*base)->left, &tok);
            
            base = &(*base)->right;

            CONSUME(&tok);

            PEEK(&tok, 0);
            if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
            {
                MK_AUX(base, TOK_PARAMETER);
                PASS$(!expression(&(*base)->right), return PARSER_PASS_ERR; );

                PEEK(&tok, 0);
                while(tok.type == TYPE_OP && tok.val.op == TOK_COMMA)
                {
                    CONSUME(&tok);

                    Node* tmp = *base;
                    MK_AUX(base, TOK_PARAMETER);
                    PASS$(!expression(&(*base)->right), return PARSER_PASS_ERR; );
                    (*base)->left = tmp;

                    PEEK(&tok, 0);
                }
            }

            CONSUME(&tok);
            if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
                syntax_error("Closing parenthesis expected (function call)",&tok);
        }
        else if(probe.type == TYPE_OP && probe.val.op == TOK_SHIFT)
        {
            MK_NODE(base, &tok);

            CONSUME(&tok);
            PASS$(!primary(&(*base)->right), return PARSER_PASS_ERR; );
        }
        else
        {
            MK_NODE(base, &tok);
        }
    }
    else if(tok.type == TYPE_EMBED)
    {
        MK_NODE(base, &tok);

        CONSUME(&tok);
        if(tok.type != TYPE_OP || tok.val.op != TOK_LRPAR)
            syntax_error("Opening parenthesis expected (embedded)",&tok);
        
        PEEK(&tok, 0);
        if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
            PASS$(!expression(&(*base)->right), return PARSER_PASS_ERR; );
                
        PEEK(&tok, 0);
        if(tok.type == TYPE_OP && tok.val.op == TOK_COMMA)
        {
            CONSUME(&tok);
            (*base)->left = (*base)->right;
            PASS$(!expression(&(*base)->right), return PARSER_PASS_ERR; );
        }

        CONSUME(&tok);
        if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
            syntax_error("Closing parenthesis expected (embedded)",&tok);
    }
    else
    {
        syntax_error("Primary expression expected", &tok);
    }

    return PARSER_NOERR;
}

static parser_err power(Node** base)
{
    LOG$("Entering");

    PASS$(!primary(base), return PARSER_PASS_ERR; );

    Token tok = {};
    PEEK(&tok, 0);
    
    if(tok.type == TYPE_OP && tok.val.op == TOK_POWER)
    {
        CONSUME(&tok);

        Node* tmp = *base;

        MK_NODE(base, &tok);

        (*base)->left = tmp;
        PASS$(!primary(&(*base)->right), return PARSER_PASS_ERR; );
    }

    return PARSER_NOERR;
}

static parser_err term(Node** base)
{
    LOG$("Entering");

    PASS$(!power(base), return PARSER_PASS_ERR; );

    Token tok = {};
    PEEK(&tok, 0);

    while(tok.type == TYPE_OP && (tok.val.op == TOK_MUL || tok.val.op == TOK_DIV))
    {
        CONSUME(&tok);

        Node* tmp = *base;

        MK_NODE(base, &tok);
        
        (*base)->left = tmp;
        PASS$(!power(&(*base)->right), return PARSER_PASS_ERR; );
        
        PEEK(&tok, 0);
    }
    
    return PARSER_NOERR;
}

static parser_err ariphmetic(Node** base)
{
    LOG$("Entering");

    PASS$(!term(base), return PARSER_PASS_ERR; );

    Token tok = {};
    PEEK(&tok, 0);

    while(tok.type == TYPE_OP && (tok.val.op == TOK_ADD || tok.val.op == TOK_SUB))
    {
        CONSUME(&tok);

        Node* tmp = *base;

        MK_NODE(base, &tok);

        (*base)->left = tmp;
        PASS$(!term(&(*base)->right), return PARSER_PASS_ERR; );

        PEEK(&tok, 0);
    }

    return PARSER_NOERR;
}

static parser_err boolean(Node** base)
{
    LOG$("Entering");

    PASS$(!ariphmetic(base), return PARSER_PASS_ERR; );

    Token tok = {};
    PEEK(&tok, 0);

    while(tok.type == TYPE_OP && (tok.val.op == TOK_EQ    || tok.val.op == TOK_NEQ ||
                                  tok.val.op == TOK_LESS  || tok.val.op == TOK_LEQ ||
                                  tok.val.op == TOK_GREAT || tok.val.op == TOK_GEQ ))
    {
        CONSUME(&tok);

        Node* tmp = *base;

        MK_NODE(base, &tok);

        (*base)->left = tmp;
        PASS$(!ariphmetic(&(*base)->right), return PARSER_PASS_ERR; );

        PEEK(&tok, 0);
    }

    return PARSER_NOERR;
}

static parser_err expression(Node** base)
{
    LOG$("Entering");

    PASS$(!boolean(base), return PARSER_PASS_ERR; );

    Token tok = {};
    PEEK(&tok, 0);

    while(tok.type == TYPE_OP && (tok.val.op == TOK_AND || tok.val.op == TOK_OR))
    {
        CONSUME(&tok);

        Node* tmp = *base;

        MK_NODE(base, &tok);

        (*base)->left = tmp;
        PASS$(!boolean(&(*base)->right), return PARSER_PASS_ERR; );

        PEEK(&tok, 0);
    }

    return PARSER_NOERR;
}

static parser_err assign(Node** base)
{
    LOG$("Entering");

    Token tok = {};

    CONSUME(&tok);
    if(tok.type == TYPE_KEYWORD && tok.val.key == TOK_CONST)
    {
        MK_NODE(base, &tok);
        CONSUME(&tok);
    }

    if(tok.type != TYPE_ID)
        syntax_error("Assignment requires lvalue",&tok);
    
    Node* tmp = *base;
    MK_NODE(base, &tok);
    (*base)->left = tmp;

    CONSUME(&tok);
    if(tok.type == TYPE_OP && tok.val.op == TOK_LQPAR)
    {
        PASS$(!expression(&(*base)->right), return PARSER_PASS_ERR; );

        CONSUME(&tok);
        if(tok.type != TYPE_OP && tok.val.op != TOK_RQPAR)
            syntax_error("Closing parenthesis expected (array access)",&tok);
        
        CONSUME(&tok);
    }

    if(tok.type != TYPE_OP || tok.val.op != TOK_ASSIGN)
        syntax_error("'=' operator expected",&tok);
    
    tmp = *base;
    MK_NODE(base, &tok);
    (*base)->left = tmp;

    PASS$(!expression(&(*base)->right), return PARSER_PASS_ERR; );

    CONSUME(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_SEMICOLON)
        syntax_error("Terminational literal expected",&tok);
    
    return PARSER_NOERR;
}

static parser_err conditional(Node** base)
{
    LOG$("Entering");

    assert(base);

    Token tok = {};
    CONSUME(&tok);
    assert(tok.type == TYPE_KEYWORD && tok.val.key == TOK_IF);
    
    MK_NODE(base, &tok);

    CONSUME(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LRPAR)
        syntax_error("Opening parenthesis expected (condition)",&tok);

    PASS$(!expression(&(*base)->left), return PARSER_PASS_ERR; );

    CONSUME(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
        syntax_error("Closing parenthesis expected (condition)",&tok);
    
    MK_AUX(&(*base)->right, TOK_DECISION);
    base = &(*base)->right;

    CONSUME(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LFPAR)
        syntax_error("Opening parenthesis expected (body of positive)",&tok);
    
    PEEK(&tok, 0);
    while(tok.type != TYPE_OP || tok.val.op != TOK_RFPAR)
    {
        Node* tmp = (*base)->left;
        MK_AUX(&(*base)->left, TOK_STATEMENT);
        PASS$(!statement(&(*base)->left->right), return PARSER_PASS_ERR; );
        (*base)->left->left = tmp;

        PEEK(&tok, 0);
    }
    
    CONSUME(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RFPAR)
        syntax_error("Closing parenthesis expected (body of positive)",&tok);
    
    PEEK(&tok, 0);
    if(tok.type != TYPE_KEYWORD || tok.val.key != TOK_ELSE)
        return PARSER_NOERR;
    
    CONSUME(&tok);
    CONSUME(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LFPAR)
        syntax_error("Opening parenthesis expected (body of negative)",&tok);
    
    PEEK(&tok, 0);
    while(tok.type != TYPE_OP || tok.val.op != TOK_RFPAR)
    {
        Node* tmp = (*base)->right;
        MK_AUX(&(*base)->right, TOK_STATEMENT);
        PASS$(!statement(&(*base)->right->right), return PARSER_PASS_ERR; );
        (*base)->right->left = tmp;

        PEEK(&tok, 0);
    }

    CONSUME(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RFPAR)
        syntax_error("Closing parenthesis expected (body of negative)",&tok);
    
    return PARSER_NOERR;
}

static parser_err cycle(Node** base)
{
    LOG$("Entering");

    assert(base);

    Token tok = {};
    CONSUME(&tok);
    assert(tok.type == TYPE_KEYWORD && tok.val.key == TOK_WHILE);

    MK_NODE(base, &tok);

    CONSUME(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LRPAR)
        syntax_error("Opening parenthesis expected (condition of loop)",&tok);
    
    PASS$(!expression(&(*base)->left), return PARSER_PASS_ERR; );

    CONSUME(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
        syntax_error("Closing parenthesis expected (condition of loop)",&tok);

    CONSUME(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LFPAR)
        syntax_error("Opening parenthesis expected (body of loop)",&tok);
        
    base = &(*base)->right;

    PEEK(&tok, 0);
    while(tok.type != TYPE_OP || tok.val.op != TOK_RFPAR)
    {
        Node* tmp = (*base);
        MK_AUX(base, TOK_STATEMENT);
        PASS$(!statement(&(*base)->right), return PARSER_PASS_ERR; );
        (*base)->left = tmp;

        PEEK(&tok, 0);
    }

    CONSUME(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RFPAR)
        syntax_error("Closing parenthesis expected (body of loop)",&tok);

    return PARSER_NOERR;
}

static parser_err terminational(Node** base)
{
    LOG$("Entering");

    assert(base);

    Token tok = {};
    CONSUME(&tok);

    assert(tok.type == TYPE_KEYWORD && tok.val.key == TOK_RETURN);
    
    MK_NODE(base, &tok);

    PASS$(!expression(&(*base)->right), return PARSER_PASS_ERR; );

    CONSUME(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_SEMICOLON)
        syntax_error("Terminational literal expected",&tok);

    return PARSER_NOERR;
}

static parser_err statement(Node** base)
{
    LOG$("Entering");

    assert(base);

    Token tok = {};
    PEEK(&tok, 0);

    if(tok.type == TYPE_KEYWORD)
    {
        if(tok.val.key == TOK_IF)
        {
            PASS$(!conditional(base), return PARSER_PASS_ERR; );
            return PARSER_NOERR;
        }
        else if(tok.val.key == TOK_WHILE)
        {
            PASS$(!cycle(base), return PARSER_PASS_ERR; );
            return PARSER_NOERR;
        }
        else if(tok.val.key == TOK_RETURN)
        {
            PASS$(!terminational(base), return PARSER_PASS_ERR; );
            return PARSER_NOERR;
        }
        else if(tok.val.key == TOK_CONST)
        {
            PASS$(!assign(base), return PARSER_PASS_ERR; );
            return PARSER_NOERR;
        }
    }
    
    if(tok.type == TYPE_ID)
    {
        PEEK(&tok, 1);
        if(tok.type == TYPE_OP && (tok.val.op == TOK_ASSIGN || tok.val.op == TOK_LQPAR))
        {
            PASS$(!assign(base), return PARSER_PASS_ERR; );
            return PARSER_NOERR;
        }
    }
    
    PASS$(!expression(base), return PARSER_PASS_ERR; );
    CONSUME(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_SEMICOLON)
        syntax_error("Terminational literal expected",&tok);
    
    return PARSER_NOERR;
}

static parser_err define(Node** base)
{
    LOG$("Entering");

    assert(base);

    MK_AUX(base, TOK_DEFINE);
    MK_AUX(&(*base)->left, TOK_FUNCTION);

    Token tok = {};
    CONSUME(&tok);
    if(tok.type != TYPE_ID)
        syntax_error("Function name expected",&tok);
    
    MK_NODE(&(*base)->left->left, &tok);
    
    CONSUME(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LRPAR)
        syntax_error("Opening parenthesis expected (function parameters)",&tok);
    
    PEEK(&tok, 0);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
    {
        MK_AUX(&(*base)->left->right, TOK_PARAMETER);

        PASS$(!expression(&(*base)->left->right->right), return PARSER_PASS_ERR; );
    }

    PEEK(&tok, 0);
    while(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
    {
        Node* tmp = (*base)->left->right;
        MK_AUX(&(*base)->left->right, TOK_PARAMETER);

        CONSUME(&tok);
        if(tok.type != TYPE_OP || tok.val.op != TOK_COMMA)
            syntax_error("Expected comma (function parameters)",&tok);

        PASS$(!expression(&(*base)->left->right->right), return PARSER_PASS_ERR; );
        (*base)->left->right->left = tmp;

        PEEK(&tok, 0);
    }

    CONSUME(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
        syntax_error("Closing parenthesis expected (function parameters)",&tok);

    CONSUME(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LFPAR)
        syntax_error("Opening parenthesis expected (function body)",&tok);

    PEEK(&tok, 0);
    while(tok.type != TYPE_OP || tok.val.op != TOK_RFPAR)
    {
        Node* tmp = (*base)->right;
        MK_AUX(&(*base)->right, TOK_STATEMENT);

        PASS$(!statement(&(*base)->right->right), return PARSER_PASS_ERR; );
        (*base)->right->left = tmp;

        PEEK(&tok, 0);
    }

    CONSUME(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RFPAR)
        syntax_error("Closing parenthesis expected (function body)",&tok);

    return PARSER_NOERR;
}

static parser_err directive()
{
    LOG$("Entering");

    assert(DEPENDENCIES_);
    
    Dep dep = {};
    Token tok = {};

    CONSUME(&tok);
    if(tok.type != TYPE_DIRECTIVE_BEGIN)
        syntax_error("Begin of directive expected", &tok);

    CONSUME(&tok);
    if(tok.type != TYPE_ID)
        syntax_error("Function name expected (directive)", &tok);
        
    dep.func = tok;
    
    CONSUME(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LRPAR)
        syntax_error("Opening parenthesis expected (directive)", &tok);
    
    PEEK(&tok, 0);
    if(tok.type == TYPE_ID)
    {
        CONSUME(&tok);
        dep.n_args++;
    }

    PEEK(&tok, 0);
    while(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
    {
        CONSUME(&tok);
        if(tok.type != TYPE_OP || tok.val.op != TOK_COMMA)
            syntax_error("Expected comma (directive)", &tok);

        CONSUME(&tok);
        if(tok.type != TYPE_ID)
            syntax_error("Expected comma (directive)", &tok);

        dep.n_args++;
        PEEK(&tok, 0);
    }

    CONSUME(&tok);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
        syntax_error("Closing parenthesis expected (directive)", &tok);
    
    int err = dep_add(DEPENDENCIES_, dep);
    if(err == DEP_ALREADY_INSERTED)
        syntax_error("Redeclaration of function (directive)", &tok);
    
    ASSERT_RET$(!err, PARSER_PASS_ERR);

    CONSUME(&tok);
    if(tok.type != TYPE_DIRECTIVE_END)
        syntax_error("End of directive expected", &tok);

    return PARSER_NOERR;
}

static parser_err general(Node** base)
{
    LOG$("Entering");

    assert(base);

    Token tok = {};
    Node* tmp = nullptr;

    PEEK(&tok, 0);
    while(tok.type != TYPE_EOF)
    {
        if(tok.type == TYPE_DIRECTIVE_BEGIN)
        {
            PASS$(!directive(), return PARSER_PASS_ERR; );
            PEEK(&tok, 0);
            continue;
        }

        tmp = *base;
        MK_AUX(base, TOK_STATEMENT);
        (*base)->left = tmp;

        if(tok.type == TYPE_KEYWORD && tok.val.key == TOK_CONST)
        {
            PASS$(!assign(&(*base)->right), return PARSER_PASS_ERR; );
            PEEK(&tok, 0);
            continue;
        }

        PEEK(&tok, 1);
        if(tok.type == TYPE_OP && (tok.val.op == TOK_ASSIGN || tok.val.op == TOK_LQPAR))
        {
            PASS$(!assign(&(*base)->right), return PARSER_PASS_ERR; );
            PEEK(&tok, 0);
            continue;
        }

        PASS$(!define(&(*base)->right), return PARSER_PASS_ERR; );
        PEEK(&tok, 0);
    }
    
    return PARSER_NOERR;
}

parser_err parse(Tree* tree, Dependencies* deps, Token_array* tok_arr)
{
    assert(tree && tok_arr);

    TREE_         = tree;
    TOKEN_ARRAY_  = tok_arr;
    DEPENDENCIES_ = deps;

    MSG$("\n\n----------------------Parsing started----------------------\n\n");

    parser_err err = general(&TREE_->root);

    MSG$("\n\n----------------------Parsing finished----------------------\n\n");

    if(err)
        token_array_dump(tok_arr);
        
    tree_dump(TREE_, "DUMP");

    TREE_         = nullptr;
    TOKEN_ARRAY_  = nullptr;
    DEPENDENCIES_ = nullptr;

    return err;
}

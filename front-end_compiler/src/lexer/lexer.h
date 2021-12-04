#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

enum lexer_err
{
    LEXER_NOERR          = 0,
    LEXER_PASS_ERR       = 1,
    LEXER_NULLPTR        = 2,
    LEXER_STACK_FAIL     = 3,
    LEXER_ARRAY_FAIL     = 4,
    LEXER_UNKNWN_SYMB    = 5,
    LEXER_NOTFOUND       = 6,
    LEXER_NAME_EXISTS    = 7,
    LEXER_RESERVED_NAMES = 8,
    LEXER_EMPTY_DATA     = 9,
};

enum token_type
{
    TYPE_EOF      = -1,
    TYPE_NOTYPE   =  0,
    TYPE_OP       =  1,
    TYPE_EMBED    =  2,
    TYPE_KEYWORD  =  3,
    TYPE_CONSTANT =  4,
    TYPE_NUMBER   =  5,
    TYPE_VAR      =  6,
    TYPE_FUNC     =  7,
    TYPE_ID       =  8,
};

#define DEF_OP(HASH, NAME, STD_NAME, MANGLE) TOK_##MANGLE,
enum token_operators
{
    #include "../reserved_operators.inc"
};
#undef DEF_OP

#define DEF_KEY(HASH, NAME, STD_NAME, MANGLE) TOK_##MANGLE,
enum token_keywords
{
    #include "../reserved_keywords.inc"
};
#undef DEF_KEY

#define DEF_EMB(HASH, NAME, STD_NAME, MANGLE) TOK_##MANGLE,
enum token_embedded
{
    #include "../reserved_embedded.inc"
};
#undef DEF_EMB

struct Token 
{
    token_type type = TYPE_NOTYPE;
    
    union Value
    {
        double num;
        char*  name;
        token_keywords  key;
        token_operators op;
        token_embedded  emb;
    } val = {};

    int name_len = 0;
};

lexer_err lexer_init(char* data, size_t data_sz);
lexer_err lexer_dstr();

lexer_err consume(Token* tok);

lexer_err peek(Token* tok);

char* demangle(const Token* tok);

#endif // LEXER_H

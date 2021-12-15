#ifndef TOKEN_H
#define TOKEN_H

#include <stdint.h>

enum token_type
{
    TYPE_EOF      = -1,
    TYPE_NOTYPE   =  0,
    TYPE_OP       =  1,
    TYPE_EMBED    =  2,
    TYPE_KEYWORD  =  3,
    TYPE_AUX      =  4,
    TYPE_VAR      =  5,
    TYPE_NUMBER   =  7,
    TYPE_FUNC     =  8,
    TYPE_ID       =  9,
};

#define DEF_OP(NAME, STD_NAME, MANGLE) TOK_##MANGLE,
enum token_operators
{
    #include "../reserved_operators.inc"
};
#undef DEF_OP

#define DEF_KEY(NAME, STD_NAME, MANGLE) TOK_##MANGLE,
enum token_keywords
{
    #include "../reserved_keywords.inc"
};
#undef DEF_KEY

#define DEF_EMB(NAME, STD_NAME, MANGLE) TOK_##MANGLE,
enum token_embedded
{
    #include "../reserved_embedded.inc"
};
#undef DEF_EMB

#define DEF_AUX(NAME, MANGLE) TOK_##MANGLE,
enum token_auxiliary
{
    #include "../reserved_auxiliary.inc"
};
#undef DEF_AUX

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
        token_auxiliary aux;
    } val = {};
};

struct Token_array
{
    Token* data = nullptr;
    ptrdiff_t size = 0;
    ptrdiff_t cap  = 0;

    ptrdiff_t pos  = 0;
    ptrdiff_t eof  = 0;
};

struct Token_nametable 
{
    char** name_arr = nullptr;

    ptrdiff_t size = 0;
    ptrdiff_t cap  = 0;
};

enum token_err
{
    TOKEN_NOERR = 0,
    TOKEN_BAD_ALLOC = 1,
    TOKEN_EMPTY_DATA = 2,
    TOKEN_UNKNWN_SYMB = 3,
};

token_err token_array_add(Token_array* tok_arr, const Token* token);
void      token_array_dstr(Token_array* tok_arr);

void      token_array_dump(Token_array* tok_arr);
void      token_nametable_dump(Token_nametable* tok_table);
void      token_dump_init(FILE* dump_stream);

token_err token_nametable_add(Token_nametable* tok_table, char** dst_ptr, const char name[], ptrdiff_t name_sz);
void      token_nametable_dstr(Token_nametable* tok_table);

char*     demangle(const Token* tok);

#endif // TOKEN_H
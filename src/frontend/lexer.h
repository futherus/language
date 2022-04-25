#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>
#include <stdint.h>

#include "../token/Token.h"

enum lexer_err
{
    LEXER_NOERR          = 0,
    LEXER_LEXICAL_ERROR  = 1,
    LEXER_EMPTY_DATA     = 2,
    LEXER_BAD_ALLOC      = 3,
};

lexer_err lexer(Token_array* tok_arr, Token_nametable* tok_table, const char data[], ptrdiff_t data_sz);

void consume(Token* tok, Token_array* tok_arr);
void peek(Token* tok, ptrdiff_t offset, Token_array* tok_arr);

#endif // LEXER_H

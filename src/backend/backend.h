#ifndef BACKEND_H
#define BACKEND_H

#include "../tree/Tree.h"

enum backend_err
{
    BACKEND_NOERR          = 0,
    BACKEND_FORMAT_ERROR   = 1,
    BACKEND_SEMANTIC_ERROR = 2,
    BACKEND_ARGS_ERROR     = 3,
    BACKEND_LEXER_FAIL     = 4,
    BACKEND_PARSER_FAIL    = 5,
    BACKEND_READ_FAIL      = 6,
    BACKEND_INFILE_FAIL    = 7,
    BACKEND_BAD_ALLOC      = 8,
    BACKEND_WRITE_FAIL     = 9,
    BACKEND_OUTFILE_FAIL   = 10,
    BACKEND_TREE_FAIL      = 11,
};

backend_err tree_read(Tree* tree, Token_nametable* tok_table, const char data[], ptrdiff_t data_sz);

#endif // BACKEND_H
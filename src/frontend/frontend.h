#ifndef TREE_WRITE_H
#define TREE_WRITE_H

#include "../tree/Tree.h"

enum frontend_err
{
    FRONTEND_NOERR = 0,
    FRONTEND_SYNTAX_ERROR = 1,
    FRONTEND_LEXER_FAIL = 2,
    FRONTEND_PARSER_FAIL = 3,
    FRONTEND_READ_FAIL = 4,
    FRONTEND_BAD_ALLOC = 5,
    FRONTEND_WRITE_FAIL = 6,
};

void tree_write(Tree* tree, FILE* ostream);

#endif // TREE_WRITE_H
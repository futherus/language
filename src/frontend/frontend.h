#ifndef FRONTEND_H
#define FRONTEND_H

#include "../tree/Tree.h"

enum frontend_err
{
    FRONTEND_NOERR        = 0,
    FRONTEND_SYNTAX_ERROR = 1,
    FRONTEND_ARGS_ERROR   = 2,
    FRONTEND_LEXER_FAIL   = 3,
    FRONTEND_PARSER_FAIL  = 4,
    FRONTEND_READ_FAIL    = 5,
    FRONTEND_INFILE_FAIL  = 6,
    FRONTEND_BAD_ALLOC    = 7,
    FRONTEND_WRITE_FAIL   = 8,
    FRONTEND_OUTFILE_FAIL = 9,
};

#endif // FRONTEND_H
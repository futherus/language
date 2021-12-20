#ifndef TRANSPILER_H
#define TRANSPILER_H

#include "../tree/Tree.h"

enum generator_err
{
    GENERATOR_NOERR = 0,
    GENERATOR_PASS_ERROR = 1,
    GENERATOR_SEMANTIC_ERROR = 2,
    GENERATOR_FORMAT_ERROR = 3,
    GENERATOR_BAD_ALLOC = 4,
};

enum transpiler_err
{
    TRANSP_NOERR          = 0,
    TRANSP_FORMAT_ERROR   = 1,
    TRANSP_SEMANTIC_ERROR = 2,
    TRANSP_READ_FAIL      = 3,
    TRANSP_INFILE_FAIL    = 4,
    TRANSP_BAD_ALLOC      = 5,
    TRANSP_GENERATOR_FAIL = 6,
};

generator_err generator(Tree* tree, FILE* ostream);

#endif // TRANSPILER_H
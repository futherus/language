#ifndef TRANSPILER_H
#define TRANSPILER_H

#include "../tree/Tree.h"

enum degenerator_err
{
    DEGENERATOR_NOERR = 0,
    DEGENERATOR_PASS_ERROR = 1,
    DEGENERATOR_SEMANTIC_ERROR = 2,
    DEGENERATOR_FORMAT_ERROR = 3,
    DEGENERATOR_BAD_ALLOC = 4,
};

enum transpiler_err
{
    TRANSP_NOERR          = 0,
    TRANSP_FORMAT_ERROR   = 1,
    TRANSP_SEMANTIC_ERROR = 2,
    TRANSP_READ_FAIL      = 3,
    TRANSP_INFILE_FAIL    = 4,
    TRANSP_BAD_ALLOC      = 5,
    TRANSP_DEGENERATOR_FAIL = 6,
};

degenerator_err degenerator(Tree* tree, FILE* ostream);

#endif // TRANSPILER_H
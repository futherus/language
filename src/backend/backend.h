#ifndef BACKEND_H
#define BACKEND_H

#include "../tree/Tree.h"

enum backend_err
{
    BACKEND_NOERR          = 0,
    BACKEND_FORMAT_ERROR   = 1,
    BACKEND_SEMANTIC_ERROR = 2,
    BACKEND_READ_FAIL      = 3,
    BACKEND_INFILE_FAIL    = 4,
    BACKEND_BAD_ALLOC      = 5,
    BACKEND_GENERATOR_FAIL = 6,
};

#endif // BACKEND_H
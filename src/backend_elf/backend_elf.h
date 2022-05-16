#ifndef BACKEND_ELF
#define BACKEND_ELF

#include "../tree/Tree.h"

enum backend_err
{
    BACKEND_ELF_NOERR          = 0,
    BACKEND_ELF_FORMAT_ERROR   = 1,
    BACKEND_ELF_SEMANTIC_ERROR = 2,
    BACKEND_ELF_READ_FAIL      = 3,
    BACKEND_ELF_INFILE_FAIL    = 4,
    BACKEND_ELF_OUTFILE_FAIL   = 5,
    BACKEND_ELF_BAD_ALLOC      = 6,
    BACKEND_ELF_GENERATOR_FAIL = 7,
};

#endif // BACKEND_ELF
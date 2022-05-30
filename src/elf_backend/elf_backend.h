#ifndef ELF_BACKEND
#define ELF_BACKEND

#include "../tree/Tree.h"

enum elf_backend_err
{
    ELF_BACKEND_NOERR          = 0,
    ELF_BACKEND_FORMAT_ERROR   = 1,
    ELF_BACKEND_SEMANTIC_ERROR = 2,
    ELF_BACKEND_READ_FAIL      = 3,
    ELF_BACKEND_INFILE_FAIL    = 4,
    ELF_BACKEND_OUTFILE_FAIL   = 5,
    ELF_BACKEND_BAD_ALLOC      = 6,
    ELF_BACKEND_GENERATOR_FAIL = 7,
};

#endif // ELF_BACKEND

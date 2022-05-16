#ifndef GENERATOR_H
#define GENERATOR_H

#include "../tree/Tree.h"
#include "elf.h"

enum generator_err
{
    GENERATOR_NOERR = 0,
    GENERATOR_PASS_ERROR = 1,
    GENERATOR_SEMANTIC_ERROR = 2,
    GENERATOR_FORMAT_ERROR = 3,
    GENERATOR_BAD_ALLOC = 4,
};

generator_err generator(Tree* tree, Binary* bin);

#endif // GENERATOR_H
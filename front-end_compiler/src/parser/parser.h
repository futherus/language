#ifndef PARSER_H
#define PARSER_H

#include "../tree/Tree.h"

enum parser_err
{
    PARSER_NOERR = 0,
    PARSER_PASS_ERR = 1,
    PARSER_TREE_FAIL = 2,
};

parser_err parse(Tree* tree, char* data, size_t data_sz);

#endif // PARSER_H
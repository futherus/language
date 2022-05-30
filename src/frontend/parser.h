#ifndef PARSER_H
#define PARSER_H

#include "../tree/Tree.h"
#include "../common/depend.h"

enum parser_err
{
    PARSER_NOERR = 0,
    PARSER_SYNTAX_ERR = 1,
    PARSER_PASS_ERR = 2,
    PARSER_TREE_FAIL = 3,
};

parser_err parse(Tree* tree, Dependencies* deps, Token_array* tok_arr);

#endif // PARSER_H

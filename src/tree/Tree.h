#ifndef TREE_H
#define TREE_H

#include <stdint.h>
#include <stdio.h>

#include "../token/Token.h"

const ptrdiff_t TREE_PTR_ARR_MIN_CAP = 8;
const ptrdiff_t TREE_CHUNK_SIZE      = 512;

struct Node
{
    Token tok = {};

    Node* left  = 0;
    Node* right = 0;
};

struct Tree
{
    Node**    ptr_arr     = nullptr;
    ptrdiff_t ptr_arr_cap = 0;

    ptrdiff_t size = 0;
    ptrdiff_t cap  = 0;

    Node*     root = nullptr;

    char**    id_arr     = nullptr;
    ptrdiff_t id_arr_sz  = 0;
    ptrdiff_t id_arr_cap = 0;
};

enum tree_err
{
    TREE_NOERR      = 0,
    TREE_NULLPTR    = 1,
    TREE_BAD_ALLOC  = 2,
    TREE_REINIT     = 3,
    TREE_NOTINIT    = 4,
    TREE_STACK_FAIL = 5,
    TREE_READ_FAIL  = 6,
};

tree_err tree_dstr(Tree* tree);

tree_err tree_add(Tree* tree, Node** base_ptr, const Token* data);
tree_err tree_copy(Tree* tree, Node** base_ptr, Node* origin);

tree_err tree_visitor(Tree* tree, void (*function)(Node* node, size_t depth));

void     tree_dump_init(FILE* dumpstream = nullptr);
void     tree_dump(Tree* tree, const char msg[], tree_err errcode = TREE_NOERR);

#endif // TREE_H
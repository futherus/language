#ifndef TREE_H
#define TREE_H

#include <stddef.h>
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
};

enum tree_err
{
    TREE_NOERR        = 0,
    TREE_READ_FAIL    = 1,
    TREE_BAD_ALLOC    = 2,
    TREE_FORMAT_ERROR = 3,
};

tree_err tree_dstr(Tree* tree);

tree_err tree_add(Tree* tree, Node** base_ptr, const Token* data);
tree_err tree_copy(Tree* tree, Node** base_ptr, Node* origin);

tree_err tree_visitor(Tree* tree, void (*function)(Node* node, size_t depth));

tree_err tree_read(Tree* tree, Token_nametable* tok_table, const char data[], ptrdiff_t data_sz);
void     tree_write(Tree* tree, FILE* ostream);

void     tree_dump_init(FILE* dumpstream = nullptr);
void     tree_dump(Tree* tree, const char msg[], tree_err errcode = TREE_NOERR);

#endif // TREE_H
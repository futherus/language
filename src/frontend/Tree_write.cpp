#include <stdlib.h>
#include <assert.h>

#include "frontend.h"

#define PRINT(format, ...) fprintf(stream, format, ##__VA_ARGS__)

static void tree_print_node_(Node* node, FILE* stream)
{
    assert(stream);

    static int depth = 0;
    bool depth_rised = false;

    if(node->tok.type == TYPE_KEYWORD)
    {
        if(node->tok.val.key == TOK_IF || node->tok.val.key == TOK_WHILE)
        {
            depth++;
            depth_rised = true;
        }
    }
    else if (node->tok.type == TYPE_AUX && node->tok.val.aux == TOK_DEFINE)
    {
        depth++;
        depth_rised = true;
    }

    PRINT("(");

    if(node->left)
        tree_print_node_(node->left, stream);

    if(node->tok.type == TYPE_FUNC || node->tok.type == TYPE_VAR)
        PRINT("\'%s\'", demangle(&node->tok));
    else if(node->tok.type == TYPE_AUX && (node->tok.val.aux == TOK_STATEMENT || node->tok.val.aux == TOK_DECISION))
        PRINT("\n%*s%s", depth * 4, "", demangle(&node->tok));
    else
        PRINT("%s", demangle(&node->tok));

    if(node->right)
        tree_print_node_(node->right, stream);
    
    PRINT(")");

    if(depth_rised)
        depth--;
}

void tree_write(Tree* tree, FILE* ostream)
{
    assert(ostream);

    tree_print_node_(tree->root, ostream);
}

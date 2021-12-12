#include <stdlib.h>

#include "Tree.h"

static const char TREE_OFILE[] = "tree.txt";
static FILE* OSTREAM = nullptr;

#define PRINT(format, ...) fprintf(stream, format, ##__VA_ARGS__)

static void tree_print_node_(Node* node)
{
    FILE* stream = OSTREAM;
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
        tree_print_node_(node->left);

    if(node->tok.type == TYPE_FUNC || node->tok.type == TYPE_VAR)
        PRINT("\'%s\'", demangle(&node->tok));
    else if(node->tok.type == TYPE_AUX && (node->tok.val.aux == TOK_STATEMENT || node->tok.val.aux == TOK_DECISION))
        PRINT("\n%*s%s", depth * 4, "", demangle(&node->tok));
    else
        PRINT("%s", demangle(&node->tok));

    if(node->right)
        tree_print_node_(node->right);
    
    PRINT(")");

    if(depth_rised)
        depth--;
}

void tree_write(Tree* tree)
{
    tree_print_node_(tree->root);
}

static void close_file_()
{
    if(fclose(OSTREAM) != 0)
        perror(__FILE__": file can't be succesfully closed");
}

void tree_write_init(FILE* ostream)
{
    if(ostream)
    {
        OSTREAM = ostream;
        return;
    }

    if(TREE_OFILE[0] != 0)
    {
        OSTREAM = fopen(TREE_OFILE, "w");

        if(OSTREAM)
        {
            atexit(&close_file_);            
            return;
        }
    }

    perror(__FILE__ ": can't open outfile");

    return;
}

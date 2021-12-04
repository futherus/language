#include <stdio.h>

#include "parser/parser.h"
#include "tree/Tree.h"
#include "dumpsystem/dumpsystem.h"

int main()
{
    tree_dump_init(dumpsystem_get_stream(log));
    tree_write_init();
    char buffer[] = "1*2*3*8;";

    Tree tree = {};
    parse(&tree, buffer, sizeof(buffer) - 1);

    tree_write(&tree);
    return 0;
}
#include <stdio.h>

#include "parser/parser.h"
#include "tree/Tree.h"
#include "dumpsystem/dumpsystem.h"

int main()
{
    tree_dump_init(dumpsystem_get_stream(log));
    tree_write_init();
    char buffer[] = "while(a + 3*b) { variable = 1*2*3*func(3, foo(1, 0, my_sin(2*y)), 8+x)+my_sin(1); return 5; }";

    Tree tree = {};
    parse(&tree, buffer, sizeof(buffer) - 1);

    tree_write(&tree);
    return 0;
}
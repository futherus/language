#include <stdio.h>

#include "parser/parser.h"
#include "tree/Tree.h"
#include "dumpsystem/dumpsystem.h"

int main()
{
    tree_dump_init(dumpsystem_get_stream(log));
    tree_write_init();
    //char buffer[] = "while(a >= b + c) { varelse = 1*2*3*func(3, foo(1, 0, my_sin(2*y)), 8+x)+my_sin(1); return 5; }";
    char buffer[] = R"(
    
    main()
    {
        var1 = 1 * 1;
        var2 = 1 && (2 * 3);
        var_unary_op = my_sin(5);
        var2_unary_op = !(1 + 2);

        return 0;
    }
    )";
    Tree tree = {};
    parse(&tree, buffer, sizeof(buffer) - 1);

    tree_write(&tree);
    return 0;
}
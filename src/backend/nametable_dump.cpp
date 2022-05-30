#include <stdio.h>
#include <assert.h>

#include "nametable.h"
#include "../config.h"

#ifdef VERBOSE

static FILE* DUMP_STREAM = nullptr;

#define PRINT(format, ...) fprintf(stream, format, ##__VA_ARGS__)

void vartable_dump(Variable_table* table)
{
    assert(table);
    FILE* stream = DUMP_STREAM;
    if(!stream)
        return;

    PRINT("\n\n<table class = \"log\" border=\"1\" style=\"border-collapse:collapse; border-color:E59E1F; border-width: 1px; width: 600px;\"><tbody>\n"
          "<tr><th colspan=\"4\" class = \"title\">Variable table</th></tr>\n");
        
    PRINT("<tr><td colspan=\"4\">\n\tsize: %ld\n</td></tr>\n"
          "<tr><td colspan=\"4\">\n\tcapacity: %ld\n</td></tr>\n",
          table->size, table->cap);
    
    PRINT("<tr><th>id</th><th>offset</th><th>size</th></tr>\n");
    
    if(!table->array)
    {
        PRINT("<tr><th colspan=\"4\" class = \"error\">\n\tDATA IS NULL\n</th></tr>\n"
              "</tbody></table>\n");
        return;
    }

    for(ptrdiff_t iter = 0; iter < table->size; iter++)
    {
        Variable* ptr = &table->array[iter];

        if(ptr->is_const)
            PRINT("<tr style = \"height: 40px; font-weight: bold;\">\n");
        else
            PRINT("<tr style = \"height: 40px;\">\n");

        PRINT("<td>  %s  </td>\n<td>  %ld  </td>\n<td>  %ld  </td>\n", ptr->id, ptr->offset, ptr->size);

        PRINT("</tr>\n");
    }

    PRINT("</tbody></table>\n\n\n");
}

void functable_dump(Function_table* table)
{
    assert(table);
    FILE* stream = DUMP_STREAM;
    if(!stream)
        return;

    PRINT("\n\n<table class = \"log\" border=\"1\" style=\"border-collapse:collapse; border-color:E59E1F; border-width: 1px; width: 600px;\"><tbody>\n"
          "<tr><th colspan=\"2\" class = \"title\">Function table</th></tr>\n");
        
    PRINT("<tr><td colspan=\"2\">\n\tsize: %ld\n</td></tr>\n"
          "<tr><td colspan=\"2\">\n\tcapacity: %ld\n</td></tr>\n",
          table->size, table->cap);
    
    PRINT("<tr><th>id</th><th>n_args</th></tr>\n");

    if(!table->array)
    {
        PRINT("<tr><th colspan=\"2\" class = \"error\">\n\tDATA IS NULL\n</th></tr>\n"
              "</tbody></table>\n");
        return;
    }

    for(ptrdiff_t iter = 0; iter < table->size; iter++)
    {
        Function* ptr = &table->array[iter];

        PRINT("<tr style = \"height: 40px;\">\n");

        PRINT("<td>  %s  </td>\n<td>  %ld  </td>\n", ptr->id, ptr->n_args);

        PRINT("</tr>\n");
    }

    PRINT("</tbody></table>\n\n\n");
}

void nametable_dump_init(FILE* dump_stream)
{
    if(!dump_stream)
        dump_stream = stderr;

    DUMP_STREAM = dump_stream;
}

#else // VERBOSE

void vartable_dump(Variable_table*)
{
    void(0);
}

void functable_dump(Function_table*)
{
    void(0);
}

void nametable_dump_init(FILE*)
{
    void(0);
}

#endif // VERBOSE
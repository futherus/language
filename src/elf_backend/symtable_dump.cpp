#include <stdio.h>
#include <assert.h>

#include "symtable.h"
#include "../config.h"

#ifdef VERBOSE

static FILE* DUMP_STREAM = nullptr;

#define PRINT(format, ...) fprintf(stream, format, ##__VA_ARGS__)

void symtable_dump(Symtable* tbl)
{
    assert(tbl);
    FILE* stream = DUMP_STREAM;
    if(!stream)
        return;

    PRINT("\n\n<table class = \"log\" border=\"1\" style=\"border-collapse:collapse; border-color:E59E1F; border-width: 1px; width: 800px;\"><tbody>\n"
          "<tr><th colspan=\"7\" class = \"title\">Symbol table</th></tr>\n");
        
    PRINT("<tr><td colspan=\"7\">\n\tsize: %lu\n</td></tr>\n"
          "<tr><td colspan=\"7\">\n\tcapacity: %lu\n</td></tr>\n",
          tbl->buffer_sz, tbl->buffer_cap);
    
    PRINT("<tr><th>type</th><th>id</th><th>offset</th><th>section</th><th>s_size</th><th>s_name</th><th>#args / size</th></tr>\n");
    
    if(!tbl->buffer)
    {
        PRINT("<tr><th colspan=\"7\" class = \"error\">\n\tDATA IS NULL\n</th></tr>\n"
              "</tbody></table>\n");
        return;
    }

    for(size_t iter = 0; iter < tbl->buffer_sz; iter++)
    {
        Symbol* ptr = &tbl->buffer[iter];

        switch(ptr->type)
        {
            case SYMBOL_TYPE_VARIABLE:
            {
                if(ptr->var.is_const)
                    PRINT("<tr style = \"height: 40px; font-weight: bold;\">\n");
                else
                    PRINT("<tr style = \"height: 40px;\">\n");
                
                PRINT("<td>  variable </td>\n"
                    "<td>  %s  </td>\n"
                    "<td>  0x%lx  </td>\n"
                    "<td>  %lu    </td>\n"
                    "<td>  %lu    </td>\n"
                    "<td>  %lu    </td>\n"
                    "<td>  %lu    </td>\n",
                    ptr->id, ptr->offset, ptr->section_descriptor, ptr->s_size, ptr->s_name, ptr->var.size);

                break;
            }
            case SYMBOL_TYPE_FUNCTION:
            {
                PRINT("<tr style = \"height: 40px;\">\n");
                PRINT("<td>  function </td>\n"
                    "<td>  %s  </td>\n"
                    "<td>  0x%lx  </td>\n"
                    "<td>  %lu    </td>\n"
                    "<td>  %lu    </td>\n"
                    "<td>  %lu    </td>\n"
                    "<td>  %lu    </td>\n",
                    ptr->id, ptr->offset, ptr->section_descriptor, ptr->s_size, ptr->s_name, ptr->func.n_args);

                break;
            }
            case SYMBOL_TYPE_NOTYPE:
            {
                PRINT("<tr style = \"height: 40px;\">\n");
                PRINT("<td>  notype </td>\n"
                    "<td>  </td>\n"
                    "<td>  </td>\n"
                    "<td>  </td>\n"
                    "<td>  </td>\n"
                    "<td>  </td>\n"
                    "<td>  </td>\n");

                break;
            }
            default:
            {
                assert(0 && "Unknown symbol type");
            }
        }

        PRINT("</tr>\n");
    }

    PRINT("</tbody></table>\n\n\n");
}

void localtable_dump(Localtable* tbl)
{
    assert(tbl);
    FILE* stream = DUMP_STREAM;
    if(!stream)
        return;

    PRINT("\n\n<table class = \"log\" border=\"1\" style=\"border-collapse:collapse; border-color:E59E1F; border-width: 1px; width: 800px;\"><tbody>\n"
          "<tr><th colspan=\"3\" class = \"title\">Local table</th></tr>\n");
        
    PRINT("<tr><td colspan=\"3\">\n\tsize: %lu\n</td></tr>\n"
          "<tr><td colspan=\"3\">\n\tcapacity: %lu\n</td></tr>\n",
          tbl->buffer_sz, tbl->buffer_cap);
    
    PRINT("<tr><th>id</th><th>offset</th><th>size</th></tr>\n");
    
    if(!tbl->buffer)
    {
        PRINT("<tr><th colspan=\"3\" class = \"error\">\n\tDATA IS NULL\n</th></tr>\n"
              "</tbody></table>\n");
        return;
    }

    for(size_t iter = 0; iter < tbl->buffer_sz; iter++)
    {
        Local_var* ptr = &tbl->buffer[iter];

        if(ptr->is_const)
            PRINT("<tr style = \"height: 40px; font-weight: bold;\">\n");
        else
            PRINT("<tr style = \"height: 40px;\">\n");
        
        PRINT("<td>  %s   </td>\n"
              "<td>  %d   </td>\n"
              "<td>  %lu  </td>\n",
              ptr->id, ptr->offset, ptr->size);

        PRINT("</tr>\n");
    }

    PRINT("</tbody></table>\n\n\n");
}

void symtable_dump_init(FILE* dump_stream)
{
    if(!dump_stream)
        dump_stream = stderr;

    DUMP_STREAM = dump_stream;
}

#undef PRINT

#else // VERBOSE

void symtable_dump(Symtable*)
{
    void(0);
}

void localtable_dump(Localtable*)
{
    void(0);
}

void symtable_dump_init(FILE*)
{
    void(0);
}

#endif // VERBOSE

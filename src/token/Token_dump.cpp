#include <stdio.h>
#include <assert.h>

#include "Token.h"

static FILE* DUMP_STREAM = nullptr;

#define PRINT(format, ...) fprintf(stream, format, ##__VA_ARGS__)

#define DEF_OP(NAME, STD_NAME, MANGLE)                  \
    case TOK_##MANGLE:                                  \
        PRINT("<td>  %s  </td>\n<td>  %s  </td>\n<td>  %s  </td>\n",     \
             (NAME), (STD_NAME), #MANGLE);              \
        break;                                          \

#define DEF_AUX(STD_NAME, MANGLE)                       \
    case TOK_##MANGLE:                                  \
        PRINT("<td>  --//--  </td>\n<td>  %s  </td>\n<td>  %s  </td>\n", \
              (STD_NAME), #MANGLE);                     \
        break;                                          \

#define DEF_KEY(NAME, STD_NAME, MANGLE)                 \
    case TOK_##MANGLE:                                  \
        PRINT("<td>  %s  </td>\n<td>  %s  </td>\n<td>  %s  </td>\n",     \
              (NAME), (STD_NAME), #MANGLE);             \
        break;                                          \

#define DEF_EMB(NAME, STD_NAME, MANGLE)                 \
    case TOK_##MANGLE:                                  \
        PRINT("<td>  %s  </td>\n<td>  %s  </td>\n<td>  %s  </td>\n",     \
              (NAME), (STD_NAME), #MANGLE);             \
        break;                                          \

void token_array_dump(Token_array* tok_arr)
{
    assert(tok_arr);

    if(!DUMP_STREAM)
        return;
    
    FILE* stream = DUMP_STREAM;
    
    PRINT("\n\n<table class = \"log\" border=\"1\" style=\"border-collapse:collapse; border-color:E59E1F; border-width: 1px; width: 600px;\"><tbody>\n"
          "<tr><th colspan=\"3\" class = \"title\">Token array</th></tr>\n");
    
    PRINT("<tr><td colspan=\"3\">\n\tsize: %ld\n</td></tr>\n"
          "<tr><td colspan=\"3\">\n\tcapacity: %ld\n</td></tr>\n"
          "<tr><td colspan=\"3\">\n\tposition: %ld\n</td></tr>\n"
          "<tr><td colspan=\"3\">\n\tEOF: %ld\n</td></tr>\n",
          tok_arr->size, tok_arr->cap, tok_arr->pos, tok_arr->eof);

    PRINT("<tr><th>name</th><th>std name</th><th>mangle</th></tr>\n");

    if(!tok_arr->data)
    {
        PRINT("<tr><th colspan=\"3\" class = \"error\">\n\tDATA IS NULL\n</th></tr>\n"
              "</tbody></table>\n");
        return;
    }
    
    for(ptrdiff_t iter = 0; iter < tok_arr->size; iter++)
    {
        if(iter == tok_arr->pos)
            PRINT("<tr style = \"height: 40px; color: yellow\">\n");
        else
            PRINT("<tr style = \"height: 40px;\">\n");
        switch(tok_arr->data[iter].type)
        {
            case TYPE_OP:
                switch(tok_arr->data[iter].val.op)
                {
                    #include "../reserved_operators.inc"

                    default:
                        assert(0);
                }
                break;
                
            case TYPE_AUX:
                switch(tok_arr->data[iter].val.aux)
                {
                    #include "../reserved_auxiliary.inc"

                    default:
                        assert(0);
                }
                break;
            
            case TYPE_KEYWORD:
                switch(tok_arr->data[iter].val.key)
                {
                    #include "../reserved_keywords.inc"

                    default:
                        assert(0);
                }
                break;
            
            case TYPE_EMBED:
                switch(tok_arr->data[iter].val.emb)
                {
                    #include "../reserved_embedded.inc"

                    default:
                        assert(0);
                }
                break;
            
            case TYPE_ID:
                PRINT("<td>  %s  </td>\n<td>  [%p]  </td>\n<td>  TYPE_ID  </td>\n", tok_arr->data[iter].val.name, tok_arr->data[iter].val.name);
                break;
            
            case TYPE_NUMBER:
                PRINT("<td>  %lg  </td>\n<td>  --//--  </td>\n<td>  TYPE_NUMBER  </td>\n", tok_arr->data[iter].val.num);
                break;
            
            case TYPE_EOF:
                PRINT("<td>  EOF  </td>\n<td>  --//--  </td>\n<td>  TYPE_EOF  </td>\n");
                break;
            
            default : case TYPE_NOTYPE :
                assert(0);
        }
        PRINT("</tr>\n");
    }
    PRINT("</tbody></table>\n\n\n");
}

#undef DEF_OP
#undef DEF_AUX
#undef DEF_KEY
#undef DEF_EMB

void token_nametable_dump(Token_nametable* tok_table)
{
    assert(tok_table);

    if(!DUMP_STREAM)
        return;
    
    FILE* stream = DUMP_STREAM;
    
    PRINT("<table class = \"log\" border=\"1\" style=\"border-collapse:collapse; border-color:E59E1F; border-width: 1px; width: 600px;\"><tbody>\n");
    PRINT("<tr><th colspan=\"2\" class = \"title\">Token nametable</th></tr>\n");

    PRINT("<tr><td colspan=\"2\">\n\tsize: %ld\n</td></tr>\n"
          "<tr><td colspan=\"2\">\n\tcapacity: %ld\n</td></tr>\n",
          tok_table->size, tok_table->cap);

    PRINT("<tr><th>name</th><th>pointer</th></tr>\n");

    if(!tok_table->name_arr)
    {
        PRINT("<tr><th colspan=\"2\" class = \"error\">\n\tDATA IS NULL\n</th></tr>\n"
              "</tbody></table>\n");
        return;
    }

    for(ptrdiff_t iter = 0; iter < tok_table->size; iter++)
    {
        if(tok_table->name_arr[iter])
            PRINT("<tr style=\"height: 40px\"><td>  %s  </td>\n<td>  [%p]  </td></tr>\n", tok_table->name_arr[iter], tok_table->name_arr[iter]);
        else
            PRINT("<tr><td colspan=\"2\" class = \"error\">\n\tDATA IS NULL\n</td></tr>\n");
    }

    PRINT("</tbody></table>\n");
}

void token_dump_init(FILE* dump_stream)
{
    if(!dump_stream)
        dump_stream = stderr;

    DUMP_STREAM = dump_stream;
}

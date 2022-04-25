#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "Token.h"
#include "../reserved_names.h"
#include "../common/dumpsystem.h"

static const ptrdiff_t TOK_MIN_CAP     = 8;
static const ptrdiff_t TOK_CAP_MULTPLR = 2;

static token_err array_resize_(Token_array* tok_arr)
{
    assert(tok_arr);

    ptrdiff_t new_cap = 0;
    if(tok_arr->cap == 0)
        new_cap = TOK_MIN_CAP;
    else
        new_cap = tok_arr->cap * TOK_CAP_MULTPLR;

    Token* new_data = (Token*) realloc(tok_arr->data, (size_t) new_cap * sizeof(Token));
    ASSERT_RET$(new_data, TOKEN_BAD_ALLOC);

    tok_arr->data = new_data;
    tok_arr->cap  = new_cap;

    return TOKEN_NOERR;
}

token_err token_array_add(Token_array* tok_arr, const Token* token)
{
    assert(token && tok_arr);

    if(tok_arr->cap == tok_arr->size)
        PASS$(!array_resize_(tok_arr), return TOKEN_BAD_ALLOC; );
    
    *(tok_arr->data + tok_arr->size) = *token;
    tok_arr->size++;

    return TOKEN_NOERR;
}

void token_array_dstr(Token_array* tok_arr)
{
    assert(tok_arr);

    if(tok_arr->data)
        free(tok_arr->data);

    tok_arr->cap = 0;
    tok_arr->size = 0;
    tok_arr->eof = 0;
    tok_arr->size = 0;
}

static token_err nametable_resize_(Token_nametable* tok_table)
{
    assert(tok_table);

    ptrdiff_t new_cap = 0;
    if(tok_table->cap == 0)
        new_cap = TOK_MIN_CAP;
    else
        new_cap = tok_table->cap * TOK_CAP_MULTPLR;
    
    char** new_ptr = (char**) realloc(tok_table->name_arr, (size_t) new_cap * sizeof(char*));
    ASSERT_RET$(new_ptr, TOKEN_BAD_ALLOC);

    tok_table->name_arr = new_ptr;
    tok_table->cap = new_cap;

    return TOKEN_NOERR;
}

static ptrdiff_t nametable_find_(Token_nametable* tok_table, const char name[], ptrdiff_t name_sz)
{
    assert(tok_table);

    for(ptrdiff_t iter = 0; iter < tok_table->size; iter++)
    {
        ptrdiff_t len = (ptrdiff_t) strlen(tok_table->name_arr[iter]);

        if(len == name_sz)
            if(strncmp(name, tok_table->name_arr[iter], (size_t) name_sz) == 0)
                return iter;
    }

    return -1;
}
token_err token_nametable_add(Token_nametable* tok_table, char** dst_ptr, const char name[], ptrdiff_t name_sz)
{
    assert(tok_table && name);

    ptrdiff_t indx = nametable_find_(tok_table, name, name_sz);
    if(indx != -1)
    {
        *dst_ptr = tok_table->name_arr[indx];
        return TOKEN_NOERR;
    }

    if(tok_table->cap == tok_table->size)
        PASS$(!nametable_resize_(tok_table), return TOKEN_BAD_ALLOC; );
    
    char* new_ptr = (char*) calloc((size_t) name_sz + 1, sizeof(char));
    ASSERT_RET$(new_ptr, TOKEN_BAD_ALLOC);

    memcpy(new_ptr, name, (size_t) name_sz);
    new_ptr[name_sz] = '\0';

    tok_table->name_arr[tok_table->size] = new_ptr;
    tok_table->size++;

    *dst_ptr = new_ptr;

    return TOKEN_NOERR;
}

void token_nametable_dstr(Token_nametable* tok_table)
{
    assert(tok_table);

    for(ptrdiff_t iter = 0; iter < tok_table->size; iter++)
        free(tok_table->name_arr[iter]);

    if(tok_table->name_arr)
        free(tok_table->name_arr);
    
    tok_table->cap = 0;
    tok_table->size = 0;
    tok_table->name_arr = nullptr;
}

#define DEF_OP(NAME, STD_NAME, MANGLE)          \
    case(TOK_##MANGLE):                         \
        sprintf(buffer, "%s", (STD_NAME));      \
        break;                                  \

#define DEF_KEY(NAME, STD_NAME, MANGLE)         \
    case(TOK_##MANGLE):                         \
        sprintf(buffer, "%s", (STD_NAME));      \
        break;                                  \
    
#define DEF_EMB(NAME, STD_NAME, MANGLE)         \
    case(TOK_##MANGLE):                         \
        sprintf(buffer, "%s", (STD_NAME));      \
        break;                                  \

#define DEF_AUX(NAME, MANGLE)                   \
    case(TOK_##MANGLE):                         \
        sprintf(buffer, "%s", (NAME));          \
        break;                                  \

char* std_demangle(const Token* tok)
{
    assert(tok);
    
    static char buffer[BUFSIZ] = "";

    switch(tok->type)
    {
        case TYPE_NUMBER:
            sprintf(buffer, "%lg", tok->val.num);
            break;

        case TYPE_ID :
            sprintf(buffer, "%s", tok->val.name);
            break;

        case TYPE_OP:
        {
            switch(tok->val.op)
            {
                #include "../reserved_operators.inc"

                default:
                    assert(0);
            }

            break;
        }
        case TYPE_KEYWORD:
        {
            switch(tok->val.key)
            {
                #include "../reserved_keywords.inc"

                default:
                    assert(0);
            }

            break;
        }
        case TYPE_EMBED:
        {
            switch(tok->val.emb)
            {
                #include "../reserved_embedded.inc"

                default:
                    assert(0);
            }

            break;
        }
        case TYPE_AUX:
        {
            switch(tok->val.aux)
            {
                #include "../reserved_auxiliary.inc"

                default:
                    assert(0);
            }
            
            break;
        }
        case TYPE_EOF:
        {
            sprintf(buffer, "EOF");
            break;
        }
        default: case TYPE_NOTYPE :
            assert(0 && tok->type);
    }

    return buffer;
}

#undef DEF_OP
#undef DEF_KEY
#undef DEF_EMB
#undef DEF_AUX

#define DEF_OP(NAME, STD_NAME, MANGLE)          \
    case(TOK_##MANGLE):                         \
        sprintf(buffer, "%s", (NAME));          \
        break;                                  \

#define DEF_KEY(NAME, STD_NAME, MANGLE)         \
    case(TOK_##MANGLE):                         \
        sprintf(buffer, "%s", (NAME));          \
        break;                                  \
    
#define DEF_EMB(NAME, STD_NAME, MANGLE)         \
    case(TOK_##MANGLE):                         \
        sprintf(buffer, "%s", (NAME));          \
        break;                                  \

#define DEF_AUX(NAME, MANGLE)                   \
    case(TOK_##MANGLE):                         \
        sprintf(buffer, "%s", (NAME));          \
        break;                                  \

char* demangle(const Token* tok)
{
    assert(tok);
    
    static char buffer[BUFSIZ] = "";

    switch(tok->type)
    {
        case TYPE_NUMBER:
        {
            sprintf(buffer, "%lg", tok->val.num);
            break;
        }
        case TYPE_ID :
        {
            if(strcmp(MAIN_STD_NAME, tok->val.name) == 0)
                sprintf(buffer, "%s", MAIN_NAME);
            else
                sprintf(buffer, "%s", tok->val.name);

            break;
        }
        case TYPE_OP:
        {
            switch(tok->val.op)
            {
                #include "../reserved_operators.inc"

                default:
                    assert(0);
            }

            break;
        }
        case TYPE_KEYWORD:
        {
            switch(tok->val.key)
            {
                #include "../reserved_keywords.inc"

                default:
                    assert(0);
            }

            break;
        }
        case TYPE_EMBED:
        {
            switch(tok->val.emb)
            {
                #include "../reserved_embedded.inc"

                default:
                    assert(0);
            }

            break;
        }
        case TYPE_AUX:
        {
            switch(tok->val.aux)
            {
                #include "../reserved_auxiliary.inc"

                default:
                    assert(0);
            }
            
            break;
        }
        case TYPE_EOF:
        {
            sprintf(buffer, "EOF");
            break;
        }
        default: case TYPE_NOTYPE :
        {
            assert(0 && tok->type);
        }
    }

    return buffer;
}

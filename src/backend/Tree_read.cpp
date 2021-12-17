#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "backend.h"
#include "../dumpsystem/dumpsystem.h"

static void clean_whitespaces_(const char data[], ptrdiff_t* pos)
{
    assert(data && pos);

    while(isspace(data[*pos]))
        (*pos)++;
}

static bool get_number(const char data[], double* num, int* n_read)
{
    if(isdigit(*data))
    {
        sscanf(data, "%lg%n", num, n_read);
        return true;
    }

    return false;
}

static void wordlen_(const char data[], int* n_read)
{
    assert(data && n_read);
    assert(*data == '\'');

    (*n_read)++;

    int last_quote = 0;
    while(data[*n_read] != ')' && data[*n_read] != '(' && data[*n_read])
    {
        if(data[*n_read] == '\'')
            last_quote = *n_read;
        
        (*n_read)++;
    }

    *(n_read) = last_quote + 1;
}

#define DEF_KEY(NAME, STD_NAME, MANGLE)                             \
    if(strncmp(data + pos, (STD_NAME), sizeof(STD_NAME) - 1) == 0)  \
    {                                                               \
        tmp.type = TYPE_KEYWORD;                                    \
        tmp.val.key = TOK_##MANGLE;                                 \
        pos += sizeof(STD_NAME) - 1;                                \
        PASS$(!token_array_add(tok_arr, &tmp), return TOKEN_BAD_ALLOC; ); \
        continue;                                           \
    }                                                       \

#define DEF_OP(NAME, STD_NAME, MANGLE)                    \
    if(strncmp(data + pos, (STD_NAME), sizeof(STD_NAME) - 1) == 0)  \
    {                                                       \
        tmp.type = TYPE_OP;                                 \
        tmp.val.op = TOK_##MANGLE;                          \
        pos += sizeof(STD_NAME) - 1;                            \
        PASS$(!token_array_add(tok_arr, &tmp), return TOKEN_BAD_ALLOC; ); \
        continue;                                           \
    }                                                       \

#define DEF_EMB(NAME, STD_NAME, MANGLE)                   \
    if(strncmp(data + pos, (STD_NAME), sizeof(STD_NAME) - 1) == 0)  \
    {                                                       \
        tmp.type = TYPE_EMBED;                              \
        tmp.val.emb = TOK_##MANGLE;                         \
        pos += sizeof(STD_NAME) - 1;                            \
        PASS$(!token_array_add(tok_arr, &tmp), return TOKEN_BAD_ALLOC; ); \
        continue;                                           \
    }                                                       \

#define DEF_AUX(STD_NAME, MANGLE)                                         \
    if(strncmp(data + pos, (STD_NAME), sizeof(STD_NAME) - 1) == 0)        \
    {                                                                     \
        tmp.type = TYPE_AUX;                                              \
        tmp.val.aux = TOK_##MANGLE;                                       \
        pos += sizeof(STD_NAME) - 1;                                      \
        PASS$(!token_array_add(tok_arr, &tmp), return TOKEN_BAD_ALLOC; ); \
        continue;                                                         \
    }                                                                     \

static token_err lexer_(Token_array* tok_arr, Token_nametable* tok_table, const char data[], ptrdiff_t data_sz)
{
    assert(data && tok_arr && tok_table);
    ASSERT_RET$(data_sz > 0, TOKEN_EMPTY_DATA);
    
    ptrdiff_t pos = 0;
    Token tmp = {};

    while(true)
    {
        clean_whitespaces_(data, &pos);   
        if(pos == data_sz)
            break;
        
        int n_read = 0;
        tmp = {};

        if(get_number(data + pos, &tmp.val.num, &n_read))
        {
            tmp.type = TYPE_NUMBER;
            PASS$(!token_array_add(tok_arr, &tmp), return TOKEN_BAD_ALLOC; );

            pos += n_read;
            continue;
        }
        if(data[pos] == '\'')
        {
            wordlen_(data + pos, &n_read);

            tmp.type = TYPE_ID;
            PASS$(!token_nametable_add(tok_table, &tmp.val.name, data + pos + 1, n_read - 2), return TOKEN_BAD_ALLOC; );
            PASS$(!token_array_add(tok_arr, &tmp), return TOKEN_BAD_ALLOC; );

            pos += n_read;
            continue;
        }

        #include "../reserved_operators.inc"
        #include "../reserved_keywords.inc"
        #include "../reserved_embedded.inc"
        #include "../reserved_auxiliary.inc"

        ASSERT$(0, Tree_read: unknown token, return TOKEN_UNKNWN_SYMB; );        
    }

    tok_arr->eof = tok_arr->size;
    tmp.type = TYPE_EOF;
    PASS$(!token_array_add(tok_arr, &tmp), return TOKEN_BAD_ALLOC; );

    return TOKEN_NOERR;
}

#undef DEF_OP
#undef DEF_KEY
#undef DEF_EMB

static void consume(Token* tok, Token_array* tok_arr)
{
    assert(tok);
    assert(tok_arr->pos <= tok_arr->eof);

    *tok = tok_arr->data[tok_arr->pos];
    tok_arr->pos++;
}

static void peek(Token* tok, ptrdiff_t offset, Token_array* tok_arr)
{
    assert(tok);

    if(tok_arr->pos + offset < 0 || tok_arr->pos + offset > tok_arr->eof)
    {
        *tok = tok_arr->data[tok_arr->eof];
        
        return;
    }

    *tok = tok_arr->data[tok_arr->pos + offset];
}

#define format_error(TOK) ASSERT$(0, Tree read: wrong format, return BACKEND_FORMAT_ERROR; )

static backend_err read_node_(Node** base, Tree* tree, Token_array* tok_arr)
{
    assert(base);
    
    Token tok = {};

    consume(&tok, tok_arr);
    if(tok.type != TYPE_OP || tok.val.op != TOK_LRPAR)
        format_error(&tok);
    
    peek(&tok, 0, tok_arr);
    if(tok.type == TYPE_OP && tok.val.op == TOK_LRPAR)
        PASS$(!read_node_(base, tree, tok_arr), return BACKEND_TREE_FAIL; );
    
    consume(&tok, tok_arr);

    Node* tmp = *base;
    PASS$(!tree_add(tree, base, &tok), return BACKEND_TREE_FAIL; );
    (*base)->left = tmp;

    peek(&tok, 0, tok_arr);
    if(tok.type == TYPE_OP && tok.val.op == TOK_LRPAR)
        PASS$(!read_node_(&(*base)->right, tree, tok_arr), return BACKEND_TREE_FAIL; );
    
    consume(&tok, tok_arr);
    if(tok.type != TYPE_OP || tok.val.op != TOK_RRPAR)
        format_error(&tok);

    return BACKEND_NOERR;
}

backend_err tree_read(Tree* tree, Token_nametable* tok_table, const char data[], ptrdiff_t data_sz)
{
    assert(tree && tok_table && data);

    Token_array tok_arr = {};

    token_err token_error = lexer_(&tok_arr, tok_table, data, data_sz);
    token_array_dump(&tok_arr);
    token_nametable_dump(tok_table);

    if(token_error)
    {
        token_array_dstr(&tok_arr);
        PASS$(!token_error, return BACKEND_READ_FAIL; );
    }

    backend_err tree_error = read_node_(&tree->root, tree, &tok_arr);
    if(tree_error)
    {
        token_array_dump(&tok_arr);
        tree_dump(tree, "Dump");
    }

    token_array_dstr(&tok_arr);

    PASS$(!tree_error, return BACKEND_READ_FAIL; );
    
    return BACKEND_NOERR;
}
